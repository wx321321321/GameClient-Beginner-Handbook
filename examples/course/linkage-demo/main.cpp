#include "counter.h"
#include <cassert>
#include <iostream>

int main() {
    const int first = record_hit();
    const int second = record_hit();
    assert(first == 1 && second == 2 && hits == 2);
    (void)first;
    (void)second;
    std::cout << "hits=" << hits << ", doubled=" << doubled(hits) << '\n';
}
