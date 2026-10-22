// C++17 companion to the six C++ language/container/pattern chapters.
// Build with assertions enabled, for example:
//   g++ -std=c++17 -Wall -Wextra -pedantic cpp-language-labs.cpp -o cpp-language-labs
// Run the executable: each [ok] line follows that group's assertions.
// Intentionally invalid examples remain comments in the chapters, not here.

#ifdef NDEBUG
#error "These teaching labs require assertions; compile without NDEBUG."
#endif

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <functional>
#include <iostream>
#include <iterator>
#include <map>
#include <memory>
#include <queue>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

namespace pointer_labs {

void damage_copy(int hp) { hp -= 10; }
void damage_ref(int& hp) { hp -= 10; }
void damage_ptr(int* hp) {
    if (hp != nullptr) { *hp -= 10; }
}

void parameter_access() {
    int hp = 100;
    damage_copy(hp);
    assert(hp == 100);
    damage_ref(hp);
    assert(hp == 90);
    damage_ptr(&hp);
    assert(hp == 80);
    damage_ptr(nullptr);
    assert(hp == 80);

    int other = 50;
    int& alias = hp;
    alias = other;
    assert(hp == 50 && &alias == &hp);
    int* target = &hp;
    target = &other;
    *target = 40;
    assert(other == 40 && hp == 50);
}

void unique_transfer() {
    auto first_owner = std::make_unique<int>(10);
    auto&& movable = std::move(first_owner);
    assert(first_owner && *first_owner == 10);
    assert(&movable == &first_owner); // move alone only produces a reference
    auto second_owner = std::move(movable);
    assert(!first_owner);
    assert(second_owner && *second_owner == 10);
}

void weak_lifetime() {
    std::weak_ptr<int> observer;
    {
        auto owner = std::make_shared<int>(42);
        observer = owner;
        assert(owner.use_count() == 1);
        {
            auto hold = observer.lock();
            assert(hold && *hold == 42);
            assert(owner.use_count() == 2);
        }
        assert(owner.use_count() == 1);
    }
    auto missing = observer.lock();
    assert(observer.expired() && !missing);
}

void run() {
    parameter_access();
    unique_transfer();
    weak_lifetime();
}
} // namespace pointer_labs

namespace class_labs {

class Health {
    int value_ = 100;
    mutable int reads_ = 0;
public:
    inline static int created = 0;
    Health() { ++created; }
    int value() const { ++reads_; return value_; }
    int reads() const { return reads_; }
};

void member_kinds() {
    const Health first;
    Health second;
    int hp = first.value();
    first.value();
    assert(hp == 100);
    assert(first.reads() == 2 && second.reads() == 0);
    assert(Health::created == 2);
}

// Teaching-only event collection: string/vector operations can allocate.
// Real cleanup code should not allow exceptions to escape a destructor.
struct Trace {
    std::string name;
    std::vector<std::string>& events;
    Trace(std::string n, std::vector<std::string>& e)
        : name(std::move(n)), events(e) { events.push_back(name + "+"); }
    ~Trace() { events.push_back(name + "-"); }
};
struct Character : Trace {
    Trace weapon;
    Trace armor;
    explicit Character(std::vector<std::string>& e)
        : Trace("base", e), weapon("weapon", e), armor("armor", e) {
        events.push_back("body+");
    }
    ~Character() { events.push_back("body-"); }
};

void lifetime_order() {
    std::vector<std::string> events;
    { Character character(events); }
    assert((events == std::vector<std::string>{
        "base+", "weapon+", "armor+", "body+",
        "body-", "armor-", "weapon-", "base-"
    }));
}

struct Actor {
    virtual ~Actor() = default;
    virtual int damage() const { return 1; }
};
struct Ninja : Actor {
    int damage() const override { return 10; }
};
int through_reference(const Actor& object) { return object.damage(); }
int through_value(Actor object) { return object.damage(); }

void dispatch_and_slice() {
    Ninja fighter;
    int a = through_reference(fighter);
    int b = through_value(fighter);
    int c = fighter.Actor::damage();
    assert(a == 10 && b == 1 && c == 1);
    assert(fighter.damage() == 10);
}

struct A { int value = 0; };
struct B : virtual A {};
struct C : virtual A {};
struct D : B, C {};

void shared_virtual_base() {
    D diamond;
    A* via_b = static_cast<B*>(&diamond);
    A* via_c = static_cast<C*>(&diamond);
    via_b->value = 7;
    assert(via_b == via_c && via_c->value == 7);
}

void run() {
    member_kinds();
    lifetime_order();
    dispatch_and_slice();
    shared_virtual_base();
}
} // namespace class_labs

namespace keyword_labs {

void numeric_conversion() {
    int score = 5;
    int games = 2;
    double a = score / games;
    double b = static_cast<double>(score / games);
    double c = static_cast<double>(score) / games;
    assert(a == 2.0 && b == 2.0 && c == 2.5);
}

void unevaluated_sizeof() {
    int counter = 0;
    std::size_t bytes = sizeof(++counter);
    assert(counter == 0 && bytes == sizeof(int));
}

int next_ticket() {
    static int ticket = 0;
    return ++ticket;
}

void persistent_local() {
    int first = next_ticket();
    int second = next_ticket();
    assert(first == 1 && second == 2);
}

void capture_time() {
    int bonus = 10;
    auto snapshot = [bonus](int score) { return score + bonus; };
    auto live = [&bonus](int score) { return score + bonus; };
    bonus = 20;
    int a = snapshot(5);
    int b = live(5);
    assert(a == 15 && b == 25);
}

void copied_callable() {
    std::function<int()> first = [n = 0]() mutable { return ++n; };
    int a = first();
    auto second = first;
    int b = first();
    int c = second();
    int d = first();
    assert(a == 1 && b == 2 && c == 2 && d == 3);
}

void run() {
    numeric_conversion();
    unevaluated_sizeof();
    persistent_local();
    capture_time();
    copied_callable();
}
} // namespace keyword_labs

namespace template_labs {

const char* choose(int) { return "ordinary"; }
template<class T> const char* choose(T) { return "template"; }

template<class T>
T add(T a, T b) { return a + b; }

void deduction_and_overload() {
    assert(std::string(choose(1)) == "ordinary");
    assert(std::string(choose(1.5)) == "template");
    int total = add(1, 2);
    double speed = add(1.5, 2.0);
    std::string name = add(std::string("Red"), std::string("Ninja"));
    double mixed = add<double>(1, 2.0);
    assert(total == 3 && speed == 3.5 && name == "RedNinja" && mixed == 3.0);
    static_assert(std::is_same_v<decltype(add(1, 2)), int>);
    static_assert(std::is_same_v<decltype(add<double>(1, 2)), double>);
}

template<class T, std::size_t N>
constexpr std::size_t array_count(const T (&)[N]) { return N; }

void array_extent() {
    int slots[4]{};
    static_assert(array_count(slots) == 4);
    assert(array_count(slots) == 4);
}

template<class T, class U> struct PairInfo {
    static const char* name() { return "general"; }
};
template<> struct PairInfo<int, float> {
    static const char* name() { return "exact"; }
};
template<class T> struct PairInfo<T, T*> {
    static const char* name() { return "pointer-pair"; }
};

void specialization_selection() {
    assert((std::string(PairInfo<int, double>::name()) == "general"));
    assert((std::string(PairInfo<int, float>::name()) == "exact"));
    assert((std::string(PairInfo<int, int*>::name()) == "pointer-pair"));
    assert((std::string(PairInfo<int, double*>::name()) == "general"));
}

void return_type_preservation() {
    int hp = 100;
    auto get_hp = [&hp]() -> int& { return hp; };
    auto copy = get_hp();
    decltype(auto) alias = get_hp();
    copy = 50;
    assert(hp == 100);
    alias = 80;
    assert(hp == 80 && copy == 50);
    static_assert(std::is_same_v<decltype(copy), int>);
    static_assert(std::is_same_v<decltype(alias), int&>);
}

template<class Derived>
class Base {
protected:
    Base() = default;
public:
    void interface() { static_cast<Derived*>(this)->implementation(); }
};
class Player : public Base<Player> {
    int attacks_ = 0;
public:
    void implementation() { ++attacks_; }
    int attacks() const { return attacks_; }
};

void static_dispatch() {
    Player player;
    player.interface();
    player.interface();
    assert(player.attacks() == 2);
}

void run() {
    deduction_and_overload();
    array_extent();
    specialization_selection();
    return_type_preservation();
    static_dispatch();
}
} // namespace template_labs

namespace stl_labs {

void vector_state() {
    std::vector<int> bag{10, 20};
    bag.reserve(8);
    const auto reserved = bag.capacity();
    assert(reserved >= 8 && bag.size() == 2);
    bag.push_back(30);
    assert((bag == std::vector<int>{10, 20, 30}));
    bag.resize(5);
    assert((bag == std::vector<int>{10, 20, 30, 0, 0}));
    bag.resize(1);
    assert((bag == std::vector<int>{10}));
    assert(bag.capacity() == reserved);
}

void map_lookup() {
    std::map<std::string, int> scores{{"Ninja", 10}};
    auto missing = scores.find("Mage");
    bool found = missing != scores.end();
    assert(!found && scores.size() == 1);
    int default_score = scores["Mage"];
    assert(default_score == 0 && scores.size() == 2);
}

void hash_reference() {
    std::unordered_map<int, std::string> names{{1, "Ninja"}};
    std::string& borrowed = names.at(1);
    names.rehash(names.bucket_count() + 1);
    borrowed += "!";
    assert(names.at(1) == "Ninja!");
}

void erase_in_two_steps() {
    std::vector<int> values{1, 2, 3, 2};
    auto logical_end = std::remove(values.begin(), values.end(), 2);
    auto kept = std::distance(values.begin(), logical_end);
    assert(values.size() == 4 && kept == 2);
    assert(values[0] == 1 && values[1] == 3);
    values.erase(logical_end, values.end());
    assert((values == std::vector<int>{1, 3}));
}

void dijkstra_small_graph() {
    using Edge = std::pair<int, int>;
    std::vector<std::vector<Edge>> graph{
        {{1, 4}, {2, 1}},
        {{3, 1}},
        {{1, 2}, {3, 5}},
        {}
    };
    constexpr int infinity = 1'000'000;
    std::vector<int> distance(4, infinity);
    using Entry = std::pair<int, int>;
    std::priority_queue<Entry, std::vector<Entry>, std::greater<Entry>> queue;
    distance[0] = 0;
    queue.push({0, 0});
    int skipped = 0;
    while (!queue.empty()) {
        auto [cost, u] = queue.top();
        queue.pop();
        if (cost != distance[u]) { ++skipped; continue; }
        for (auto [v, weight] : graph[u]) {
            int candidate = cost + weight;
            if (candidate < distance[v]) {
                distance[v] = candidate;
                queue.push({candidate, v});
            }
        }
    }
    assert((distance == std::vector<int>{0, 3, 1, 4}));
    assert(skipped == 2);
}

void run() {
    vector_state();
    map_lookup();
    hash_reference();
    erase_in_two_steps();
    dijkstra_small_graph();
}
} // namespace stl_labs

namespace pattern_labs {

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

void singleton_identity() {
    Singleton& first = Singleton::get();
    Singleton& second = Singleton::get();
    bool same_object = &first == &second;
    assert(same_object);
}

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

void factory_selection() {
    auto sword = make_weapon(WeaponKind::sword);
    auto bow = make_weapon(WeaponKind::bow);
    assert(sword->damage() == 10 && bow->damage() == 6);
}

class Checkout {
    std::function<double(double)> strategy_ = [](double price) { return price; };
public:
    void set_strategy(std::function<double(double)> strategy) {
        if (!strategy) {
            throw std::invalid_argument("empty pricing strategy");
        }
        strategy_ = std::move(strategy);
    }
    double total(double price) const { return strategy_(price); }
};

void strategy_replacement() {
    Checkout checkout;
    double normal = checkout.total(100.0);
    checkout.set_strategy([](double p) { return p * 0.8; });
    double vip = checkout.total(100.0);
    checkout.set_strategy([](double p) { return std::max(0.0, p - 30.0); });
    double sale = checkout.total(100.0);
    double small = checkout.total(20.0);
    assert(normal == 100.0 && std::abs(vip - 80.0) < 1e-9);
    assert(sale == 70.0 && small == 0.0);
    bool rejected = false;
    try { checkout.set_strategy({}); }
    catch (const std::invalid_argument&) { rejected = true; }
    assert(rejected && checkout.total(100.0) == 70.0);
}

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
    std::vector<Observer*> observers_;
};

struct Display : Observer {
    std::vector<double> received;
    void onTemperatureChanged(double temperature) override {
        received.push_back(temperature);
    }
};

void observer_subscription() {
    Display wall;
    Display phone;
    WeatherStation station;
    station.subscribe(wall);
    station.subscribe(wall);
    station.subscribe(phone);
    station.setTemperature(20.0);
    station.unsubscribe(phone);
    station.setTemperature(25.0);
    station.unsubscribe(wall);
    assert((wall.received == std::vector<double>{20.0, 25.0}));
    assert((phone.received == std::vector<double>{20.0}));
}

void run() {
    singleton_identity();
    factory_selection();
    strategy_replacement();
    observer_subscription();
}
} // namespace pattern_labs

int main() {
    pointer_labs::run();
    std::cout << "[ok] pointers: access, move, weak lifetime\n";
    class_labs::run();
    std::cout << "[ok] classes: members, lifetime, dispatch, virtual base\n";
    keyword_labs::run();
    std::cout << "[ok] keywords: conversion, sizeof, static, closures\n";
    template_labs::run();
    std::cout << "[ok] templates: deduction, extent, specialization, references, CRTP\n";
    stl_labs::run();
    std::cout << "[ok] STL: vector, map, rehash, erase, Dijkstra\n";
    pattern_labs::run();
    std::cout << "[ok] patterns: singleton, factory, strategy, observer\n";
}
