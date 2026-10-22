# C++ 关键字与可调用对象

这一篇把零散语法按用途放在一起：先看类型转换和类型信息，再看存储与定义规则，最后把函数指针、Lambda、`std::function` 串起来。对象所有权和转发的细节放在[指针与引用](pointers-and-references.md)，排序实现放在[STL 与常用算法](stl-and-algorithms.md)。

## 四种显式类型转换

选择 cast 时先问：是在做数值或继承关系转换，检查动态类型，修改访问限定，还是重新解释底层表示？四种 cast 表达的意图不同，不能互相替代。

| 写法 | 主要用途 | 需要自己保证什么 |
| --- | --- | --- |
| `static_cast<T>(x)` | 数值转换、明确的类型转换、允许的继承转换等 | 不检查向下转换时真实对象类型；数值转换也可能丢失信息 |
| `dynamic_cast<T>(x)` | 类层次中的受检查转换 | 运行时检查通常要求多态源类型；对象必须仍然有效 |
| `const_cast<T>(x)` | 添加/移除指针、引用访问路径上的 `const` / `volatile` | 去掉限定不代表原对象真的可修改 |
| `reinterpret_cast<T>(x)` | 语言允许的底层指针/整数等重新解释 | 对齐、生命周期、可访问类型等规则仍然存在 |

### `static_cast`：转换关系由编译器检查

它不是“所有转换都不安全”：例如 `int` 转 `double`、无歧义的公开派生类转基类都有明确规则。危险的典型情形是向下转换，编译器允许 `Base*` 转 `Derived*`，却不在运行时验证对象是否真是 `Derived`。

编译器需要足够的类型定义来判断转换是否合法、是否调整地址。转换可能生成数值变换或指针调整指令，也可能优化为无操作，不存在统一的机器码形式。兄弟基类不能直接静态侧向转换，相关示例见[继承与对象转换](classes-inheritance-and-polymorphism.md#向下转换和侧向转换)。[标准草案：static_cast](https://eel.is/c++draft/expr.static.cast)

### `dynamic_cast`：失败也有定义

常见向下、侧向转换依赖运行时类型信息 RTTI；源类需为多态类，即有虚函数（包括继承来的虚函数）。普通向上转换是特殊情况，并不要求源类一定有虚函数。目标通常为完整类类型的指针/引用，也支持由多态指针转换到 `void*` 取得最派生对象地址。

- `dynamic_cast<T*>(p)` 失败返回 `nullptr`。
- `dynamic_cast<T&>(obj)` 失败抛出 `std::bad_cast`。

RTTI 不等于虚表：非多态类型的 `typeid` 可由静态类型确定；通过多态对象取得动态类型信息时，主流 ABI 常通过虚表附近的元数据实现。运行时转换有检查成本，但不能脱离实现、继承结构和调用频率断言“性能很烂”，热点中应测量。[标准草案：dynamic_cast](https://eel.is/c++draft/expr.dynamic.cast)

### `const_cast` 与 `volatile`

`const_cast` 修改访问路径的限定，不负责复制对象，也不会让真正的只读对象获得可写性。

```cpp
int value = 42;
const int* view = &value;
*const_cast<int*>(view) = 7;     // 可以：实际对象不是 const

const int fixed = 42;
// *const_cast<int*>(&fixed) = 7; // 未定义行为：修改真正的 const 对象
```

不应使用“它不生成任何机器指令”作为语义保证；关键是它不改变对象本身的 const 属性。[标准草案：cv 限定](https://eel.is/c++draft/dcl.type.cv)

`volatile` 让对指定对象的访问成为实现必须按相应规则处理的可观察行为，常用于设备寄存器等底层场景。原先的“每次都读写物理内存、绝不重排任何访问”太强：它不保证绕过 CPU 缓存，不提供原子性，不建立线程间同步关系，也不能充当普通内存访问的通用屏障。线程共享数据应使用原子操作或锁，设备语义则需结合平台文档。

### `reinterpret_cast` 不创建目标对象

```cpp
int value = 42;
float* p = reinterpret_cast<float*>(&value);
// float f = *p;                // 未定义行为：不能把 int 对象这样当 float 读
```

换了指针类型，不等于原地址上已经存在一个合法 `float` 对象。它也不是任意类型之间都能使用的“按位转换”。若要复制可平凡复制类型的对象表示，可按条件使用 `std::memcpy`，C++20 起可考虑 `std::bit_cast`；两者与通过错误类型指针访问对象是不同操作。[标准草案：reinterpret_cast](https://eel.is/c++draft/expr.reinterpret.cast)

## `sizeof`：看静态类型，不看动态对象

`sizeof` 的结果是字节数，类型为 `std::size_t`。通常在编译期根据类型布局得到结果，其操作数不求值。

```cpp
int arr[5]{};
int* p = arr;
// sizeof(arr) == 5 * sizeof(int)
// sizeof(p)   == sizeof(int*)
```

数组表达式作为 `sizeof` 的操作数时不退化。函数形参写成 `int arr[]` 则已经被调整为指针，在函数体内 `sizeof(arr)` 得到指针大小。

若 `Base* p` 指向派生对象，`sizeof(p)` 是指针大小，`sizeof(*p)` 是静态类型 `Base` 的大小，不会查询派生对象的完整大小。引用表达式的 `sizeof` 也是被引用类型的大小。[标准草案：sizeof](https://eel.is/c++draft/expr.sizeof)

## `static`：根据出现的位置判断含义

### 局部静态变量：只初始化一次，跨调用保存状态

```cpp
void f() {
    static int count = 0;
    ++count;
}
```

`count` 的名字仍只在当前代码块可见，但它具有静态存储期。需要动态初始化时，首次执行到声明处完成初始化；常量初始化等可能更早发生。后续调用保留此前的值。

C++11 起，局部静态变量的初始化具有线程安全保证；这不保证之后的 `++count` 是线程安全的。静态存储期是语言概念，不应简单当成固定的“静态区”物理位置。[标准草案：块作用域静态变量](https://eel.is/c++draft/stmt.dcl)

### 命名空间变量和函数：内部链接

```cpp
static int g_value = 10;
static void helper() {}
```

这里的重点是内部链接：不同翻译单元里的同名实体彼此独立，其他翻译单元不能用普通 `extern` 声明引用到这里的这个实体。翻译单元大致是一个 `.cpp` 加上它包含的头文件，而不只是文件文本本身。

命名空间变量即使不写 `static` 通常也有静态存储期，所以不能把两个含义混淆。内部实现也常放进匿名命名空间：

```cpp
namespace {
    int g_value = 10;
    void helper() {}
}
```

### 静态数据成员：类共享的数据

```cpp
class A {
public:
    static int count;          // 声明
};
int A::count = 0;              // 一处定义
```

它不属于某个具体对象，不占每个对象的独立成员空间。C++17 起可以直接定义 inline 静态成员：

```cpp
class A {
public:
    inline static int count = 0;
};
```

### 静态成员函数：没有隐式对象

```cpp
class A {
    inline static int count = 0; // C++17
public:
    static void foo() { ++count; }
};
// A::foo();
```

静态成员函数没有隐式 `this`，可以直接使用静态成员，通过 `A::foo()` 调用。它不能直接访问某个对象的非静态成员，但可以显式接收对象并访问该对象。

## `inline` 与宏不是一回事

`inline` 不强制调用处展开。对通常的头文件函数/变量，它的重要作用是允许满足 ODR 条件的多处定义共同表示同一个实体；不仅要定义文本一致，相关名字解析等也必须符合规则。C++17 才引入 inline 变量，模块中的规则还需单独考虑。

| 对比 | inline 函数 | 函数式宏 |
| --- | --- | --- |
| 处理阶段 | C++ 语义分析 | 预处理文本替换 |
| 类型、作用域与重载 | 遵守语言规则 | 不提供对应的 C++ 类型/作用域机制 |
| 参数求值 | 每个实参表达式按函数调用规则求值 | 替换后可能求值多次，也可能不求值 |
| 调试 | 保有函数语义，通常更方便 | 展开结果可能难对应原代码 |
| 是否展开 | 由优化器决定 | 是文本展开，不是函数内联优化 |

```cpp
#define SQUARE(x) ((x) * (x))
inline int square(int x) { return x * x; }
// SQUARE(++i) 有重复求值问题；square(++i) 只递增一次。
```

类内定义的普通成员函数通常隐式 inline；模板也常放在头文件，但模板可多处定义有自己的规则，不能把两者完全等同。[标准草案：inline](https://eel.is/c++draft/dcl.inline)

## 从函数指针到 Lambda

### 函数指针只有调用入口

```cpp
int Add(int a, int b) { return a + b; }
int (*addPtr)(int, int) = Add;
// addPtr(1, 2) == 3
```

普通函数指针不携带一次具体调用所需的额外状态。例如“每次相加还要加一个运行时的 `base`”，就还需要另外保存 `base`。

### Lambda 是带调用运算符的闭包对象

```cpp
int base = 10;
auto add = [base](int a, int b) {
    return base + a + b;
};
```

可以用下面的近似模型理解它，但这不是编译器必须生成的源码布局：

```cpp
struct ClosureModel {
    int base;
    int operator()(int a, int b) const { return base + a + b; }
};
```

`[x]` 保存值副本；`[&x]` 引用外部变量，不延长那个变量的生命周期。普通值捕获 Lambda 的调用运算符默认是 `const`；使用 `mutable` 可修改闭包中的副本，不等于修改原变量。

在成员函数中，`[this]` 保存当前对象的指针，对象销毁后再调用可能悬空。C++17 的 `[*this]` 复制当前对象到闭包中，但是否真正独立还取决于对象的拷贝语义，例如裸指针成员复制后仍可能指向同一资源。

无捕获、满足对应条件的 Lambda 可以转换成匹配的函数指针：

```cpp
auto lambda = [](int a, int b) { return a + b; };
int (*lambdaPtr)(int, int) = lambda;
```

捕获 Lambda 不能直接这样转换，因为函数指针没有地方保存捕获状态。[标准草案：Lambda](https://eel.is/c++draft/expr.prim.lambda)

## `std::function`：用统一签名保存可调用对象

```cpp
#include <functional>

int Add(int a, int b) { return a + b; }
struct Functor {
    int operator()(int a, int b) const { return a + b; }
};
auto lambda = [base = 10](int a, int b) { return base + a + b; };

std::function<int(int, int)> f1 = Add;
std::function<int(int, int)> f2 = Functor{};
std::function<int(int, int)> f3 = lambda;
```

三者类型不同，但都能按 `int(int, int)` 调用，因此可放进同一种包装器。它不是“任何对象都能装”：目标必须满足签名和构造约束，`std::function` 保存的目标还必须可复制。

### 类型擦除的实现模型

```text
std::function
├─ storage：闭包对象，或指向它的指针
├─ invoke：知道怎样调用这个具体类型的入口
└─ manager：知道怎样复制、移动、销毁目标的管理逻辑
```

对于上面的 `[base = 10]`，存储中保留 `base`；包装器的调用入口知道将其还原为什么具体类型：

```cpp
// 原理示意，非标准库实际源码
int InvokeClosure(void* storage, int a, int b) {
    auto* object = static_cast<ClosureModel*>(storage);
    return (*object)(a, b);
}
```

外部只看到统一的 `std::function<int(int, int)>`，具体闭包类型被隐藏；类型专属的调用与管理函数负责处理它。这就是类型擦除，不是丢掉了调用所需的全部类型知识。

小对象可能直接放在包装器内部，其他对象可能在堆上分配。小对象优化的容量、对齐限制和适用条件由实现决定，不是“只要够小就一定内嵌”。

### 使用时关注三种成本

- 间接调用可能阻碍内联；目标足够明确时，优化器仍可能消除部分开销。
- 保存某些目标会发生分配，复制包装器也可能复制状态或分配内存。
- 持有闭包不等于持有闭包引用的对象，引用捕获和 `this` 仍可能悬空。

捕获 `std::unique_ptr` 的仅可移动 Lambda 不能直接放进 `std::function`。可以根据接口选用模板接收可调用对象，或在支持 C++23 时考虑 `std::move_only_function`。空 `std::function` 可以通过布尔判断检查，直接调用会抛 `std::bad_function_call`。[标准草案：std::function](https://eel.is/c++draft/func.wrap.func)

## `union`：共享存储不等于成员同时有效

`union` 的非静态数据成员共享存储。大小足以容纳最大成员，并满足对齐要求，因此可能有尾部填充。

```cpp
union U {
    int i;
    float f;
};
U u;
u.i = 10;                     // i 为活动成员
u.f = 3.14f;                  // 此例切换为 f
// 之后不能普遍认为读取 u.i 是合法的位模式转换。
```

一般同一时刻只有一个活动成员。对于含非平凡构造/析构的成员，切换还需要正确管理生命周期，不能照搬上面标量赋值的做法。共同初始序列等少数例外也不能推广为任意类型别名访问。

`union` 可以有成员函数、构造和析构函数，不能有基类、不能作为基类，也不能有虚函数。[标准草案：union](https://eel.is/c++draft/class.union)

## 排序笔记的对应位置

原笔记中的 `sort` 不属于语言关键字。`std::sort` 常见实现采用内省排序：快速排序处理大区间、递归深度受限时转堆排序、小区间用插入排序。标准规定排序结果和复杂度要求，不强制这个实现组合；完整说明见[算法与容器配合](stl-and-algorithms.md#算法与容器配合)。
