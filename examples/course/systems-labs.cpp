// C++17 teaching experiments. No benchmark claims are made.
// Build: g++ -std=c++17 -O0 -Wall -Wextra -pedantic -pthread systems-labs.cpp -o systems-labs
// Run:   systems-labs [all|hardware|os|memory|threads]
// File experiments only create their own fresh temporary directory and remove
// the one file they create, then the empty directory. They never recurse.
#include <array>
#include <atomic>
#include <cassert>
#include <chrono>
#include <climits>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <functional>
#include <future>
#include <iomanip>
#include <iostream>
#include <limits>
#include <memory>
#include <mutex>
#include <new>
#include <queue>
#include <stdexcept>
#include <string>
#include <system_error>
#include <thread>
#include <utility>
#include <vector>

namespace hardware {
void unsigned_wrap() {
    unsigned value = std::numeric_limits<unsigned>::max();
    ++value;
    assert(value == 0);
    std::cout << "unsigned wrap: " << value << '\n';
    // Do not repeat this with signed INT_MAX: signed overflow is undefined.
}

void object_bytes() {
    static_assert(CHAR_BIT == 8, "This byte demonstration requires 8-bit bytes.");
    static_assert(sizeof(std::uint32_t) == 4, "This example needs a 32-bit integer.");
    const std::uint32_t value = 0x12345678u;
    std::array<unsigned char, sizeof(value)> bytes{};
    std::memcpy(bytes.data(), &value, sizeof(value));
    std::cout << "native bytes:";
    for (unsigned char byte : bytes) {
        std::cout << ' ' << std::hex << std::setw(2) << std::setfill('0')
                  << static_cast<unsigned>(byte);
    }
    std::cout << std::dec << std::setfill(' ') << '\n';
    // Observe the native byte order; neither little nor big endian is asserted.
    std::uint32_t restored = 0;
    std::memcpy(&restored, bytes.data(), sizeof(restored));
    assert(restored == value);
}

// A deliberately tiny MODEL: two direct-mapped lines; four elements per line.
// Element indexes, not actual CPU addresses, are fed to the model.
class CacheModel {
public:
    bool access(int element_index) {
        const int block = element_index / 4;
        const int slot = block % 2;
        const bool hit = tags_[slot] == block;
        tags_[slot] = block;
        return hit;
    }
private:
    std::array<int, 2> tags_{{-1, -1}};
};

void traversal() {
    std::array<int, 16> data{};
    for (int i = 0; i < 16; ++i) data[i] = i;
    CacheModel row_cache;
    CacheModel column_cache;
    int row_sum = 0, column_sum = 0;
    int row_misses = 0, column_misses = 0;
    for (int row = 0; row < 4; ++row) {
        for (int column = 0; column < 4; ++column) {
            const int index = row * 4 + column;
            row_sum += data[index];
            if (!row_cache.access(index)) ++row_misses;
        }
    }
    for (int column = 0; column < 4; ++column) {
        for (int row = 0; row < 4; ++row) {
            const int index = row * 4 + column;
            column_sum += data[index];
            if (!column_cache.access(index)) ++column_misses;
        }
    }
    assert(row_sum == 120 && column_sum == 120);
    (void)column_sum;
    assert(row_misses == 4 && column_misses == 16);
    std::cout << "same sum: " << row_sum << "; model misses row/column: "
              << row_misses << '/' << column_misses << '\n';
}
void run() { unsigned_wrap(); object_bytes(); traversal(); }
} // namespace hardware

namespace os {
class TemporaryFile {
public:
    TemporaryFile() {
        const auto parent = std::filesystem::temp_directory_path();
        const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
        for (int attempt = 0; attempt < 64; ++attempt) {
            directory_ = parent / ("handbook-systems-" + std::to_string(stamp)
                                    + "-" + std::to_string(attempt));
            file_ = directory_ / "records.bin";
            std::error_code error;
            if (std::filesystem::create_directory(directory_, error)) {
                owns_directory_ = true;
                return;
            }
            if (error) throw std::filesystem::filesystem_error(
                "create temporary directory", directory_, error);
        }
        throw std::runtime_error("Unable to create a fresh temporary directory");
    }
    TemporaryFile(const TemporaryFile&) = delete;
    TemporaryFile& operator=(const TemporaryFile&) = delete;
    ~TemporaryFile() {
        if (!owns_directory_) return;
        std::error_code ignored;
        std::filesystem::remove(file_, ignored);
        std::filesystem::remove(directory_, ignored);
    }
    const std::filesystem::path& path() const { return file_; }
    void cleanup() {
        if (!owns_directory_) return;
        // Exact known paths only: never remove_all, never follow a list of paths.
        std::filesystem::remove(file_);
        std::filesystem::remove(directory_);
        owns_directory_ = false;
        assert(!std::filesystem::exists(directory_));
    }
private:
    std::filesystem::path directory_;
    std::filesystem::path file_;
    bool owns_directory_ = false;
};

void write_bytes(const std::filesystem::path& path, const std::string& bytes) {
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out) throw std::runtime_error("Cannot open private temporary file");
    out.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
    out.close(); // Check errors while they can still be reported.
    if (!out) throw std::runtime_error("Cannot write private temporary file");
}

void offset_and_eof() {
    TemporaryFile temporary;
    write_bytes(temporary.path(), "ABCDEF");
    {
        std::ifstream input(temporary.path(), std::ios::binary);
        if (!input) throw std::runtime_error("Cannot reopen private temporary file");
        std::array<char, 2> prefix{};
        input.read(prefix.data(), 2);
        assert(input.gcount() == 2 && input.tellg() == std::streampos(2));
        assert((prefix == std::array<char, 2>{{'A', 'B'}}));
        input.seekg(4, std::ios::beg);
        char fifth = '\0';
        input.get(fifth);
        assert(fifth == 'E' && input.tellg() == std::streampos(5));
        std::array<char, 4> tail{};
        input.read(tail.data(), 4);
        assert(input.gcount() == 1 && tail[0] == 'F');
        assert(input.eof() && input.fail());
        input.clear(); // Clear the failed/EOF state before seeking and reading again.
        input.seekg(0, std::ios::beg);
        char first = '\0';
        input.get(first);
        assert(first == 'A');
        std::cout << "file offset: AB, E, tail=F (1 byte); rewind=" << first << '\n';
    }
    temporary.cleanup();
}

void fixed_records() {
    TemporaryFile temporary;
    write_bytes(temporary.path(), "1001Ada\n1002Lin\n1003Bo \n");
    {
        std::ifstream input(temporary.path(), std::ios::binary);
        if (!input) throw std::runtime_error("Cannot open fixed records");
        constexpr std::streamoff record_size = 8;
        input.seekg(2 * record_size, std::ios::beg);
        std::array<char, 8> record{};
        input.read(record.data(), static_cast<std::streamsize>(record.size()));
        assert(input.gcount() == 8);
        const std::string actual(record.data(), record.size());
        assert(actual == "1003Bo \n");
        std::cout << "record index 2: " << actual;
    }
    temporary.cleanup();
}

void independent_opens() {
    TemporaryFile temporary;
    write_bytes(temporary.path(), "ABCDEF");
    {
        std::ifstream left(temporary.path(), std::ios::binary);
        std::ifstream right(temporary.path(), std::ios::binary);
        if (!left || !right) throw std::runtime_error("Cannot open two input streams");
        char a = '\0', b = '\0', c = '\0';
        left.get(a);
        left.get(b);
        right.get(c);
        assert(a == 'A' && b == 'B' && c == 'A');
        std::cout << "independent opens: " << a << ' ' << b << ' ' << c << '\n';
    } // Each stream closes its own file resource here.
    temporary.cleanup();
}
void run() { offset_and_eof(); fixed_records(); independent_opens(); }
} // namespace os

namespace memory {
struct Trace {
    const char* name;
    std::vector<std::string>& events;
    Trace(const char* n, std::vector<std::string>& e) : name(n), events(e) {
        events.emplace_back(std::string("+") + name);
    }
    ~Trace() { events.emplace_back(std::string("-") + name); }
};

void lifetime() {
    std::vector<std::string> events;
    events.reserve(6);
    {
        Trace outer("outer", events);
        { Trace inner("inner", events); }
        auto owned = std::make_unique<Trace>("dynamic", events);
    }
    const std::vector<std::string> expected{
        "+outer", "+inner", "-inner", "+dynamic", "-dynamic", "-outer"};
    assert(events == expected);
    std::cout << "lifetime:";
    for (const auto& event : events) std::cout << ' ' << event;
    std::cout << '\n';
}

std::pair<int, int> call_counts() {
    int local = 0;
    static int calls = 0;
    return {++local, ++calls};
}
void storage_duration() {
    const auto first = call_counts();
    const auto second = call_counts();
    assert(first.first == 1 && second.first == 1);
    assert(second.second == first.second + 1);
    std::cout << "local/static: " << first.first << '/' << first.second
              << ", " << second.first << '/' << second.second << '\n';
}

struct LayoutA { char tag; double value; char state; };
struct LayoutB { double value; char tag; char state; };
void layout() {
    static_assert(offsetof(LayoutA, value) % alignof(double) == 0, "Member alignment");
    static_assert(sizeof(LayoutA) % alignof(LayoutA) == 0, "Array stride alignment");
    std::cout << "layout A/B sizes: " << sizeof(LayoutA) << '/' << sizeof(LayoutB)
              << "; A.value offset: " << offsetof(LayoutA, value)
              << "; double alignment: " << alignof(double) << '\n';
    // 24/16 is common, not a cross-platform assertion.
}

struct Counted {
    inline static int alive = 0;
    Counted() { ++alive; }
    Counted(const Counted&) { ++alive; }
    Counted(Counted&&) noexcept { ++alive; }
    ~Counted() { --alive; }
};
void storage_vs_objects() {
    assert(Counted::alive == 0);
    {
        std::vector<Counted> objects;
        objects.reserve(3);
        assert(objects.capacity() >= 3 && objects.empty() && Counted::alive == 0);
        objects.emplace_back();
        objects.emplace_back();
        assert(objects.size() == 2 && Counted::alive == 2);
        objects.clear();
        assert(objects.empty() && Counted::alive == 0 && objects.capacity() >= 3);
        std::cout << "vector live objects: 0 -> 2 -> 0; capacity >= 3\n";
    }
    alignas(std::string) std::byte storage[sizeof(std::string)];
    using Text = std::string;
    Text* text = ::new (static_cast<void*>(storage)) Text("hello");
    assert(*text == "hello");
    text->~Text();
}
void run() { lifetime(); storage_duration(); layout(); storage_vs_objects(); }
} // namespace memory

namespace threads {
void join_and_move() {
    int value = 0;
    std::thread worker([](int& out) { out = 42; }, std::ref(value));
    assert(worker.joinable());
    std::thread owner = std::move(worker);
    assert(!worker.joinable() && owner.joinable());
    owner.join();
    assert(!owner.joinable() && value == 42);
    std::cout << "join result: " << value << '\n';
}

class BlockingQueue {
public:
    bool push(int value) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (closed_) return false;
            queue_.push(value);
        }
        changed_.notify_one();
        return true;
    }
    bool pop(int& value) {
        std::unique_lock<std::mutex> lock(mutex_);
        changed_.wait(lock, [this] { return closed_ || !queue_.empty(); });
        if (queue_.empty()) return false; // closed and drained
        value = queue_.front();
        queue_.pop();
        return true;
    }
    void close() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            closed_ = true;
        }
        changed_.notify_all();
    }
private:
    std::mutex mutex_;
    std::condition_variable changed_;
    std::queue<int> queue_;
    bool closed_ = false;
};

void queue_shutdown() {
    BlockingQueue queue;
    std::vector<int> received;
    received.reserve(3);
    std::thread reader([&] {
        int value = 0;
        while (queue.pop(value)) received.push_back(value);
    });
    try {
        // Keep side effects outside assert: the example also works with NDEBUG.
        const bool a = queue.push(1);
        const bool b = queue.push(2);
        const bool c = queue.push(3);
        assert(a && b && c);
        (void)a; (void)b; (void)c;
        queue.close();
    } catch (...) {
        queue.close();
        reader.join();
        throw;
    }
    reader.join();
    assert((received == std::vector<int>{1, 2, 3}));
    const bool accepted = queue.push(4);
    assert(!accepted);
    (void)accepted;
    std::cout << "queue drained: 1 2 3; closed push rejected\n";
}

void split_atomic_schedule() {
    std::atomic<int> counter{0};
    // Sequential rehearsal of a possible interleaving, not a racy experiment.
    const int seen_by_a = counter.load();
    const int seen_by_b = counter.load();
    counter.store(seen_by_a + 1);
    counter.store(seen_by_b + 1);
    assert(counter.load() == 1);
    counter.store(0);
    counter.fetch_add(1);
    counter.fetch_add(1);
    assert(counter.load() == 2);
    std::cout << "split load/store: 1; fetch_add: 2\n";
}

void publication() {
    int data = 0;
    int observed = 0;
    std::atomic<bool> ready{false};
    // Run the writer in the calling thread so only one child needs to be joined.
    std::thread reader([&] {
        while (!ready.load(std::memory_order_acquire)) std::this_thread::yield();
        observed = data;
    });
    data = 42;
    ready.store(true, std::memory_order_release);
    reader.join();
    assert(observed == 42);
    std::cout << "published data: " << observed << '\n';
}

void async_result() {
    auto result = std::async(std::launch::async,
                             [](int a, int b) { return a + b; }, 10, 20);
    const int sum = result.get();
    assert(sum == 30 && !result.valid());
    int calls = 0;
    auto deferred = std::async(std::launch::deferred, [&] { return ++calls; });
    assert(calls == 0);
    const int call_result = deferred.get();
    assert(call_result == 1 && calls == 1);
    (void)call_result;
    std::cout << "async result: " << sum << "; deferred calls: " << calls << '\n';
}
void run() { join_and_move(); queue_shutdown(); split_atomic_schedule(); publication(); async_result(); }
} // namespace threads

int main(int argc, char** argv) {
    try {
        const std::string group = argc > 1 ? argv[1] : "all";
        if (group != "all" && group != "hardware" && group != "os"
            && group != "memory" && group != "threads") {
            std::cerr << "Use all, hardware, os, memory, or threads\n";
            return 2;
        }
        if (group == "all" || group == "hardware") hardware::run();
        if (group == "all" || group == "os") os::run();
        if (group == "all" || group == "memory") memory::run();
        if (group == "all" || group == "threads") threads::run();
        std::cout << "systems labs passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "systems labs failed: " << error.what() << '\n';
        return 1;
    }
}
