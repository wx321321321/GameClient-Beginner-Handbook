#ifdef NDEBUG
#error "Teaching labs require assertions; compile without NDEBUG."
#endif

#include <algorithm>
#include <array>
#include <cassert>
#include <iostream>

struct AnimSnapshot { float speedMetresPerSecond; bool grounded; };
struct AnimParameters { float runWeight; bool inAir; };
float RunWeight(float speed) {
    return std::clamp((speed - 2.0f) / 2.0f, 0.0f, 1.0f);
}
AnimParameters EvaluateParameters(AnimSnapshot snapshot) {
    return {RunWeight(snapshot.speedMetresPerSecond), !snapshot.grounded};
}
int main() {
    assert(RunWeight(1) == 0 && RunWeight(3) == 0.5f && RunWeight(5) == 1);
    const auto parameters = EvaluateParameters({3, true});
    assert(parameters.runWeight == 0.5f && !parameters.inAir);
    constexpr std::array<int, 3> compactToMesh{0, 2, 5};
    constexpr std::array<int, 6> meshToCompact{0, -1, 1, -1, -1, 2};
    for (int i = 0; i < 3; ++i) assert(meshToCompact[compactToMesh[i]] == i);
    assert(meshToCompact[3] == -1);
    const std::array<int, 3> parent{-1, 0, 1};
    std::array<float, 3> local{10, 2, 3};
    std::array<float, 3> component{};
    const auto compose = [&] {
        for (int i = 0; i < 3; ++i) {
            assert(parent[i] < i && parent[i] >= -1);
            component[i] = parent[i] < 0 ? local[i] : component[parent[i]] + local[i];
        }
    };
    compose();
    assert(component[0] == 10 && component[1] == 12 && component[2] == 15);
    local[0] = 20;
    compose();
    assert(component[0] == 20 && component[1] == 22 && component[2] == 25);
    assert(local[1] == 2 && local[2] == 3);
    std::cout << "animation data checks passed\n";
}
