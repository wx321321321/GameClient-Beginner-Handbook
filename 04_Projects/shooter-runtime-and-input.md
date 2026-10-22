# 4.2 Arena 的运行循环、输入与角色移动

<!-- course-navigation:start -->
[上一节：4.1 从零规划 Arena 射击项目](shooter-scope-and-architecture.md) · [本章目录](README.md) · [下一节：4.3 武器状态、散布与弹道](shooter-weapons-and-ballistics.md) · [手册首页](../README.md)
<!-- course-navigation:end -->

本篇把“每帧处理一下”拆成可重复的模拟步骤。
输入采样、固定模拟、网络接收和渲染具有不同节奏，混在一起会让射速、移动速度和重放结果随帧率变化。
统一约定见[架构篇](shooter-scope-and-architecture.md)：米/秒、Z 向上、60 Hz 示例模拟、64 位逻辑序号。

## 4.2.1 一帧不是一个服务器 Tick

渲染帧负责给玩家显示当前结果，可以是 30、60 或 144 FPS。
模拟 Tick 推进碰撞、武器计时和生命状态，示例每步 `1/60 s`。
快照发送可以是另一频率；例如每三个 Tick 发送一次，并不让服务器逻辑降成 20 Hz。

```text
设备事件 → 输入采样缓存
网络包   → 已验证消息队列
              ↓
固定模拟 Tick
  消费本 Tick 输入 → 移动 → 行为 → 碰撞/伤害 → 产出状态与事件
              ↓
渲染帧读取稳定快照 → 插值/动画/相机 → 提交渲染
```

上述先后是 Arena 的设计选择。
若改成“先开火、再移动”，同一 Tick 的枪口位置就会改变，必须全端采用同一契约。
不要让哪个组件碰巧先 Tick 来决定规则。

## 4.2.2 固定步长需要处理积压

以下 C++17 核心可编译为对象文件；`simulate` 与 `render` 由宿主提供。
它展示有上限的客户端固定步长驱动，不是完整服务端调度器。

```cpp
#include <algorithm>
#include <cstdint>
#include <functional>

struct FixedLoop {
    double accumulator = 0.0;
    std::uint64_t tick = 0;
    static constexpr double dt = 1.0 / 60.0;

    void Frame(double elapsed,
               const std::function<void(std::uint64_t, double)>& simulate,
               const std::function<void(double)>& render) {
        accumulator += std::clamp(elapsed, 0.0, 0.25);
        int steps = 0;
        while (accumulator >= dt && steps < 8) {
            simulate(tick++, dt);
            accumulator -= dt;
            ++steps;
        }
        // 积压尚未清完时不外推；宿主必须记录并处理 overload。
        render(std::clamp(accumulator / dt, 0.0, 1.0));
    }
};
```

累计 25 ms 时可执行一个 Tick，剩余约 8.33 ms 用于渲染插值。
如果某次卡顿产生大量积压，无限制补帧可能让计算永远追不上。
这里每帧最多补八步，但没有偷偷增加单步 dt，也没有把未模拟的时间标成已经完成。

客户端可以按产品规则舍弃过期输入、重置预测并申请快照。
权威服务器不能简单删除积压然后声称时间一致；应监控超时、降低可降级工作、限流或结束无法维持的会话。
上面的 0.25 s 夹取只是本地暂停/异常帧保护，不能直接移到服务器作为时间裁定规则。
固定步长也不自动保证跨平台浮点确定性。

## 4.2.3 输入层先输出意图，再接触角色

把设备细节限制在 InputAdapter：鼠标、键盘、手柄都转换为相同的动作。
改键改变设备到动作的映射，不改变武器系统接收的命令类型。
UI 消费输入后，战斗层就不应再收到同一次攻击按下。

```cpp
#include <cstdint>
struct InputCommand {
    std::uint64_t commandSeq;
    std::uint64_t clientTick;
    float moveForward, moveRight; // [-1,1]
    float yawDelta, pitchDelta;   // 弧度，分配到本命令区间的增量
    bool fireHeld;
    bool firePressed;
    bool reloadPressed;
    bool jumpPressed;
};
```

`fireHeld` 表示持续状态，`firePressed` 表示一次边沿。
半自动枪消费按下边沿，全自动枪根据持有状态和武器计时决定何时请求下一发。
“按下后很快松开”即使发生在两次渲染之间，也不能被只读取最终按键状态的方式吞掉。

鼠标 delta 是一个采样区间内的总量。
若一帧补四个 Tick，不能给四次模拟都重复完整 delta，否则转动角度会变四倍。
可以按时间戳分配事件到命令区间，或者将累计增量按明确规则消费一次。
不同设备的灵敏度、加速和死区处理应在映射层说明。

## 4.2.4 命令流的生命周期

每个本地命令有递增 `CommandSeq`，提交后保存到有限预测历史。
服务器按受控时序处理命令，并在快照中确认 `lastProcessedCommand`。
客户端只丢弃已确认历史，未确认部分还要用于重放。

```text
Captured → Queued → Predicted → Sent
                         ↓       ↓
                  ServerProcessed / Rejected
                         ↓
                    Acknowledged → Retired
```

一个输入命令可能没有子弹，也可能触发武器调度。
`CommandSeq` 不能替代 `ShotSeq`：自动射击频率、单次输入丢包和重复发送都与命令频率不同。
首次版本可让输入命令每 Tick 一条，并限制每 Tick 每武器最多接受一发；更高射速要明确子步规则。

失焦、切菜单、断线时必须生成明确的“清除持有状态”处理，避免玩家未按键仍持续开火。

## 4.2.5 用实体句柄阻止旧任务访问复用对象

裸指针可能在子弹延迟命中前已经失效。
只用数组下标也不够，因为同一槽位可能已被新实体复用。
句柄必须同时验证槽位和 generation。

```cpp
#include <cstdint>
#include <optional>
#include <vector>
struct EntityHandle { std::uint32_t slot, generation; };
struct Entity { int hp = 100; };
struct Slot {
    std::uint32_t generation = 1;
    std::optional<Entity> value;
};
Entity* Resolve(std::vector<Slot>& slots, EntityHandle h) {
    if (h.slot >= slots.size()) return nullptr;
    Slot& s = slots[h.slot];
    if (s.generation != h.generation || !s.value) return nullptr;
    return &*s.value;
}
```

销毁时先取消所属任务或使其失效，再清空对象并增加 generation。
返回的指针只是当前受保护阶段的借用，不能越过并发销毁继续持有。
generation 回绕、地图切换和进程重启还需要作用域策略，不能说“加版本号后永远安全”。
复活若复用同一实体对象，另加 `lifeId`；它和内存槽位 generation 解决的是不同问题。

## 4.2.6 角色移动：用形状扫掠，不只改 Transform

输入先转换为期望速度或加速度，再交给运动模块。
斜向输入要限制长度，避免同时按前和右比直走更快。
速度变化、重力与碰撞共同决定本 Tick 最终位移。

```text
输入轴 → 归一/限幅 → 转到移动参考方向
→ 加速或制动 → 施加重力
→ 胶囊体 Sweep
→ 沿碰撞法线去掉进入表面的位移分量
→ 在有限迭代内继续剩余移动
→ 地面检查、台阶与坡度规则
→ 输出位置、速度、isGrounded
```

滑动的几何直觉是 `remaining -= normal * dot(remaining, normal)`，只在进入表面时移除该分量。
真正角色控制器还要处理接触偏移、边缘、多面碰撞、初始穿透和移动平台。
教学项目优先使用引擎已提供的控制器，不能靠这一个公式宣称实现了稳健 CMC。

### 4.2.6.1 三种空间要明确

移动朝向可以取相机水平朝向，身体朝向可能渐变追随，瞄准方向则由视线控制。
它们可以不同，但服务器必须知道哪一个参与加速、哪一个参与射击。
动画只读取最终速度与模式，不反过来每帧任意写胶囊体位置。
使用 Root Motion 的动作要建立单独移动模式，见[表现篇](shooter-presentation.md)。

## 4.2.7 失败边界与观测值

| 症状 | 首先检查 |
| --- | --- |
| 144 FPS 跑得比 30 FPS 快 | 位移是否乘 dt，固定步长是否被重复推进 |
| 一次点击发了两发 | 按下边沿是否重复消费，ShotSeq 是否复用 |
| 斜走更快 | 输入向量是否限幅 |
| 卡一帧后转头突然放大 | 完整鼠标增量是否被多个补帧重复应用 |
| 重连后角色继续走 | held 状态和旧命令是否跨会话残留 |
| 异步任务伤害了新对象 | 是否只保存槽位、缺 generation/lifeId |

记录每 Tick 的命令序号、模拟 dt、补步次数、积压秒数及碰撞迭代次数。
只看平均 FPS 无法发现一次 200 ms 的模拟积压。
服务器收到异常输入应夹取或拒绝并计数，不能让 NaN 坐标进入碰撞世界。

## 4.2.8 本篇验收与练习

在 30、60、144 FPS 的宿主驱动下，运行相同十秒命令，比较最终位置与射击请求数。
连续开关菜单，确认没有粘住的移动或开火状态。
销毁实体并复用槽位，旧句柄必须解析失败。
在窄门、斜坡、台阶、墙角和移动平台分别记录碰撞结果。

练习：给命令历史设置容量上限，设计“确认长期不来”的处理。
答案不能只是覆盖最老命令；覆盖后仍尝试重放会得到无法解释的位置。
下一篇把这个稳定 Tick 和命令入口接到武器状态机。
