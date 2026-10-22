# 1.12 设计模式

<!-- course-navigation:start -->
[上一节：1.11 计算机网络：从消息发送到游戏协议选择](computer-networks.md) · [本章目录](README.md) · [下一节：1.13 Lua：语言规则、游戏脚本与运行时](lua.md) · [手册首页](../README.md)
<!-- course-navigation:end -->

这里关注四种常见需求：谁提供全局服务、谁负责创建对象、怎样替换算法、怎样通知多个监听者。模式的价值在于把变化放到合适的位置；它们本身不会自动解决线程安全和对象生命周期。

先学过类、虚函数、智能指针、Lambda 和容器，再把这些机制组合起来。每节先从具体需求出发，运行后再讨论代价；不用为了“用了模式”而增加层次。[C++17 语言实验](../examples/course/cpp-language-labs.cpp)中的 `pattern_labs` 包含四组调用和断言。

## 1.12.1 单例：一份身份，一个访问入口

单例用于表达进程范围内唯一的服务，例如日志、配置管理或全局资源入口。先确定确实需要唯一实例，再决定用它；“访问方便”本身不足以证明应该成为单例。

```cpp
class Singleton {
    Singleton() = default;
    ~Singleton() = default;
public:
    Singleton(const Singleton&) = delete;
    Singleton& operator=(const Singleton&) = delete;
    Singleton(Singleton&&) = delete;
    Singleton& operator=(Singleton&&) = delete;

    static Singleton& get() {
        static Singleton instance;
        return instance;
    }
};
```

这个 Meyers Singleton 使用局部静态对象：首次需要时初始化，之后各调用共享同一对象；静态成员函数没有隐式 `this`，所以不必先有实例才能获取实例。

```cpp
Singleton& first = Singleton::get();
Singleton& second = Singleton::get();
bool same_object = &first == &second; // true
// auto copy = Singleton::get();     // 错误：auto 按值推导会尝试复制
```

第一次调用得到局部静态对象的引用，第二次得到同一对象的引用，所以比较地址为真。必须保留引用语义；`auto&` 可以，单独的 `auto` 会创建新对象并触发已删除的拷贝构造。[singleton_identity()](../examples/course/cpp-language-labs.cpp) 验证唯一身份，这个测试本身不证明任意单例业务操作具有线程安全性。

删除复制和移动是为了保持唯一身份。移动操作只是用源对象构造/赋值另一个对象，不会自动销毁源对象，不能靠“资源转移了”证明仍只有一个实例。示例把拷贝参数写为 `const Singleton&`，并补齐类定义末尾分号，可直接编译。

C++11 起局部静态对象的初始化具有线程安全保证；初始化后成员操作是否安全取决于服务本身。跨静态对象的析构顺序也要考虑：程序结束时，别的静态对象不能继续使用已经销毁的单例。[标准草案：局部静态初始化](https://eel.is/c++draft/stmt.dcl)

它的收益是统一身份、延迟初始化和访问方便，适合生命周期接近整个程序的基础设施。代价则是全局可变状态和隐藏依赖：从函数签名看不出它用了哪些服务；测试时不容易替换 Mock，还可能互相污染。若需要多实例、独立测试配置或清晰的依赖，可以把服务对象显式传入调用方。

## 1.12.2 工厂：把创建过程集中起来

调用方需要一种对象或接口，但不希望知道具体类型的构造细节时，可以把选择、创建和返回的规则放到工厂。

![工厂模式](assets/factory-pattern.png)

几种常见形式要区分：简单工厂常以 `switch` 选择具体类型；工厂方法把创建步骤留给派生类；抽象工厂提供一组配套对象的创建接口；注册式工厂把类型标识映射到创建器。

工厂可降低调用方对具体类型的依赖，统一参数验证、错误处理和所有权返回方式。注册式工厂新增类型时，常能保持已有调用代码不变，但仍需安排注册过程和重复标识处理。

例如装备栏只想获得一件武器并调用攻击接口，不想在各处写 `new Sword` 或 `new Bow`：

```cpp
struct Weapon {
    virtual ~Weapon() = default;
    virtual int damage() const = 0;
};
struct Sword : Weapon { int damage() const override { return 10; } };
struct Bow : Weapon { int damage() const override { return 6; } };
enum class WeaponKind { sword, bow };

std::unique_ptr<Weapon> make_weapon(WeaponKind kind) {
    switch (kind) {
    case WeaponKind::sword: return std::make_unique<Sword>();
    case WeaponKind::bow: return std::make_unique<Bow>();
    }
    throw std::invalid_argument("unknown weapon kind");
}

auto weapon = make_weapon(WeaponKind::sword);
int damage = weapon->damage(); // 10
```

需要 `<memory>` 和 `<stdexcept>`。`make_weapon` 先选择具体类型，再把独占所有权作为 `unique_ptr<Weapon>` 交给调用者；调用者通过虚函数使用它，离开作用域时通过虚析构正确销毁。将参数换成 `bow`，调用代码不变而伤害变为 6。这里是简单工厂，新增一种武器仍需修改 `switch`；不要把它称为完整的抽象工厂体系。运行 [factory_selection()](../examples/course/cpp-language-labs.cpp) 检查两种选择。

它也有成本。巨大 `switch` 只是把分支集中到一个位置，新增类型仍要修改工厂；复杂的工厂层次会增加接口和类的数量；间接创建会让定位实际构造点更费力。工厂也不自动管理创建后的生命周期，返回 `unique_ptr`、`shared_ptr`、对象池句柄还是借用指针，需要接口自己明确。

## 1.12.3 策略：让同一流程使用不同算法

购物结账流程不变，价格计算可以有多种规则：普通用户原价、VIP 八折、促销减 30 元。策略模式把这部分算法独立出来，结账对象持有当前策略，并在计算时调用它。

下面用 `std::function` 表达策略，和使用抽象 `Strategy` 接口的思路相同：

```cpp
#include <algorithm>
#include <functional>
#include <stdexcept>
#include <utility>

class Checkout {
    std::function<double(double)> strategy_ = [](double price) {
        return price;
    };
public:
    void set_strategy(std::function<double(double)> strategy) {
        if (!strategy) {
            throw std::invalid_argument("empty pricing strategy");
        }
        strategy_ = std::move(strategy);
    }
    double total(double price) const { return strategy_(price); }
};

// 使用示例：金额计算仅为模式演示，不处理货币精度。
// Checkout checkout;
// checkout.set_strategy([](double p) { return p * 0.8; });
// checkout.set_strategy([](double p) { return std::max(0.0, p - 30.0); });
```

把原来的两种优惠按顺序装入同一个结账对象，观察每次“当前策略”的含义：

```cpp
Checkout checkout;
double normal = checkout.total(100.0); // 100
checkout.set_strategy([](double p) { return p * 0.8; });
double vip = checkout.total(100.0);    // 80
checkout.set_strategy([](double p) { return std::max(0.0, p - 30.0); });
double sale = checkout.total(100.0);   // 70
double small = checkout.total(20.0);   // 0，下限保护
```

第二次 `set_strategy` 替换了第一次的策略，没有自动把“八折”和“减 30”叠加。若需求变成优惠组合，需要另行定义执行顺序和组合规则。[strategy_replacement()](../examples/course/cpp-language-labs.cpp) 验证上述数值，并检查传入空策略会被拒绝；浮点金额仅用于教学，实际支付还要明确最小货币单位与舍入规则。

类型码或配置可以用于“选择哪种策略”，但不是策略模式的核心。核心是结账流程依赖统一调用接口，替换算法时不必在结账内部不断添加类型判断。

策略需要的额外状态可以保存在具体策略对象或 Lambda 捕获中；若捕获引用或 `this`，必须保证被引用对象活得足够久。包装器的成本和限制见[std::function](cpp-keywords-and-callables.md#166-stdfunction用统一签名保存可调用对象)。

## 1.12.4 观察者：状态变化后通知多个对象

被观察者维护订阅关系，状态改变时调用观察者的统一接口。它不必认识每个具体观察者类，添加新的监听者也不要求修改通知逻辑。

下面保留天气站例子，并补齐观察者接口与头文件。这个版本是**同步、单线程、非拥有型订阅**：观察者需在销毁前退订，回调期间不得修改订阅列表或销毁相关对象。

```cpp
#include <algorithm>
#include <vector>

class Observer {
public:
    virtual ~Observer() = default;
    virtual void onTemperatureChanged(double temperature) = 0;
};

class WeatherStation {
public:
    void subscribe(Observer& observer) {
        if (std::find(observers_.begin(), observers_.end(), &observer)
            == observers_.end()) {
            observers_.push_back(&observer);
        }
    }

    void unsubscribe(Observer& observer) {
        observers_.erase(
            std::remove(observers_.begin(), observers_.end(), &observer),
            observers_.end());
    }

    void setTemperature(double temperature) {
        temperature_ = temperature;
        notify();
    }
private:
    void notify() {
        for (Observer* observer : observers_) {
            observer->onTemperatureChanged(temperature_);
        }
    }

    double temperature_ = 0.0;
    std::vector<Observer*> observers_; // 不拥有观察者，不延长其生命周期
};
```

`erase-remove` 把仍保留的指针压到前段，再真正擦除尾段，详见[STL 中的删除惯用法](stl-and-algorithms.md#1954-erase-remove删除元素而不只是改区间)。

再补一个真正接收通知的对象，让订阅关系的变化可观察：

```cpp
struct Display : Observer {
    std::vector<double> received;
    void onTemperatureChanged(double temperature) override {
        received.push_back(temperature);
    }
};

Display wall;
Display phone;
WeatherStation station;
station.subscribe(wall);
station.subscribe(wall);        // 本实现防止重复订阅
station.subscribe(phone);
station.setTemperature(20.0);   // 两者各收到一次 20
station.unsubscribe(phone);
station.setTemperature(25.0);   // 只有 wall 收到 25
station.unsubscribe(wall);      // 在观察者结束生命周期前清理订阅
// wall.received == {20.0, 25.0}，phone.received == {20.0}
```

先登记地址，改变状态时逐个调用，退订后下一轮不再调用，这就是此例的完整时间线。[observer_subscription()](../examples/course/cpp-language-labs.cpp) 比较两份接收记录；重复订阅不重复通知是我们在 `subscribe` 中写出的策略，不是所有观察者系统天然都有的性质。这里的 `Display` 不会在回调内退订，遵守了当前实现的限制。

### 1.12.4.1 生命周期与通知期间修改

裸指针**不会延长**观察者生命周期。观察者已经销毁却没退订，下一次通知会访问悬空对象。可选择由作用域订阅令牌在析构时退订，或在观察者本就使用共享所有权时存 `weak_ptr`，每次通过 `lock()` 临时获得存活对象。

`weak_ptr` 解决的是观察对象存活问题，不自动解决订阅容器修改、并发或重入。若允许回调期间增删订阅，需要另外规定“本轮哪些回调仍会执行”，再用延迟修改队列、稳定节点或快照等实现。快照中的裸指针也不能保证对象仍活着。

观察者模式支持动态订阅，一次事件可分发给多个对象，但需要明确通知顺序、异常处理、重入和线程模型。同步通知中，某个观察者执行很久会拖住后续观察者；观察者之间相互触发又可能形成复杂更新链。

### 1.12.4.2 S18 事件系统记录

```text
Publisher → Event Bus → Subscribers
发布者       事件总线      订阅者
```

这是原笔记留下的项目线索，表达发布者经中间总线通知订阅者。仓库目前没有对应事件系统源码，不能据此确认它采用同步还是异步分发、是否持有强引用、如何退订或保证顺序；这些实现细节保留为待对照原工程核实的内容。

用需求变化检查自己是否选对机制：新增 `Bow` 是创建职责的变化；把八折改成九折是算法策略的变化；再增加一个天气屏幕是订阅者的变化。最后试着让 `phone` 在通知过程中退订自己：当前代码的遍历可能因 `vector` 擦除而失效，因此不能直接这样改；需要先选定延迟退订或快照等通知规则，再实现它。这道题的答案首先是接口契约，而不是多加一层类。
