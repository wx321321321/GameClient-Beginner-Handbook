# 1.4 指针与引用

<!-- course-navigation:start -->
[上一节：1.3 操作系统](operating-systems.md) · [本章目录](README.md) · [下一节：1.5 类、继承与多态](classes-inheritance-and-polymorphism.md) · [手册首页](../README.md)
<!-- course-navigation:end -->

这一篇从“怎样访问对象”走到“谁负责对象的生命周期”：先认清指针和引用，再理解值类别、移动和转发，最后看 RAII 与智能指针。地址有效、类型正确、所有权清楚，是不同层面的要求。

先具备变量、函数参数和作用域的基础。第一次读到 `std::forward` 时，只需理解它解决的问题；其中的模板推导可以学完[模板](templates.md)后再回来推演。下面标注的实验都收在 [C++17 语言实验](../examples/course/cpp-language-labs.cpp)中；先预测结果，再运行对应的 `pointer_labs` 部分。

## 1.4.1 指针、引用与 `const`

指针是对象，可以保存地址、空指针值等；引用是绑定到对象或函数的别名。常见问题先分成三种：

- 空指针：明确不指向对象，例如 `nullptr`。
- 未初始化指针：局部指针没有初始化就被使用，口语里常叫“野指针”。野指针不是严格的标准术语。
- 悬空指针/引用：曾绑定的对象生命周期已经结束。删除资源不会自动把所有别名清空。

### 1.4.1.1 先解决一个问题：函数能不能修改调用者的血量

```cpp
void damage_copy(int hp) { hp -= 10; }
void damage_ref(int& hp) { hp -= 10; }
void damage_ptr(int* hp) {
    if (hp != nullptr) { *hp -= 10; }
}

int hp = 100;
damage_copy(hp);       // hp 仍为 100
damage_ref(hp);        // hp 变为 90
damage_ptr(&hp);       // hp 变为 80
damage_ptr(nullptr);   // 本函数约定：没有目标时什么都不做
```

`damage_copy` 的形参是另一个 `int` 对象，修改它不影响调用者。`damage_ref` 的形参绑定原对象；`damage_ptr` 虽然也按值复制了指针，但复制后的地址仍指向原来的 `hp`，所以修改 `*hp` 会影响原对象。最后一行能够成立，是因为我们主动检查并定义了空状态的行为；指针不会自动替我们判空。

再区分“改指针”和“改对象”：

```cpp
int other = 50;
int& alias = hp;
alias = other;         // 给 hp 赋值，hp 变为 50；alias 没有改绑
int* target = &hp;
target = &other;       // 改指向，现在 *target 是 other
*target = 40;          // other 变为 40，hp 仍为 50
```

只需修改一个必定存在的对象时，引用能直接表达这个前提；允许“没有对象”时，指针可以表达空状态。这两个接口都只是借用，均没有获得 `delete` 调用者对象的权利。实验文件的 [parameter_access()](../examples/course/cpp-language-labs.cpp) 会断言这里每一步的数值。

### 1.4.1.2 顶层和底层 `const`

```cpp
int value = 10;
const int* p = &value;         // 底层 const：不能通过 p 修改 value
int* const q = &value;         // 顶层 const：q 不能改指向，但 *q 可改
const int* const r = &value;   // 两者都有
```

`p` 指向非 `const` 对象时，其他合法访问路径仍可修改该对象；如果对象本来就被定义为 `const`，通过强制转换再修改仍是未定义行为。引用不可重新绑定，可用“自动解引用的固定绑定”帮助理解，但**引用在语言上不是顶层 const 指针**。

| 比较 | 指针 | 引用 |
| --- | --- | --- |
| 初始化 | 可声明后再赋值，但使用前必须有有效值 | 通常在定义时绑定；成员需在初始化阶段绑定 |
| 空状态 | 可以是 `nullptr` | 没有合法的“空引用”状态 |
| 改变目标 | 指针可重新赋值 | 赋值是给被引用对象赋值，不是重新绑定 |
| 使用 | `*p`、`p->member` | 通常像原对象一样使用 |
| 取地址 | `&p` 是指针对象的地址 | `&ref` 是被引用对象的地址 |
| `sizeof` | 指针类型的大小 | 被引用类型的大小 |

指针大小由实现和指针种类决定，常见平台对象指针为 4 或 8 字节，但不是语言保证。引用是否需要单独存储没有统一规定：局部别名可能被完全消除，引用参数或引用成员常以指针实现。`sizeof(ref)`、`&ref` 都不能用来观察所谓“引用自己的存储”。[标准草案：引用](https://eel.is/c++draft/dcl.ref)

### 1.4.1.3 `NULL` 与 `nullptr`

`NULL` 是实现定义的空指针常量宏，可能按整数形式展开。遇到整数和指针重载时，`fun(NULL)` 可能选整数重载，也可能产生歧义，不能保证总调用 `int` 版本。C++11 的 `nullptr` 类型为 `std::nullptr_t`，可转换到对象指针、函数指针等空指针类型，能更明确表达意图。

### 1.4.1.4 空指针调用成员函数

通过空指针调用普通非静态成员函数是未定义行为，即使函数体没有读成员，也不能依赖它“碰巧能运行”。常见实现可能把成员函数看成接收 `this` 的函数，这只能解释某次实验结果，不能证明合法。虚调用通常还需读取虚表信息，但也不能简单把它规定为“必然崩溃”。静态成员函数应直接通过类名调用。[标准草案：非静态成员函数](https://eel.is/c++draft/class.mfct.non.static)

## 1.4.2 数组、指针与 `void*`

### 1.4.2.1 指针数组和数组指针

```cpp
int arr[5]{};
int* pointers[5]{};            // 5 个 int* 元素组成的数组
int (*whole)[5] = &arr;        // 指向“5 个 int 的数组”的指针
int* first = arr;              // 指向 arr 的首元素

(*whole)[1] = 7;               // arr[1]
// first + 1 跳过 1 个 int。
// whole + 1 跳过整个 int[5]，此处是尾后位置，不能解引用。
// pointers + 1 跳过 1 个 int*，不是跳过 1 个 int。
```

`whole[0]` 是这个数组，`whole[1]` 会尝试访问下一个数组；对上面的独立 `arr`，不存在可访问的下一个数组。`sizeof(whole)` 是指针大小，`sizeof(*whole)` 是 `5 * sizeof(int)`。

数组在许多表达式中会转换为首元素指针，但 `sizeof(arr)`、`&arr`、绑定数组引用等情况不发生这种退化。函数参数中的 `int a[5]` 会被调整成 `int*`，长度没有随之传入；普通 C++ 函数不能直接按值返回原生数组。

```cpp
int my_array[3]{};
int (&ref)[3] = my_array;       // 合法：数组引用，保留长度
// int& refs[3];               // 错误：不能声明引用的数组
```

数组引用可以写固定长度，也可以在模板里推导长度；不是必须用模板。引用数组被语言禁止，不应仅用“引用可能不占空间”替代这条类型规则。

### 1.4.2.2 `void*` 表示无具体对象类型的地址接口

对象指针可隐式转换成相应 cv 限定的 `void*`。转回具体类型前，要确认实际对象、对齐和生命周期匹配；不能直接解引用 `void*`，标准 C++ 也不允许对它做 `++` 等指针算术。函数指针与 `void*` 的互转不属于同样的通用对象指针保证。

普通指针本身一般不另外储存一份运行时类型标签：指向类型主要是编译器掌握的信息，它影响解引用、指针算术和类型检查。

### 1.4.2.3 用 `optional` 表示“可能没有值”

当结果不需要表达地址和所有权，只需表达存在或缺失时，C++17 的 `std::optional<T>` 比使用特殊数值或伪造空指针更清楚。

```cpp
#include <optional>
std::optional<int> result;
result = 42;
if (result.has_value()) {
    int value = *result;
}
```

可把它理解为“对象存储 + 是否有值的标记”，具体布局由实现决定，值存在时直接保存在 optional 内部。`operator*` 是重载运算符，访问前需保证有值；`value()` 在无值时抛 `std::bad_optional_access`。

## 1.4.3 值类别：给表达式分类，不给对象贴永久标签

C++ 的表达式有左值 lvalue、将亡值 xvalue、纯右值 prvalue；后两者合称右值。左值/将亡值强调对象身份，纯右值通常表示计算结果或初始化所需的值。

```cpp
int a = 5;
// a              左值
// 10、a + 1      纯右值
// std::move(a)   将亡值
```

“有名字就是左值、没名字就是右值”只能帮助记住普通变量表达式，不能当成定义：返回 `T&` 的函数调用也可以是左值，返回 `T&&` 的调用可以是将亡值。能否直接写 `&expr` 也不是完整判据，例如位域存在特殊规则。

对内建整数，`++i` 修改 `i` 后返回其左值，`i++` 返回修改前的值，是纯右值。例如 `i = 5` 时，前者表达式值是 6，后者表达式值是 5。后置形式的旧值语义不意味着优化后必须产生一个实际存储的副本；用户自定义运算符的返回类型也可以不同。

### 1.4.3.1 引用绑定的常用规则

| 类型 | 常见绑定对象 |
| --- | --- |
| `T&`（T 非 const） | 非 const 左值 |
| `const T&` | 同类型兼容的左值和右值 |
| `T&&`（普通右值引用） | 同类型兼容的右值 |

引用直接绑定临时对象时，某些场景可以延长临时对象生命周期，但有函数参数、返回引用等例外，不能靠把引用再传一层无限延寿。具名的右值引用变量在表达式里仍是左值：

```cpp
int&& value = 10;
const int& view = value;        // value 表达式是左值，可绑定只读引用
```

## 1.4.4 `std::move`：允许移动，不执行移动

`std::move(x)` 本质上是转换成相应右值引用类型的表达式，让后续重载决议有机会选择移动构造/移动赋值。它本身不复制、不清空，也不释放资源。[标准草案：move/forward 辅助函数](https://eel.is/c++draft/forward)

### 1.4.4.1 拷贝和移动各自负责什么

对于持有独占资源的类，典型深拷贝会分配新资源并复制内容；典型移动会转移资源句柄，并把源对象调整为可析构状态，例如把源裸指针置空。这只是常见资源管理实现，不是所有移动都必须偷指针，也不是遇到指针就必须深拷贝：观察型指针本来就可能需要共享一个目标。

编译器生成的拷贝构造按成员拷贝：裸指针成员复制地址，`std::string`、`std::vector` 等成员按各自的拷贝语义处理。所以“默认拷贝永远只复制地址”不成立。

独占裸指针若被直接浅拷贝，两份对象可能重复释放同一资源。移动语义在源对象不再需要原资源时转交所有权，可以避免无用的深拷贝。对整数等平凡类型，移动通常没有比拷贝更多的收益。

移动之后，标准库类型一般保证有效但状态未指定（具体类型可能有更强保证），可析构、可赋值，使用其他操作前要满足其前置条件。自定义类型的移动后状态由该类型契约决定。对象不会“永久变成将亡值”：`std::move(b)` 是将亡值表达式，再写 `b` 仍是左值表达式。

### 1.4.4.2 `const` 和具名右值引用

```cpp
#include <string>
#include <utility>

void consume(std::string&& value) {
    std::string local = std::move(value); // value 本身表达式是左值
}
```

是否再 `move` 取决于这里是否确实要消耗参数；不是看到 `&&` 就无条件移动。模板转发场景则用 `std::forward`。

`std::move(const T对象)` 得到 `const T&&`，不会去掉 const。通常的 `T(T&&)` 无法接受它，而 `T(const T&)` 可以，所以常见结果仍是拷贝；若类型另有接受 `const T&&` 的重载，还会按正常规则选择。

### 1.4.4.3 返回局部对象与拷贝消除

```cpp
std::string make_name() {
    std::string name = "Ninja";
    return name;
}
```

`return name;` 可以使用 NRVO，直接在结果位置构造；NRVO 不是所有情况下都强制发生，未发生时还可能使用隐式移动。C++17 对特定纯右值返回另有保证的拷贝消除规则。

写成 `return std::move(name);` 通常会破坏 NRVO 条件而多出一次移动，并不会把按值返回函数变成返回引用的函数。不要为了“优化”机械加上 `std::move`。[标准草案：拷贝/移动消除](https://eel.is/c++draft/class.copy.elision)

### 1.4.4.4 `std::move_if_noexcept`

当移动不会抛异常，或类型根本不可拷贝时，它返回可用于移动的右值引用；否则返回 `const T&`，允许调用方选择拷贝。`vector` 搬迁元素时常用类似策略保护异常保证。

若类型只有可能抛异常的移动构造，容器无法凭空退化到不存在的拷贝构造，其异常保证可能弱化。详见[vector 扩容与异常](stl-and-algorithms.md#19114-扩容为什么关心-noexcept)。

## 1.4.5 `std::forward`：保留传入时的值类别

```cpp
#include <utility>

template<class T>
void relay(T&& arg) {
    target(std::forward<T>(arg)); // target 由使用场景提供相应重载
}
```

`arg` 在函数体里总是具名左值表达式。直接 `target(arg)` 会把它当左值；无条件 `std::move(arg)` 又会连调用方的左值也当成可消耗对象。`std::forward<T>` 利用推导得到的 `T` 保留调用方的意图。

这种 `T&&` 称为**转发引用**（也常叫万能引用），条件是 `T` 为需要推导的、无 cv 限定的模板类型参数。传入左值 `X` 时，`T` 推导为 `X&`；传入右值时，通常推导为 `X`。折叠规则是：

```text
&  + &  → &
&  + && → &
&& + &  → &
&& + && → &&
```

`auto&&` 在合适的推导场景也有同类效果，但从花括号初始化列表等情形有例外。`const T&&`、`std::vector<T>&&` 不是上述转发引用；类模板里已经固定的 `T` 写成 `T&&`，也不因此成为该函数的转发引用。类型推导完整例子见[模板](templates.md)。

## 1.4.6 RAII：把资源交给对象的生命周期

RAII 用对象的构造/析构配对资源获取和释放。资源不只包括动态内存，也包括文件句柄和锁；管理对象也不必一定在栈上，可以是成员或其他具有明确生命周期的对象。

手写 `new` 后在函数末尾 `delete`，遇到中途 `return` 或异常就可能漏掉释放。正常离开作用域或异常栈展开时，已成功构造的局部对象会按逆序析构，于是资源清理可以随控制流自动执行。这不是进程被强制终止等情况下“100% 清理”的承诺。[标准草案：异常栈展开](https://eel.is/c++draft/except.ctor)

### 1.4.6.1 锁也是 RAII 资源

`std::lock_guard` 构造时取得锁、析构时释放，不能复制或移动，适合覆盖整个作用域的简单临界区。

`std::unique_lock` 额外记录是否持锁，支持移动、手动 `lock()` / `unlock()`、`std::defer_lock` 延迟加锁等。`try_lock_for`、`try_lock_until` 还要求底层互斥量支持定时操作，不是任意 `std::mutex` 都能使用。

两者减少漏解锁问题，但不能自动消除锁顺序冲突等死锁。不能只按“谁性能最高”选择，而应看是否需要中途解锁、条件变量或所有权转移。

## 1.4.7 智能指针：先选择所有权，再选择接口

### 1.4.7.1 `unique_ptr`：独占所有权

默认删除器配对 `new` / `delete`，数组形式配对 `new[]` / `delete[]`；也可通过自定义删除器管理适合的其他资源。它不是只能管理一种堆内存。

```cpp
auto p = std::make_unique<int>(10); // C++14
// auto copy = p;                  // 不允许复制
auto owner = std::move(p);          // 转移所有权，p 变空
```

这里可以把“表达式转换”和“真正接管”拆成两步观察：

```cpp
auto first_owner = std::make_unique<int>(10);
auto&& movable = std::move(first_owner);
// 仅绑定一个引用，没有构造新的 unique_ptr，first_owner 仍拥有整数。
auto second_owner = std::move(movable);
// 此处才调用 unique_ptr 的移动构造：first_owner 为空，*second_owner == 10。
```

`movable` 是原 `unique_ptr` 的别名，它有名字，在第二行之后写 `movable` 仍是左值表达式。最后一行再次使用 `std::move`，才允许构造函数接管资源。`unique_ptr` 的移动后为空是该类型的明确保证，不能据此推断移动后的任意 `string` 都必然为空。对照运行 [unique_transfer()](../examples/course/cpp-language-labs.cpp)；不要解引用已经为空的 `first_owner`。

`release()` 返回裸指针并放弃所有权，不释放对象，调用方必须接好责任。`reset(new_ptr)` 替换所管理的指针并释放旧资源；不要把当前同一个裸指针又传给自身 `reset`。默认构造/空状态析构是安全的。

### 1.4.7.2 `shared_ptr`：多个拥有者共享一个控制块

控制块通常记录强引用计数、弱引用计数、删除器，以及需要的分配器信息。

```text
强引用归零 → 销毁被管理对象 / 调用删除器
对象已销毁且不再有弱引用需要控制块 → 销毁控制块
```

实现可能把“存在任意强引用”合并记成控制块的一份内部弱计数，例如存成“显式弱引用数 + 强引用是否非零”。这是记账优化，不能把内部字段值直接等同于用户创建的 `weak_ptr` 数量。

```cpp
#include <memory>

auto p1 = std::make_shared<int>(10);
auto p2 = p1;                    // 共享控制块
int* raw = new int(10);
std::shared_ptr<int> p3(raw);      // 从此由 p3 的所有权组管理
```

从 `new T` 的裸指针构造 `shared_ptr`，常见实现另分配控制块；`make_shared<T>(args...)` 通常把对象和控制块放进一次分配中。这里传给 `make_shared` 的是 **T 的构造参数**，不是把已有裸指针交给它接管。

合并分配也有代价：强引用归零时 `T` 立即析构，但若仍有弱引用，控制块需保留，和它合在一起的那块对象存储也可能暂时无法归还。大型对象配合长期弱引用时要考虑这一点。[标准草案：shared_ptr](https://eel.is/c++draft/util.smartptr.shared)

### 1.4.7.3 控制块安全，不等于一切都线程安全

不同 `shared_ptr` 实例共享同一控制块时，各自复制、销毁等计数操作有相应线程安全保证。多个线程对**同一个** `shared_ptr` 变量进行包含写入的并发访问，需要额外同步，C++20 可用 `std::atomic<std::shared_ptr<T>>` 等方案。

对象 `*p` 的成员访问也不会自动加锁。智能指针解决所有权，不替代对象自身的线程安全；从 `get()` 借出的裸指针若活得比拥有者久，仍会悬空。

### 1.4.7.4 `weak_ptr`：观察，不延长对象生命

可由 `shared_ptr` 或另一个 `weak_ptr` 构造/赋值，也可默认构造为空；不能直接用裸指针建立共享所有权关系。

```cpp
std::weak_ptr<int> weak = p1;
if (auto alive = weak.lock()) {
    // alive 在这个作用域内保持对象存活
    int value = *alive;
}
```

`expired()` 表示当前强引用计数是否为 0，但“先检查未过期、稍后再用”可能有竞态。`lock()` 原子地尝试取得强引用：成功返回共享控制块的 `shared_ptr`，失败返回空指针，**不抛 `std::bad_weak_ptr`**。

显式 `std::shared_ptr<T> p(weak);` 在弱引用已过期时才会抛 `std::bad_weak_ptr`。这与 weak 对象自身是否已经销毁不是同一概念。[标准草案：weak_ptr 观察接口](https://eel.is/c++draft/util.smartptr.weak.obs)

把观察者留在外层，就能看见生命周期的分界线：

```cpp
std::weak_ptr<int> observer;
{
    auto owner = std::make_shared<int>(42); // 强拥有者 1 个
    observer = owner;                     // 仍是 1 个，weak 不拥有
    {
        auto hold = observer.lock();      // 成功，强拥有者暂时变为 2 个
        // *hold == 42
    }                                     // hold 销毁，回到 1 个
}                                         // owner 销毁，整数生命周期结束
auto missing = observer.lock();           // 得到空 shared_ptr
```

按花括号推演就能发现：对象能否存活，取决于强拥有者是否还在，而不是 `observer` 这个变量还在不在。实验文件的 [weak_lifetime()](../examples/course/cpp-language-labs.cpp) 验证这条时间线；计数只用于这里的单线程观察，不要把 `use_count()` 当成并发同步条件。

### 1.4.7.5 循环引用为什么不释放

```cpp
#include <cstdio>
#include <memory>

struct B;
struct A {
    std::shared_ptr<B> b;
    ~A() { std::puts("~A"); }
};
struct B {
    std::shared_ptr<A> a;
    ~B() { std::puts("~B"); }
};

auto a = std::make_shared<A>();   // A 强引用 1
auto b = std::make_shared<B>();   // B 强引用 1
a->b = b;                        // B 强引用 2
b->a = a;                        // A 强引用 2
// 外部 a、b 销毁后，两者都仍有 1 个强引用。
```

引用计数只统计强拥有者数量，不判断对象是否还能从外部到达。若 `B::a` 是回指关系，可把它改为 `std::weak_ptr<A>`，用 `lock()` 临时访问；需要从设计上确定哪条边拥有、哪条边观察。

### 1.4.7.6 不要用已有对象的 `this` 重建所有权

若对象已经被 `shared_ptr` 管理，再写 `std::shared_ptr<Player>(this)` 通常会建立第二个控制块，最后重复删除。应共享原控制块：

```cpp
class Player : public std::enable_shared_from_this<Player> {
public:
    std::shared_ptr<Player> keep_alive() { return shared_from_this(); }
};
auto player = std::make_shared<Player>();
auto same_owner = player->keep_alive();
```

`enable_shared_from_this` 概念上维护一个指向原控制块的弱引用，由合适的 `shared_ptr` 构造过程关联。必须公开、无歧义地继承，并在对象已经进入共享所有权后调用 `shared_from_this()`；在构造函数里调用通常还没有完成关联，会失败。C++17 的 `weak_from_this()` 可取得对应弱引用。

这条规则适用于从裸指针构造独立 `shared_ptr`，并非“make_shared 看见裸指针就接管”的规则。[标准草案：enable_shared_from_this](https://eel.is/c++draft/util.smartptr.enab)

### 1.4.7.7 裸指针可以借用，但不能冒充拥有者

智能指针管理资源时，普通函数参数、C API 或非拥有型查询仍可使用裸指针/引用，前提是约定清楚：不擅自 `delete`，不在拥有者结束后继续访问。

管理器返回对象时，若调用方只借用且能保证管理器和对象活得够久，可以返回指针/引用；若异步任务确实需要延长生命，就应返回或传递共享所有权。不能仅凭“是单例管理器”就断言永远不应返回 `shared_ptr`。

游戏对象还可用“索引/句柄 + 版本号”：查询时同时检查槽位和代次，避免槽位复用后把旧句柄误认为新对象。但它不会自动解决查询后并发销毁的问题。只需要可选值时则用 `optional`，避免引入不必要的共享对象。

## 1.4.8 手写练习时检查哪些点

实现练习只用于理解机制，生产代码优先使用标准库；除了几个计数，还涉及异常、删除器、数组和并发等细节。

| 类型 | 必须覆盖的行为 |
| --- | --- |
| `unique_ptr` | 禁止复制；移动接管且源为空；析构调用删除器；`release` 不删除；`reset` 替换并删除旧资源；`*` / `->` |
| `shared_ptr` | 复制/移动/自赋值；强计数归零释放对象；适时释放控制块；`reset`；`*` / `->`；没有 `release` |
| `weak_ptr` | 增减弱引用；不直接销毁仍有拥有者的对象；适时释放控制块；`reset`；`lock` 原子尝试增加非零强计数；没有 `release`、`*` 或 `->` |

尤其不要把 `weak_ptr::lock` 简化成“读到 strong > 0 就普通加一”：并发环境中检查和取得所有权必须成为不可分割的正确操作，不能复活已经析构的对象。

再用三个小问题检查是否真正分清了访问与所有权：

1. 把 `damage_ptr` 的函数体改成 `hp = nullptr;`，调用者的指针会变空吗？不会，形参指针本身是按值复制；若要修改调用者的指针变量，需要返回新指针或使用指针引用等接口。
2. 在 [weak_lifetime()](../examples/course/cpp-language-labs.cpp) 中把 `hold` 移到最外层，整数会在原来的位置销毁吗？不会，成功取得的 `shared_ptr` 本身就是新的强拥有者。
3. `std::move` 后还能访问源对象吗？先看具体类型的移动后契约。实验中的 `unique_ptr` 可以检查是否为空、重新赋值，但不能解引用空指针。
