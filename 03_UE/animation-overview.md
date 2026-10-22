# 3.3 UE 动画系统概览

<!-- course-navigation:start -->
[上一节：3.2 UE 运行时类型检查](runtime-type-checking.md) · [本章目录](README.md) · [下一节：3.4 UE 与弥赛亚动画系统对比](animation-system-comparison.md) · [手册首页](../README.md)
<!-- course-navigation:end -->

UE 骨骼动画系统把角色状态转换成最终骨骼姿势。先看一次更新的数据流，再把原笔记中的资源、运行时对象和节点放回各自的位置，就不会只记住一串类名。

```text
ACharacter / UCharacterMovementComponent 的运动状态
→ UAnimInstance 读取并整理动画参数
→ AnimGraph 更新状态、时间和混合权重
→ 采样资源，混合并修正骨骼姿势
→ USkeletalMeshComponent 接收姿势、曲线和属性
→ 生成蒙皮所需变换，交给渲染
```

这是一条逻辑依赖链，不表示所有阶段必须在同一线程依次完成。实际调度还受 Tick 依赖、并行动画、可见性与更新频率等设置影响。

## 3.3.1 资源与角色实例分别保存什么

| 对象 | 主要职责 |
| --- | --- |
| `USkeleton` | 共享的骨架与动画关联信息，提供骨骼层级等基础数据 |
| `USkeletalMesh` | 具体网格、蒙皮数据及网格自身的参考骨架 |
| `USkeletalMeshComponent` | 场景里的组件实例，持有当前角色的网格、动画实例和求值后的骨骼变换 |

同一套资源可以被多个角色使用，但每个角色有自己的运行时状态。骨骼编号如何在共享骨架、网格与当前姿势之间对应，见[UE 与弥赛亚动画系统对比](animation-system-comparison.md)。

动画资源按使用方式理解：

- `UAnimSequenceBase` 是序列类资源的基础；`UAnimSequence` 保存一段骨骼动画及相关曲线、通知。
- `UBlendSpace1D`、`UBlendSpace` 根据一个或两个输入参数，在多段动画之间采样和混合。
- `UAimOffsetBlendSpace`、`UAimOffsetBlendSpace1D` 用于瞄准偏移的混合，通常配合加法姿势。
- `UPoseAsset` 保存可复用的姿势数据。
- `UBlendProfile` 调整骨骼间的混合行为；它是混合配置，不是一段动作。
- `UAnimMontage` 组织可受游戏逻辑控制的动作播放、分段、Slot 和通知，例如攻击、换弹。

## 3.3.2 蓝图资产、运行实例与图求值

`UAnimBlueprint` 是编辑时的动画蓝图资产，编译后产生 `UAnimBlueprintGeneratedClass`。组件运行时创建相应的 `UAnimInstance`；它保存该角色当前的动画状态，并连接游戏逻辑与动画图。

`FAnimInstanceProxy` 承担动画更新、求值需要的一部分数据与接口，帮助组织游戏线程和动画工作线程之间的交接。它不是另一个角色，也不能使任意 UObject 访问自动变得线程安全。Event Graph 与线程安全更新函数的执行条件不同，多线程设计需按项目版本核对。[官方动画优化说明](https://dev.epicgames.com/documentation/en-us/unreal-engine/animation-optimization-in-unreal-engine)

AnimGraph 的运行时节点以 `FAnimNode_Base` 为基础。阅读一张图时，可以按职责追踪：

| 节点 | 在姿势链中的作用 |
| --- | --- |
| Sequence Player / Sequence Evaluator | 前者按播放时间推进，后者用于在指定时间采样姿势 |
| BlendSpace Player | 按输入参数采样 BlendSpace |
| State Machine | 选择状态并处理状态间过渡 |
| Apply Additive | 将加法姿势施加到基础姿势 |
| Layered Bone Blend | 按骨骼层级或遮罩混合不同姿势 |
| Slot | 让 Montage 等动作进入相应的图分支 |
| Inertialization | 在切换时根据先前运动信息衰减过渡差异 |
| Linked Anim Layer | 将可复用或可替换的动画层接入当前图 |

Update 决定时间、状态和权重；Evaluate 计算姿势。把两者分开，才能看懂“图更新了，但某些姿势并没有重新求值”等优化行为。

## 3.3.3 姿势之后的骨骼修正

先用一个数值例子理解“参数→权重→姿势”。假设速度单位为米/秒，走路样本速度为 2，跑步为 4，当前速度为 3：

```cpp
#include <algorithm>
float RunWeight(float speed) {
    return std::clamp((speed - 2.0f) / (4.0f - 2.0f), 0.0f, 1.0f);
}
// RunWeight(1) == 0；RunWeight(3) == 0.5；RunWeight(5) == 1。
```

`0.5` 表示这个简化两样本模型的混合权重，不是播放到动画的中间一帧。Update 计算权重与采样时间，Evaluate 才在相应时间采样、混合两份姿势。真实 BlendSpace 还涉及采样点布局、同步与插值设置，不能用这一个函数替代引擎实现。UE 默认长度单位是厘米，若输入来自 UE 速度，需要先统一单位。

### 3.3.3.1 工作线程为什么需要快照

```cpp
struct AnimSnapshot {
    float speedMetresPerSecond = 0;
    bool grounded = false;
};
struct AnimParameters { float runWeight; bool inAir; };
AnimParameters EvaluateParameters(AnimSnapshot snapshot) {
    return {RunWeight(snapshot.speedMetresPerSecond), !snapshot.grounded};
}
```

游戏线程把角色速度 3、着地状态 `true` 写入快照，工作线程拿到独立副本后得到 `{0.5, false}`。这里传值传递的是小块数据；工作线程不需要沿角色指针读取随时可能变化的组件。快照的发布、任务完成与缓冲区复用仍需要引擎调度或同步保证，普通结构体本身不会消除数据竞争。对应的纯 C++ 数据实验见 [animation-data-labs.cpp](../examples/course/animation-data-labs.cpp)。

骨骼控制节点对已有姿势做修正，常以 `FAnimNode_SkeletalControlBase` 为基础：

- `FAnimNode_ModifyBone` 修改指定骨骼变换。
- `FAnimNode_TwoBoneIK` 处理常见的两段肢体链。
- `FAnimNode_Fabrik` 对较长骨骼链进行迭代求解。

`FAnimNode_ModifyCurve` 修改动画曲线，应与修改骨骼的节点区分。曲线可以影响材质、表情、IK 权重或项目里的相机参数，不只是骨骼位移。

## 3.3.4 动画事件与真实移动

`UAnimNotify` 表示时间点通知，`UAnimNotifyState` 表示一段时间窗口。它们可触发脚步声、特效或游戏逻辑，但与 Montage 本身不是同一层概念：Montage 是播放组织方式，Notify 是资源时间线上的事件。

Root Motion 从动画中提取根骨运动，再按角色移动模式参与真实移动；Root Motion Source 则是 Character Movement 接收过程化运动的一种机制。两者不能仅因名字相近就当成同一个系统。`ACharacter` 与 `UCharacterMovementComponent` 负责胶囊体移动、碰撞和移动模式，骨骼动画主要负责姿势，使用 Root Motion 时两侧会形成更紧密的交互。

进一步处理程序化骨骼控制、IK 目标和跨骨架动画复用时，会接触 Control Rig、IK Rig、IK Retargeter。本文先建立系统入口；具体组合方式可接着看[ALS 实践](../04_Projects/als-locomotion.md)。

### 3.3.4.1 把“扣血”和“播命中动作”分开推演

服务器确认射击事件 18 后，战斗系统扣血，再发送表现事件。动画系统用事件 18 播放受击动作；如果同一事件重发，表现层可以按事件键去重。反过来，让每一次受击 Notify 都扣血，会把跳帧、动作打断和重复播放带入战斗结算。具体的事件键与预测回滚见[射击项目表现系统](../04_Projects/shooter-presentation.md)。

读完应能回答：速度为 3 为什么得到 0.5 权重？相同速度为什么不代表相同采样时间？为什么快照只含值仍需同步交接？关闭角色动画更新时，哪些战斗逻辑必须继续运行？
