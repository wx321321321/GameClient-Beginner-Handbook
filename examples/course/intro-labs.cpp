#ifdef NDEBUG
#error "Teaching labs require assertions; compile without NDEBUG."
#endif

#include <cassert>
#include <iostream>

int ApplyDamage(int health, int damage) {
    if (damage < 0) return health;
    if (damage >= health) return 0;
    return health - damage;
}

bool CanFire(int health, int ammo, bool reloading) {
    return health > 0 && ammo > 0 && !reloading;
}

struct Player {
    int health{100};
    int ammo{30};
};

int main() {
    int health = 100;
    const int hits[] = {30, 30, 50};
    const int expected[] = {70, 40, 0};
    int index = 0;
    for (int damage : hits) {
        health = ApplyDamage(health, damage);
        assert(health == expected[index++]);
        std::cout << "health=" << health << '\n';
    }
    assert(ApplyDamage(70, -1) == 70);
    assert(ApplyDamage(0, 20) == 0);
    assert(CanFire(100, 1, false));
    assert(!CanFire(0, 1, false));
    assert(!CanFire(100, 0, false));
    assert(!CanFire(100, 1, true));
    const double integerDivision = 5 / 2;
    assert(integerDivision == 2.0);
    assert(static_cast<double>(5) / 2 == 2.5);

    int total = 0;
    for (int damage : hits) total += damage;
    assert(total == 110);
    Player a;
    Player b;
    a.health = ApplyDamage(a.health, 30);
    assert(a.health == 70 && b.health == 100);
    std::cout << "intro checks passed\n";
}
