# C++ 关键字与可调用对象

## cast相关：

### static_cast：编译期转换

#### 特点：

```
不安全
编译器要有视野
底层直接生成对应的机器指令
不会安全检查
```

### dynamic_cast：带多态的类层次结构中的运行时安全转型，目标类型必须是指针或引用

#### 特点：

```
依靠RTTI
性能很烂，尤其继承链比较长的时候
RTTI 本身不必然依赖虚表，非多态类型的 typeid 可以在编译期确定类型。但对于通过基类指针或引用识别对象真实派生类型的 dynamic_cast 和 typeid(*p)，对象必须是多态对象。主流 ABI 通常把 RTTI 指针放进虚表相关结构中，因此这类运行时 RTTI 查询通常通过虚表获得类型信息。
```

#### 失败返还：

std::bad_cast：dynamic_cast<T&> 失败时抛出它，而 dynamic_cast<T*> 失败时返回 nullptr。

### const_cast：用于添加或移除 const / volatile 限定

#### volatile是什么.

默认情况下，编译器为了优化可能会对变量做各种假设，比如把值缓存到寄存器、把重复读取合并成一次。volatile 告诉编译器：

- 不要缓存这个变量的值 —— 每次访问都必须真正从内存读取（读），每次都真正写入内存（写）。
- 不要对它的访问做重排、合并或删除 —— 编译器必须按你写的顺序对 volatile 变量做每次操作

#### 特点：

```
不改变地址，也不生成机器指令，只改变类型系统中的限定，修改原生 const 对象是未定义行为
```

### reinterpret_cast  按位或地址层面重新解释类型，几乎不做语义检查

通常只是让编译器把同一段比特按另一种类型解释。reinterpret_cast 是“换一种类型看同一个地址或位模式”，但它不会修改数据，也不会创建目标类型的对象。
可以把它理解为：只换标签，不换里面的东西。
int value 42；
转成float*，*p 在语法上的类型是 float 左值，但读取它是未定义行为：

## sizeof

原理：编译器在编译阶段已经确定了每个表达式的静态类型和该类型的对象布局，sizeof直接查询这个编译期布局信息。
所以基类的动态指针，sizeof为编译器看到的size。并且数组指针sizeof不会退化。

## sort：内省排序

规模较小：插入排序

否则快速排序，如果递归深度过深，变成堆排序

## static：

### 局部变量

```
void f() {
    static int count = 0;
    ++count;
}
```

特点：
作用域仍然是当前代码块；
生命周期从第一次初始化开始，一直到程序结束；
只初始化一次；后续调用会保留上次的值；
C++11 起，局部静态变量的初始化具有线程安全保证。它通常具有静态存储期，但不一定简单等同于某个固定的“静态区”。

### 命名空间作用域变量

static int g_value = 10;
这里的重点是：具有静态存储期，本来全局变量也有；

#### 特点：

static 使它具有内部链接；
只能在当前 .cpp 文件中访问；
其他翻译单元无法通过 extern 使用它。
现代 C++ 更常用匿名命名空间：
namespace {
    int g_value;
}

### 命名空间作用域普通函数

static void helper() {}

#### 特点：

含义与命名空间作用域变量类似：
函数只能在当前翻译单元使用；
具有内部链接；
防止与其他 .cpp 中的同名函数冲突。

### 类的静态数据成员

class A {
public:
    static int count;
};

#### 特点：

属于类，而不是某个对象；
所有对象共享同一份数据；
不占每个对象的独立成员空间；
通常需要在类外提供定义：
int A::count = 0;
C++17 起可以写成：
class A {
public:
    inline static int count = 0;
};
这样可以直接类内定义。

### 类的静态成员函数

class A {
public:
    static void foo();
};

#### 特点：

属于类，不属于某个具体对象；
没有隐式 this 指针；
不能直接访问非静态成员；
可以通过类名调用：A::foo();
它可以访问静态成员：
class A {
    static int count;
public:
    static void foo() {
        ++count;
    }
};

## inline和#define：

inline 的真实核心含义不是“强制内联展开”，而是：允许同一个函数或变量在多个翻译单元中定义，只要这些定义等价，链接时视为同一个实体。

```
inline函数	             宏
有类型检查	             纯文本替换，无类型检查
参数通常只求值一次	      参数可能被重复求值
遵守作用域和重载规则	  不遵守 C++ 作用域/类型规则
调试更友好	             容易产生难定位的问题
不保证真正内联	         没有“内联调用”概念
可用于模板、类成员函数	  只是预处理替换
```

## lamda:

### 原理：生成一个匿名类，重载()；

### 捕获：

int x;      // 值捕获：保存一份副本
int& y;     // 引用捕获：引用原变量（悬空引用）

### this与*this

前者单纯捕获地址，后者拷贝一份对象。前者会有悬空引用问题。

## std::function

### 函数指针为什么不能绑定所有的lamada

```
函数指针：只有代码地址，不能保存状态。
捕获 Lambda：代码 + 捕获状态。
无捕获 Lambda：只有代码逻辑，可以转函数指针。
```

函数指针只保存代码地址
std::function保存整个Lambda对象

### std::funtion： “可以保存任意可调用对象的统一包装器

#### int Add(int a, int b);

```
int (*addPtr)(int, int) = Add;
int result = addPtr(1, 2); // 3
```

函数指针可以保存地址，但如果有捕获，就会出问题，因为指针只是一个指针，记录不了别的信息。如：

```
auto lambda = [](int a, int b) {
    return a + b;
};

int (*lambdaPtr)(int, int) = lambda;
```

可以用函数指针，但如果[]里面有别的内容，就不行，会编译错误。

#### 但 std::function可以做到：

```
int Add(int a, int b);

struct Functor {
    int operator()(int a, int b);
};

auto lambda = [base = 10](int a, int b) {
    return base + a + b;
};
```

都可用std::function<int(int, int)>来进行保存

#### 底层原理：其实是一个类对象

函数指针近似只有函数代码地址，但function是

```
template<class R, class... Args>
class function {
    Storage storage;   // 保存可调用对象，或者保存对象指针

    R (*invoke)(void*, Args...); // 知道怎么调用
    void (*manager)(Operation, void*, void*); // 复制、移动、销毁
};
std::function
┌────────────────────────┐
│ storage：对象/对象指针   │
├────────────────────────┤
│ invoke：调用函数地址     │
├────────────────────────┤
│ manager：管理函数地址    │
└────────────────────────┘
```

##### 以捕获 Lambda 为例：

```
auto lambda = [base = 10](int a, int b) {
    return base + a + b;
};
```

```
编译器：
struct LambdaType {
    int base;

    int operator()(int a, int b) const {
        return base + a + b;
    }
};
```

function：

```
storage = LambdaType{10}
invoke  = Invoke<LambdaType>的地址
manager = Manager<LambdaType>的地址
其中调用函数类似：
int InvokeLambda(void* storage, int a, int b)
{
    auto* object = static_cast<LambdaType*>(storage);
    return (*object)(a, b);
}
```

std::function
┌──────────────────────────┐
│ storage                   │
│ ┌──────────────────────┐ │
│ │ LambdaType           │ │
│ │ base = 10            │ │
│ └──────────────────────┘ │
├──────────────────────────┤
│ invoke = InvokeLambda     │
├──────────────────────────┤
│ manager = ManagerLambda   │
└──────────────────────────┘

##### 所以funtion底层原理是：对象存储 + 类型专属的调用函数 + 类型专属的管理函数  对象较小时直接放在 storage 中；对象较大时，storage里保存堆对象的指针

##### 类型擦除（1）：std::function并不知道里面一定是 Add，它只知道：storage里有一个东西，invoke知道怎么调用这个东西

#### 问题：

类型擦除会带来间接调用开销，编译器通常难以内联。
较大的捕获对象可能触发堆内存分配；小对象优化是否使用以及容量大小由实现决定。
保存的可调用对象通常必须可复制，因此不能直接保存捕获 std::unique_ptr 的 move-only Lambda。
捕获引用或 this 时容易出现悬空引用：

## union：

union 的所有非静态数据成员共用同一块存储空间。最大成员的 sizeof+尾部填充

### union 一般只有一个活动成员：

```
U u;
u.i = 10;       // 当前活动成员是 i
u.f = 3.14f;    // 改为 f，原来的 i 不再是活动成员
```

### 可以有构造函数，不能有基类，不能作为基类，也不能有虚函数。
