// Arena teaching labs, C++17. No engine, network, rendering or external libraries.
// Units: metres and seconds. 60 Hz is an example, not a production requirement.
#ifdef NDEBUG
#error "Arena labs require assertions; compile without NDEBUG."
#endif
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <map>
#include <set>
#include <stdexcept>

namespace arena {
using Tick = std::uint64_t;
using ShotSeq = std::uint64_t;
constexpr double TickSeconds = 1.0 / 60.0;

struct EntityHandle {
    std::uint32_t slot{};
    std::uint32_t generation{};
};
bool SameHandle(EntityHandle a, EntityHandle b) {
    return a.slot == b.slot && a.generation == b.generation;
}

struct WeaponConfig {
    int magazineCapacity = 3;
    Tick fireInterval = 6;  // 600 rounds/min at 60 Hz.
    Tick reloadDuration = 120;
};
enum class WeaponPhase { Ready, Reloading, Disabled };
enum class FireDecision { Accepted, Cooldown, Empty, Busy, NotAllowed, Stale };
struct FireReply {
    ShotSeq sequence{};
    Tick authoritativeTick{};
    FireDecision decision{};
};
struct FireOutcome {
    FireReply reply;
    bool firstProcessing{}; // Only a first accepted result may emit a new shot.
};

class Weapon {
    WeaponConfig config_;
    WeaponPhase phase_ = WeaponPhase::Ready;
    int magazine_{};
    int reserve_{};
    Tick nextFire_{};
    Tick reloadEnd_{};
    ShotSeq highest_{};
    std::map<ShotSeq, FireReply> replies_; // Unbounded only for this short lab.
public:
    explicit Weapon(WeaponConfig config, int reserve)
        : config_(config), magazine_(config.magazineCapacity), reserve_(reserve) {
        if (config.magazineCapacity <= 0 || reserve < 0 ||
            config.fireInterval == 0 || config.reloadDuration == 0)
            throw std::invalid_argument("invalid weapon configuration");
    }
    int Magazine() const { return magazine_; }
    int Reserve() const { return reserve_; }
    WeaponPhase Phase() const { return phase_; }
    void Update(Tick now) {
        if (phase_ == WeaponPhase::Reloading && now >= reloadEnd_) {
            const int transfer = std::min(config_.magazineCapacity - magazine_, reserve_);
            magazine_ += transfer;
            reserve_ -= transfer;
            phase_ = WeaponPhase::Ready;
        }
    }
    bool BeginReload(Tick now) {
        Update(now);
        if (phase_ != WeaponPhase::Ready || reserve_ == 0 ||
            magazine_ == config_.magazineCapacity) return false;
        phase_ = WeaponPhase::Reloading;
        reloadEnd_ = now + config_.reloadDuration; // Session ends before Tick wrap.
        return true;
    }
    void CancelReload() {
        if (phase_ == WeaponPhase::Reloading) phase_ = WeaponPhase::Ready;
    }
    void Disable() { phase_ = WeaponPhase::Disabled; }
    FireOutcome TryFire(Tick now, ShotSeq sequence, bool allowed = true) {
        if (auto it = replies_.find(sequence); it != replies_.end())
            return {it->second, false};
        if (sequence == 0 || sequence <= highest_)
            return {{sequence, now, FireDecision::Stale}, false};
        highest_ = sequence;
        Update(now);
        FireDecision decision = FireDecision::Accepted;
        if (!allowed) decision = FireDecision::NotAllowed;
        else if (phase_ != WeaponPhase::Ready) decision = FireDecision::Busy;
        else if (now < nextFire_) decision = FireDecision::Cooldown;
        else if (magazine_ == 0) decision = FireDecision::Empty;
        if (decision == FireDecision::Accepted) {
            --magazine_;
            nextFire_ = now + config_.fireInterval;
        }
        FireReply reply{sequence, now, decision};
        replies_.emplace(sequence, reply);
        return {reply, true};
    }
};

class CombatTarget {
    std::uint64_t life_ = 1;
    int hp_ = 100;
    int armor_ = 40;
    int deathTransitions_ = 0;
    std::set<std::uint64_t> applied_; // Server damage-event IDs, scoped to match.
public:
    std::uint64_t Life() const { return life_; }
    int Hp() const { return hp_; }
    int Armor() const { return armor_; }
    int DeathTransitions() const { return deathTransitions_; }
    bool Apply(std::uint64_t event, std::uint64_t expectedLife, int rawDamage) {
        if (event == 0 || expectedLife != life_ || rawDamage < 0 || hp_ == 0)
            return false;
        if (!applied_.insert(event).second) return false;
        const int absorbed = std::min(armor_, rawDamage);
        armor_ -= absorbed;
        hp_ = std::max(0, hp_ - (rawDamage - absorbed));
        if (hp_ == 0) ++deathTransitions_;
        return true;
    }
    void Respawn() {
        if (hp_ != 0) throw std::logic_error("cannot respawn a live target");
        ++life_;
        hp_ = 100;
        armor_ = 40;
        applied_.clear(); // Old-life events are rejected before lookup.
    }
};

struct Vec3 { double x{}, y{}, z{}; };
Vec3 operator+(Vec3 a, Vec3 b) { return {a.x+b.x, a.y+b.y, a.z+b.z}; }
Vec3 operator*(Vec3 a, double s) { return {a.x*s, a.y*s, a.z*s}; }
struct Snapshot { Tick tick; Vec3 position; };
Vec3 Interpolate(const Snapshot& a, const Snapshot& b, double renderTick) {
    if (b.tick <= a.tick || !std::isfinite(renderTick))
        throw std::invalid_argument("ordered samples and finite time required");
    // Convert separately before subtracting: no unsigned subtraction underflow.
    const double alpha = std::clamp(
        (renderTick - static_cast<double>(a.tick)) /
        static_cast<double>(b.tick-a.tick), 0.0, 1.0);
    return a.position*(1.0-alpha) + b.position*alpha;
}
Vec3 ProjectileStep(Vec3 position, Vec3 velocity, Vec3 gravity, double dt) {
    return position + velocity*dt + gravity*(0.5*dt*dt);
}
bool Near(double a, double b) { return std::abs(a-b) < 1e-9; }

void WeaponLab() {
    Weapon weapon({}, 5);
    auto first = weapon.TryFire(0, 1);
    assert(first.firstProcessing && first.reply.decision == FireDecision::Accepted);
    assert(weapon.Magazine() == 2);
    auto duplicate = weapon.TryFire(1, 1);
    assert(!duplicate.firstProcessing && duplicate.reply.authoritativeTick == 0);
    assert(weapon.Magazine() == 2);
    assert(weapon.TryFire(1, 2).reply.decision == FireDecision::Cooldown);
    // A rejected request must not become accepted merely by being retried later.
    assert(weapon.TryFire(6, 2).reply.decision == FireDecision::Cooldown);
    assert(weapon.TryFire(6, 3).reply.decision == FireDecision::Accepted);
    assert(weapon.TryFire(12, 4).reply.decision == FireDecision::Accepted);
    assert(weapon.TryFire(18, 5).reply.decision == FireDecision::Empty);
    assert(weapon.BeginReload(18));
    assert(weapon.TryFire(20, 6).reply.decision == FireDecision::Busy);
    weapon.Update(137);
    assert(weapon.Magazine() == 0);
    weapon.Update(138);
    weapon.Update(138);
    assert(weapon.Magazine() == 3 && weapon.Reserve() == 2);
    assert(weapon.TryFire(138, 7).reply.decision == FireDecision::Accepted);
    assert(weapon.BeginReload(139));
    weapon.CancelReload();
    weapon.Update(1000);
    assert(weapon.Magazine() == 2 && weapon.Reserve() == 2);
    weapon.Disable();
    assert(weapon.TryFire(1000, 8).reply.decision == FireDecision::Busy);
}
void CombatLab() {
    CombatTarget target;
    assert(target.Apply(1, 1, 30));
    assert(target.Hp() == 100 && target.Armor() == 10);
    assert(!target.Apply(1, 1, 30));
    assert(target.Apply(2, 1, 50));
    assert(target.Hp() == 60 && target.Armor() == 0);
    assert(target.Apply(3, 1, 100));
    assert(!target.Apply(4, 1, 100));
    assert(target.DeathTransitions() == 1);
    target.Respawn();
    assert(target.Life() == 2 && target.Hp() == 100);
    assert(!target.Apply(99, 1, 500));
    assert(target.Apply(5, 2, 140));
    assert(target.DeathTransitions() == 2);
}
void TimingLab() {
    Snapshot a{100, {0,0,0}}, b{106, {6,0,0}};
    assert(Near(Interpolate(a,b,103).x, 3));
    assert(Near(Interpolate(a,b,99).x, 0));
    assert(Near(Interpolate(a,b,200).x, 6));
    bool rejected = false;
    try { (void)Interpolate(a,a,100); }
    catch (const std::invalid_argument&) { rejected = true; }
    assert(rejected);
    Vec3 p{0,0,0}, v{600,0,0}, g{0,0,-9.81};
    Vec3 next = ProjectileStep(p,v,g,TickSeconds);
    assert(Near(next.x,10));
    assert(next.z < 0);
    assert(!SameHandle({7,1}, {7,2}));
}
} // namespace arena
int main() {
    arena::WeaponLab();
    arena::CombatLab();
    arena::TimingLab();
    std::cout << "Arena labs: weapon, combat, timing assertions passed.\n";
}
