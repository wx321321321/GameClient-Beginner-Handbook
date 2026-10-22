# 模板

模板把类型、编译期值或行为参数化，让编译器针对具体参数形成相应的类或函数。读模板代码时，最容易混淆的是三个阶段：定义模板时能检查什么、调用时怎样推导和选择候选、选中后函数体怎样实例化。

## 先分清重载、隐藏和覆盖

重载是在编译期从同名候选中选择最合适的函数。按值参数的顶层 `const` 不参与区分函数类型：

```cpp
void fun(int);
void fun(const int);           // 同一个函数，不是两个重载
```

指向对象或被引用对象的 `const` 则会影响类型：

```cpp
void fun(int&);
void fun(const int&);          // 可以重载

void use(int*);
void use(const int*);          // 可以重载
```

`int&` 接受非 const 左值；`const int&` 还可接受 const 左值和右值。对非 const 左值，两者都可行，但 `int&` 通常是更合适的匹配。`const int&` 依然是**左值引用类型**，不能因为能接临时对象就把它称为右值引用。

派生类声明同名函数可能隐藏基类的同名重载集，但“同名就覆盖”是错误的。覆盖要求基类函数是虚函数，且派生声明满足签名等规则；用 `override` 检查。完整区分见[类、继承与多态](classes-inheritance-and-polymorphism.md#重载隐藏覆盖要分开记)。

函数模板和普通函数一起参与重载决议时，先比较匹配和其他规则。如果匹配同样好，通常优先普通函数；更匹配的模板仍可以胜出。

```cpp
void choose(int);
template<class T> void choose(T);

// choose(1)   选择普通函数：两者都精确匹配。
// choose(1.5) 选择模板：T=double 精确匹配，普通函数需要转换。
```

## 模板定义、实例化与文件组织

### 定义不是立即为所有类型生成代码

```cpp
template<class T>
T add(T a, T b) {
    return a + b;
}
```

定义时描述的是一族函数。使用 `add(1, 2)` 时可以推导 `T = int`，在需要时实例化对应特化。类模板也类似，但实例化类通常不等于立即实例化它的每个成员函数体；很多成员仍在被需要时才实例化。

模板未使用，不表示内部可以任意写错。定义期会检查语法、非依赖名字等；依赖具体参数的检查常延迟到实例化期。例如 `value.begin()` 是否存在，需要知道 `value` 的类型。某些无法产生任何合法特化的错误还有无需诊断的规则，不能依赖“不调用就一定不报错”。

### 为什么实现常放头文件或 `.inl`

一般的隐式实例化需要在实例化处看到定义，因此常见写法是把声明和定义都放头文件，或者：

```cpp
// MyStack.h
#pragma once
template<class T>
class MyStack {
public:
    void push(const T& value);
};
#include "MyStack.inl"
```

```cpp
// MyStack.inl
// 示意：真实栈还需要相应的存储成员。
template<class T>
void MyStack<T>::push(const T& value) {
    // 使用 value 的实现
}
```

`.inl` 只是文件命名习惯，不是语言要求；关键是它被包含后，定义在编译器需要的位置可见。模板声明和定义可以分开写，但不能只把未知类型的定义藏在普通 `.cpp` 中，仍期待其他文件自动实例化任意类型。

### 隐式实例化与重复编译成本

`CrazySkill(myTarget)` 这类调用，在满足声明和定义可见等条件时，可推导参数并隐式实例化。多个翻译单元使用同一个特化，可能重复执行模板分析和代码生成；工具链常通过链接合并等机制处理重复实体。不能保证每次都生成一份机器码，也不能忽视头文件解析成本。

### 显式实例化：限定支持的类型

如果只支持固定的一组类型，可以在一个 `.cpp` 提供定义和显式实例化：

```cpp
// Skill.h
#pragma once
template<class T>
void CrazySkill(T value);
extern template void CrazySkill<int>(int); // 显式实例化声明
```

```cpp
// Skill.cpp
#include "Skill.h"
template<class T>
void CrazySkill(T value) {
    // 实际实现
}
template void CrazySkill<int>(int);        // 显式实例化定义
```

其他文件调用 `CrazySkill(10)`，链接到这里生成的实现。`extern template` 的意图是避免调用处再隐式实例化指定实体；它本身不生成代码，也不能替代链接进真正的定义。调用未提供实例化的类型，如 `double`，仍可能链接失败。

类模板可用 `extern template class MyStack<int>;` 配对一处 `template class MyStack<int>;`，不过使用者仍需看到类定义以知道对象布局和成员声明。这个办法可能减少重复实例化工作，并不是所有 LNK2019 的通用修复。[标准草案：显式实例化](https://eel.is/c++draft/temp.explicit)

### 非类型模板参数

```cpp
#include <array>
#include <cstddef>

template<class T, std::size_t N>
struct Buffer {
    std::array<T, N> data;
};
constexpr std::size_t count = 8;
Buffer<int, count> buffer;
```

这里 `N` 是编译期值的一部分，不同的 `N` 形成不同类型。普通运行时变量不能任意填入要求常量表达式的位置，但符合要求的 `constexpr` 变量可以。除整数外，允许的参数还包括某些指针、引用以及 C++20 起符合条件的结构类型等；具体限制依参数类型和版本决定。编译期已知有利于优化，但不是无条件“性能极致”的保证。

## 模板特化与依赖名字

### 全特化和偏特化

```cpp
template<class T, class U> struct PairInfo { /* 通用实现 */ };
template<> struct PairInfo<int, float> { /* 全特化 */ };
template<class T> struct PairInfo<T, T*> { /* 偏特化 */ };
```

全特化给出确定的全部实参，用 `template<>` 声明特殊版本。偏特化仍保留参数，只针对某类形状，如指针、两个相同类型或部分固定参数。

类模板支持偏特化，C++14 的变量模板也支持；函数模板不能偏特化，通常用函数重载或约束表达类似需求。函数模板可以全特化，但仍需注意它与重载选择的关系。[标准草案：偏特化](https://eel.is/c++draft/temp.spec.partial)

原笔记用 `vector<bool>` 说明“为特定类型换实现”，这个直觉可保留。严格看，标准库对 `vector<bool, Allocator>` 保留了分配器参数，是针对 `bool` 的部分特化，不是程序员应自行重定义的 `template<> class vector<bool>`。位压缩、代理引用等行为见[STL](stl-and-algorithms.md#vectorbool为什么取不到-bool)。

### `typename`：告诉解析器这是一个类型

```cpp
template<class T>
void inspect(const T& container) {
    typename T::const_iterator it = container.begin();
    // 使用 it
}
```

`T::const_iterator` 依赖模板参数。解析器不能仅凭名字知道它是嵌套类型还是静态成员，所以在需要的上下文中用 `typename` 消除歧义。不是把 `T::` 误认为命名空间，而是尚不知道这个依赖名字的种类。

也不是所有依赖名字都必须写 `typename`：已知为类型的名字、某些类型上下文以及新标准放宽的情形有例外。依赖成员模板还可能需要 `template` 消歧义符，两者负责不同问题。[标准草案：依赖类型](https://eel.is/c++draft/temp.res)

## 从调用到函数体：三个检查环节

1. **推导与替换**：根据实参求出模板参数，形成候选；不满足约束或发生允许忽略的替换失败者被移除。
2. **重载决议**：从可行候选中选唯一的最佳匹配，可能无匹配，也可能二义性。
3. **需要时实例化函数体**：检查选中实现依赖具体参数的内容，例如对 `int` 调 `.begin()` 将失败。

若两个候选同样好，就算它们函数体想处理不同类型，编译器也不会先运行函数体帮你挑选。需要通过不同参数形状、`enable_if` 或 `requires` 把条件放进参与选择的规则里。

不过，若两个模板声明其实等价，它们可能是重复声明/重复定义，而不是“两名候选导致二义性”。不要把所有同名模板报错都归为 ambiguous call。

## 类型推导：先求 `T`，再还原完整形参类型

### 函数模板的三类常见形参

| 形参模式 | 常用推导规律 |
| --- | --- |
| `T` 按值 | 不保留实参的引用和顶层 const；数组/函数通常转换为指针 |
| `T&`、`T*` | 按被引用/指向类型推导，保留相应 const；引用形参可保留数组类型 |
| 推导中的 `T&&` | 左值传入时 T 推导为左值引用；右值传入时 T 通常为非引用类型，再引用折叠 |

```cpp
template<class T> void by_value(T);
template<class T> void by_ref(T&);
template<class T> void forward_ref(T&&);

const int x = 10;
// by_value(x)    T=int，形参 int
// by_ref(x)      T=const int，形参 const int&
// forward_ref(x) T=const int&，形参折叠为 const int&
// forward_ref(1) T=int，形参 int&&
```

推导不是先任意转换实参再寻找一个共同类型。例如 `add(1, 2.0)` 对同一个 `T` 给出 `int`、`double` 两种冲突结果，会推导失败；显式写 `add<double>(1, 2.0)` 后才按确定的形参类型检查转换。标准还规定数组转换、资格调整等特定规则，因此“推导完全没有任何调整”也过于绝对。[标准草案：函数调用推导](https://eel.is/c++draft/temp.deduct.call)

### `auto` 与花括号的特殊情况

```cpp
const int x = 10;
const int& ref = x;
const int* const ptr = &x;

auto a = ref;                  // int

auto& b = ref;                 // const int&
auto c = ptr;                  // const int*
auto* d = ptr;                 // const int*
```

`auto` 的常见规则与模板参数推导相似，但并非所有语法都一样：

```cpp
auto values = {1, 2, 3};       // std::initializer_list<int>
auto one{1};                  // int（现行规则）
// auto bad{1, 2};             // 错误：直接列表 auto 不能这样推导
```

普通 `template<class T> void f(T);` 不能直接从 `f({1, 2, 3})` 推导 T；若形参明确是 `std::initializer_list<T>`，则可以从元素推导。这与“所有函数模板都不能推导列表”不同。

## SFINAE、`enable_if` 与 `requires`

SFINAE 是“替换失败不是错误”：在模板候选替换的规定上下文中，某些无效类型/表达式会导致该候选被丢弃，而不是立即让整个程序失败。

边界是**直接上下文（immediate context）**，不能简单总结为“签名里的任何语法错误都忽略”。模板定义本身的语法错误、替换触发其他实体实例化产生的错误、已选中函数体里的错误，都不保证被 SFINAE 吞掉。[标准草案：模板实参推导](https://eel.is/c++draft/temp.deduct)

C++17 写法示例：

```cpp
#include <type_traits>

template<class T, std::enable_if_t<std::is_integral_v<T>, int> = 0>
void print(T value) {
    // 整数处理
}
template<class T, std::enable_if_t<std::is_floating_point_v<T>, int> = 0>
void print(T value) {
    // 浮点处理
}
```

条件为 false 时，相应 `enable_if_t` 无有效类型，使候选退出。复杂嵌套可能导致诊断很长，也容易把约束与返回类型混在一起。

C++20 可以直接表达约束：

```cpp
#include <concepts>

template<std::integral T>
void print(T value) { /* 整数处理 */ }

template<class T>
    requires std::floating_point<T>
void print(T value) { /* 浮点处理 */ }
```

`requires` / Concepts 能把接口要求写得清楚，并参与候选筛选、排序，通常也改善诊断。不能保证它“不生成中间结构”“瞬间算完”或在所有项目中编译更快；约束本身也可能很复杂。[标准草案：约束](https://eel.is/c++draft/temp.constr)

## `decltype`：保留表达式的类型信息

按这个顺序判断：

1. 无括号的名字或成员访问，如 `x`、`obj.member`：通常得到该实体声明的类型。
2. 其他表达式：若是左值，得到 `T&`；将亡值得到 `T&&`；纯右值得到 `T`。

```cpp
int x = 10;
decltype(x) value = x;         // int：名字特殊规则

decltype((x)) alias = x;       // int&：(x) 按左值表达式判断
```

函数调用表达式也遵守值类别规则，结果与它的声明返回类型相对应：返回 `T&` 的调用得到 `T&`，返回 `T&&` 的调用得到 `T&&`，返回 T 的调用得到 T。`decltype` 不实际执行该函数。[标准草案：decltype](https://eel.is/c++draft/dcl.type.decltype)

### `decltype(auto)`：按 `decltype` 规则推导

```cpp
int x = 10;
int& get_ref() { return x; }

auto a = get_ref();            // int：复制值

decltype(get_ref()) b = get_ref(); // int&
decltype(auto) z = get_ref();   // int&，保留引用
```

它常用于精确保留返回类型，转发函数里尤其有用，但也更容易意外返回引用。例如返回局部变量的 `(local)` 会按左值推导为引用，离开函数后悬空。不要把“保留得更多”自动等同于更安全。

## CRTP：奇异递归模板

派生类把自己作为模板参数交给基类，基类利用已知类型调用派生实现：

```cpp
template<class Derived>
class Base {
protected:
    Base() = default;          // 避免直接创建不对应 Derived 的 Base 对象
public:
    void interface() {
        static_cast<Derived*>(this)->implementation();
    }
};

class Player : public Base<Player> {
public:
    void implementation() {
        // Player 自己的实现
    }
};
```

实例化 `Base<Player>` 后，调用目标在编译期已知。这里的 `static_cast` 仍要求对象确实处于相应 `Player` 的基类子对象中；CRTP 使用方式错误也可能违反此前提。

与虚函数动态多态相比，这种模式自身不需要虚表，常能避免间接调用并更容易内联。不过 `Base<Player>` 与 `Base<Enemy>` 是不同类型，不能自然装进同一种 `Base*` 容器；类型需要编译期确定，也可能增加模板实例化和代码体积。是否采用，应根据是否需要运行时异构集合和接口替换决定。
