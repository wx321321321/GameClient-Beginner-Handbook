// C++17 examples for the 2026-09-05 review.
// Build: g++ -std=c++17 -Wall -Wextra -pedantic cpp-os-review-2026-09-05.cpp -o review.exe
// OS address examples are numerical simulations, not real MMU/loader operations.
#include <array>
#include <cstdint>
#include <iostream>
#include <memory>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

namespace relocation {
void run() {
    std::uintptr_t preferred_base = 0x00400000;
    std::uintptr_t actual_base = 0x00600000;
    std::uintptr_t stored_function_address = 0x00401230;
    stored_function_address += actual_base - preferred_base;
    std::cout << "relocated absolute address: 0x" << std::hex
              << stored_function_address << std::dec << '\n';
}
}

namespace forwarding {
void receive(int&) { std::cout << "lvalue\n"; }
void receive(int&&) { std::cout << "rvalue\n"; }
template<class T>
void wrapper(T&& x) {
    receive(x);
    receive(std::forward<T>(x));
}
void run() {
    int a = 10;
    std::cout << "wrapper(a):\n";
    wrapper(a);
    std::cout << "wrapper(20):\n";
    wrapper(20);
    std::cout << "wrapper(std::move(a)):\n";
    wrapper(std::move(a));
}
}

namespace dynamic_relocation {
std::uint32_t translate(std::uint32_t logical_address,
                        std::uint32_t base, std::uint32_t limit) {
    if (logical_address >= limit) {
        throw std::out_of_range("address outside process region");
    }
    return base + logical_address;
}
void run() {
    std::cout << "base 10000, offset 120 -> " << translate(120, 10000, 1000) << '\n';
    std::cout << "base 20000, offset 120 -> " << translate(120, 20000, 1000) << '\n';
}
}

namespace paging {
void run() {
    // Simplified 32-bit, 10/10/12 split. Hardware PTEs have additional flags.
    using PageTable = std::array<std::optional<std::uint32_t>, 1024>;
    std::array<std::unique_ptr<PageTable>, 1024> directory{};
    const std::uint32_t va = 0x00403004;
    const auto dir_index = va >> 22;
    const auto table_index = (va >> 12) & 0x3ff;
    const auto offset = va & 0xfff;
    directory[dir_index] = std::make_unique<PageTable>();
    (*directory[dir_index])[table_index] = 9;  // physical frame number
    const auto& table = directory[dir_index];
    if (!table || !(*table)[table_index]) {
        throw std::runtime_error("unmapped page");
    }
    const auto physical = *(*table)[table_index] * 4096 + offset;
    std::cout << "directory=" << dir_index << ", entry=" << table_index
              << ", offset=" << offset << ", physical=0x" << std::hex
              << physical << std::dec << '\n';
}
}

namespace disk_blocks {
void run() {
    // Ordinary fully allocated file data; ignores metadata and special layouts.
    constexpr int file_bytes = 5000;
    constexpr int block_bytes = 4096;
    constexpr int sector_bytes = 512;
    constexpr int blocks = (file_bytes + block_bytes - 1) / block_bytes;
    std::cout << "blocks=" << blocks << ", allocated=" << blocks * block_bytes
              << ", unused=" << blocks * block_bytes - file_bytes
              << ", sectors/block=" << block_bytes / sector_bytes << '\n';
}
}

namespace arrays {
void run() {
    int arr[3] = {10, 20, 30};
    int* q = arr;
    int (*p)[3] = &arr;
    std::cout << "(*p)[1]=" << (*p)[1] << '\n';
    std::cout << "sizeof(p)=" << sizeof(p) << ", sizeof(*p)=" << sizeof(*p)
              << ", sizeof(*q)=" << sizeof(*q) << '\n';
    std::cout << "arr: " << static_cast<void*>(arr)
              << ", arr+1: " << static_cast<void*>(arr + 1)
              << ", &arr+1: " << static_cast<void*>(&arr + 1) << '\n';
    ++p; // one-past the sole array object; do not dereference p here
    --p;
    int matrix[2][3] = {{10, 20, 30}, {40, 50, 60}};
    int (*rows)[3] = matrix;
    std::cout << "(*rows)[1]=" << (*rows)[1]
              << ", rows[1][0]=" << rows[1][0] << '\n';
}
}

namespace strong_cycle {
struct B;
struct A {
    std::shared_ptr<B> b;
    ~A() { std::cout << "strong: ~A\n"; }
};
struct B {
    std::shared_ptr<A> a;
    ~B() { std::cout << "strong: ~B\n"; }
};
void run() {
    std::weak_ptr<A> observer;
    {
        auto a = std::make_shared<A>();
        auto b = std::make_shared<B>();
        observer = a;
        a->b = b;
        b->a = a;
        std::cout << "inside: " << a.use_count() << ' ' << b.use_count() << '\n';
    }
    std::cout << "after scope, A expired: " << std::boolalpha << observer.expired()
              << ", A strong count: " << observer.use_count() << '\n';
    // Cleanup for this demonstration. Without breaking the cycle, both survive.
    if (auto a = observer.lock()) {
        a->b.reset();
    }
}
}

namespace weak_cycle {
struct B;
struct A {
    std::shared_ptr<B> b;
    ~A() { std::cout << "weak: ~A\n"; }
};
struct B {
    std::weak_ptr<A> a;
    ~B() { std::cout << "weak: ~B\n"; }
};
void run() {
    auto a = std::make_shared<A>();
    auto b = std::make_shared<B>();
    a->b = b;
    b->a = a;
    std::cout << "inside: " << a.use_count() << ' ' << b.use_count() << '\n';
    if (auto owner = b->a.lock()) {
        std::cout << "weak lock succeeded\n";
    }
}
}

namespace multiple_inheritance {
class Base1 {
    int a = 1;
public:
    void op1() { std::cout << "a=" << a << '\n'; }
};
class Base2 {
    int b = 2;
public:
    void op2() {
        std::cout << "Base2::op2 this=" << static_cast<void*>(this)
                  << ", b=" << b << '\n';
    }
};
class Derived : public Base1, public Base2 {
    int d = 3;
public:
    int value() const { return d; }
};
void run() {
    Derived* d = new Derived();
    Base2* p = d;
    std::cout << "Derived*: " << static_cast<void*>(d)
              << ", Base2*: " << static_cast<void*>(p) << '\n';
    p->op2();
    delete d; // Base2 has no virtual destructor; never delete p here.
}
}

namespace construction {
struct Base {
    Base() { std::cout << "Base\n"; }
};
struct Member {
    explicit Member(const char* name) { std::cout << name << '\n'; }
};
struct Example : Base {
    Member first;
    Member second;
    Example() : Base(), first("first"), second("second") {
        std::cout << "constructor body\n";
    }
};
void run() {
    Example x;
    std::vector<int> count_value(3, 7);
    std::vector<int> elements{3, 7};
    std::cout << "vector(3,7) size=" << count_value.size()
              << ", vector{3,7} size=" << elements.size() << '\n';
}
}

namespace virtual_dispatch {
struct B {
    virtual void f() { std::cout << "B::f\n"; }
    virtual void g() { std::cout << "B::g\n"; }
    void h() { std::cout << "B::h\n"; }
    virtual ~B() = default;
};
struct C : B {
    void f() override { std::cout << "C::f\n"; }
    void h() { std::cout << "C::h\n"; }
    void call_parent() { B::f(); }
};
struct NoOverride : B {};
void run() {
    B b;
    C c;
    B* p = &c;
    b.f();
    p->f();
    p->g();
    p->h();
    c.h();
    c.B::f();
    c.call_parent();
    NoOverride n;
    n.f();
    n.h();
}
}

namespace slicing {
struct Base {
    int x = 10;
    virtual void f() const { std::cout << "Base::f, x=" << x << '\n'; }
    virtual ~Base() = default;
};
struct Derived : Base {
    int y = 20;
    void f() const override { std::cout << "Derived::f, y=" << y << '\n'; }
};
void by_value(Base b) { b.f(); }
void by_reference(const Base& b) { b.f(); }
void run() {
    Derived d;
    Base* p = &d;
    Base& r = d;
    Base b = d;
    p->f();
    r.f();
    b.f();
    by_value(d);
    by_reference(d);
    Base other;
    other.x = 99;
    r = other; // Assign the Base subobject; d remains a Derived object.
    r.f();
    std::cout << "d.x after base assignment=" << d.x << '\n';
}
}

int main() {
    std::cout << "[1] Load relocation (simulation)\n";
    relocation::run();
    std::cout << "\n[2] std::forward\n";
    forwarding::run();
    std::cout << "\n[3] Base/limit relocation (simulation)\n";
    dynamic_relocation::run();
    std::cout << "\n[4] Two-level paging (simulation)\n";
    paging::run();
    std::cout << "\n[5] Disk blocks (calculation)\n";
    disk_blocks::run();
    std::cout << "\n[6] Array pointers and sizeof\n";
    arrays::run();
    std::cout << "\n[7] shared_ptr cycle and weak_ptr\n";
    strong_cycle::run();
    weak_cycle::run();
    std::cout << "\n[8] Multiple inheritance\n";
    multiple_inheritance::run();
    std::cout << "\n[9] Construction order\n";
    construction::run();
    std::cout << "\n[10-11] Virtual and nonvirtual calls\n";
    virtual_dispatch::run();
    std::cout << "\n[12] Object slicing\n";
    slicing::run();
}
