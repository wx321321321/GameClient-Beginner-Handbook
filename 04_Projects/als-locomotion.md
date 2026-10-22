# 4.16 ALS 高级运动系统实践

<!-- course-navigation:start -->
[上一节：4.15 AOI 兴趣区域管理](aoi.md) · [本章目录](README.md) · [下一节：4.17 QQ 飞车：动画演出与换色系统](qq-speed-animation-and-ui.md) · [手册首页](../README.md)
<!-- course-navigation:end -->

先读 [3.3 动画数据流](../03_UE/animation-overview.md)和 [4.6 表现系统](shooter-presentation.md)。本篇保留我做过的 ALS 重构与未解决问题，适合在基础射击闭环跑通后深入；攀爬、预测 IK 和 Warping 不必在第一版同时加入。

我以 UE 商城的 Advanced Locomotion System（ALS，高级运动系统）示例为基础做了 C++ 重构。原项目是 UE4 的蓝图运动系统，涵盖多方向走跑、冲刺、跳跃等；我的目标是把大部分逻辑移到代码侧，再尝试 UE5 的动画技术，理解角色运动与姿势表现怎样协作。

这个项目是在 QQ 飞车实习期间利用空闲时间做的。后来在网易项目里，我看到了熟悉的运动结构和仍叫 ALS 的文件夹。这是我的工程观察，能作为对照学习的线索，但仅凭命名不能证明对方整个系统直接复制了原项目。

以下分开记录**我做过的实现**、**复盘时提出的改进**和**UE 通用机制**。`MyCharacter`、`MyAnimInstance`、`UAlsCameraComponent`、`mantlesource` 等是原笔记的项目名称或示意名，不承诺与某个公开 ALS 分支完全一致；本仓库没有对应工程源码。UE5 功能的具体接口需按使用版本核对。

## 4.16.1 角色、动画和移动各自决定什么

角色保存事实状态，动画实例把状态变成动画参数，AnimGraph 输出姿势。胶囊体的真实移动与网格骨骼的表现要分开，否则一旦滑步、IK 抖动或相机跳变，很难定位责任。

### 4.16.1.1 Character 保存角色状态

自定义 `MyCharacter : ACharacter` 处理输入、移动模式、步态、姿态和视角，协调胶囊体、MovementComponent 与相机。

```cpp
EGait DesiredGait;             // 期望走、跑、冲刺
EGait ActualGait;              // 当前实际步态
EStance Stance;                // 站立、蹲伏
ERotationMode RotationMode;    // 朝移动方向、朝视角方向
EViewMode ViewMode;            // 第一/第三人称
FRotator AimingRotation;       // 当前瞄准方向
bool bHasMovementInput;        // 是否有移动输入
```

`DesiredGait` 与 `ActualGait` 分开，意味着输入想冲刺不代表当前速度和状态已经允许进入冲刺表现。

### 4.16.1.2 AnimInstance 计算表现参数

`MyAnimInstance : UAnimInstance` 读取角色事实，生成图求值需要的参数：

```cpp
float Speed;
float Direction;
float AimYaw;
float AimPitch;
float StrideBlend;
float WalkRunBlend;
float LeanAmount;
```

```cpp
// 逻辑示意，函数所属库和调用形式按项目版本调整。
Speed = Character->GetVelocity().Size2D();
bIsInAir = Character->GetCharacterMovement()->IsFalling();
Direction = CalculateDirection(
    Character->GetVelocity(),
    Character->GetActorRotation()
);
```

动画蓝图继承相应实例类型，运行时由组件创建实例，再通过状态机、BlendSpace 和混合节点输出：

```text
FCompactPose       本次求值需要的局部骨骼 Transform
FBlendedCurve      动画曲线
CustomAttributes   自定义动画属性（具体容器类型随版本变化）
```

原笔记写“AnimInstance 每帧一定在 Character Tick 后”，更准确的说法是**数据依赖需要正确安排**。Tick 组、先决条件、并行动画和根运动会影响实际执行，不能只按类名推断顺序。

### 4.16.1.3 Compact Pose 与 CPU/GPU 的边界

当前 Mesh 和 LOD 决定需要哪些骨骼，`FBoneContainer` 维护相关映射：

```text
FCompactPoseBoneIndex
↔ MeshPoseBoneIndex
↔ SkeletonPoseBoneIndex
```

Compact Pose 用紧凑数组保存当前所需姿势，仍需逐骨骼采样、混合或组合。它的意义是减少不必要的骨骼计算并改善数据访问，而不是“不再计算骨骼”。Cached Pose 可复用图中的结果；活动分支、LOD 和更新策略共同决定工作量。

CPU 计算骨骼姿势和蒙皮所需变换，逐顶点蒙皮通常交给 GPU。单次绘制必须等待所需数据就绪，但 CPU 与 GPU 整体是流水并行，不是整帧全部动画计算结束后 GPU 才统一开始。

![ALS 项目结构原笔记图](assets/image-5.png)

## 4.16.2 相机：用动画曲线描述机位，再用几何处理碰撞

我的项目为相机创建独立的 SkeletalMesh 和 Camera AnimInstance。它读取角色运动状态、步态、视角模式和特殊动作，在相机动画图中混合出曲线，再由相机组件计算最终机位。

```text
角色状态
→ Camera AnimInstance / 相机状态机
→ CameraOffset_X/Y/Z、FOV、PivotLagSpeed 等曲线
→ UAlsCameraComponent 读取混合后的曲线
→ 计算 Pivot、局部偏移、旋转和理想相机位置
→ 碰撞修正
→ 最终位置、朝向与 FOV
```

部分曲线来自动画资源，部分由 Modify Curve 节点修改。状态过渡同时混合曲线，策划可以用动画资源调手感，减少硬编码参数。独立骨架也避免相机直接继承角色每一处骨骼抖动，但它最终读哪个 Pivot 仍很重要：若 Pivot 直接跟随剧烈变化的 pelvis，抖动仍会传进来。

### 4.16.2.1 碰撞与复杂环境下的推拉

从角色附近的安全起点向理想相机位置做球体 Sweep，碰到阻挡时将相机放在允许的位置，并留安全间隙。原稿说角色插槽“肯定不在墙里”过于绝对：胶囊体不穿墙，不保证所有动画骨骼或插槽都不穿墙，因此起点选择与初始重叠也要处理。

Sweep 半径可以参考近裁剪面的覆盖范围，实际通常使用可调安全半径，在防穿模与镜头过度贴近之间取舍。

我记录的三种抖动处理方向是：

1. 用 Collision Channel 区分阻挡。墙壁、地板、大岩石通常阻挡 Camera；树叶、草、铁丝网、小型物体及其他角色是否忽略，应按玩法配置，不能全部一刀切。
2. 非对称平滑。需要拉近以避障时快速响应；障碍消失后向外恢复较慢，可用半衰期控制回弹。
3. 对适合的特殊遮挡物做淡出。保持相机位置，使用抖动掩码或支持透明的材质参数改变遮挡表现；直接改 Opacity 只有相应材质支持时才有效。

### 4.16.2.2 隐藏计算网格的更新与代价

我用下面的设置保证隐藏相机网格仍更新姿势和骨骼：

```cpp
CameraMesh->VisibilityBasedAnimTickOption =
    EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
```

若相机网格依赖角色网格结果，原笔记记录了 Tick 先决条件：

```cpp
CameraMesh->AddTickPrerequisiteComponent(Character->GetMesh());
```

这只是安排组件 Tick 的依赖，还需确认曲线在哪个阶段求值和读取，避免动画任务尚未完成时读旧值。它也不是“设置了就没有任何性能成本”。额外 SkeletalMesh 意味着额外 Tick 和动画图求值。

原实现的精简方向是隐藏渲染（`bHiddenInGame = true`）、关闭碰撞 `SetCollisionEnabled(ECollisionEnabled::NoCollision)`、关闭物理、Cloth 和 Generate Overlap Events，仅保留计算所需能力。Camera AnimInstance 负责产生曲线，`UAlsCameraComponent` 负责几何、碰撞、肩膀和第一/第三人称切换，职责分开后更容易排查。

## 4.16.3 地面运动：先匹配方向，再匹配速度与脚步阶段

### 4.16.3.1 资产和参数

我使用前、后、左前、左后、右前、右后六组方向资产，覆盖多方向移动。走跑混合还使用 `WalkRunBlend`、`StrideBlendAmount` 与 Play Rate：

- `WalkRunBlend` 在原方案中根据 gait 选择走/跑侧，近似 0/1。
- `StrideBlendAmount` 根据当前速度与走跑标定速度求值，控制步幅相关混合。
- Play Rate 根据角色速度、动画标定速度及步幅修正计算；具体顺序和公式需按图及代码确认，不能只用两个未经定义的数相除。

资源标定走 200、跑 400，而实际配置改成走 300、跑 500 时，仅调播放速率容易让上半身摆动不自然，这也是后面引入 Stride Warping 的原因。冲刺另有常态循环与起步爆发资产；倾斜使用二维 BlendSpace。

### 4.16.3.2 六方向解决交叉腿过渡

向左移动既可能使用朝左前迈的步态，也可能使用朝右、向左后退的步态。若前进直接混到“左后退”，两腿插值可能交叉。因此我把过渡拆成“前 → 左前 → 左后 → 后”，右侧同理。

`feetcrossing` 曲线标记适合过渡的脚步阶段。动画实例读取曲线，状态过渡根据它判断时机，避免在双腿位置最容易冲突时硬切。曲线值的具体 0/1 语义需要和资源约定一致。

### 4.16.3.3 冲刺、起步与倾斜各自用什么输入

冲刺起步用真实物理加速度映射一个混合权重：开始时较大，进入匀速后下降，再混合爆发动作与循环动作，角色 tag 决定是否使用冲刺分支。

起步倾斜可由 `(当前速度 - 上帧速度) / Δt` 得到加速度，变换到角色局部空间，再采样方向性的加法姿势。持续运动中的倾斜则可由方向、输入等产生常态偏移。原笔记将两者连写，这里区分为**加速时的瞬态 Lean**和**运动中的方向性 Lean**。

Character 更新状态后从 setting 读取参数并设置 CMC 的 `MaxWalkSpeed`。它限制速度上限；达到或离开该速度的过程仍由加速度、制动减速度和摩擦等控制。

### 4.16.3.4 原地转向：动画负责挪脚，代码负责朝向

我使用左/右、小角度/大角度四个动作，通过旋转曲线与 `AddActorWorldRotation` 等操作配合，允许按实际角度调整转向。

原记录的触发条件包括：视角转速满足限制，角色朝向与控制器朝向的 Yaw 差超过 45°。把目标角度 45～180° 映射成 0.75～0 秒等待，计时到达后执行 Turn In Place。运动中转向则可用半衰期插值，小于阈值直接对齐。

这里必须分清旋转曲线的单位：

- 若存**累计角度**，本帧应使用 `Curve(t_now) - Curve(t_prev)`，再按目标角度与资源标定角度（90°/180°）缩放。
- 若存**角速度**，则乘本帧 Δt。
- 若存**每个烘焙帧的增量**，才可能需要默认动画帧率与游戏帧率换算。

原稿的“曲线值 ÷ 游戏帧率 × 默认动画帧率”不能无条件用于累计曲线。加速播放会增大采样间隔，粗糙累计更容易丢误差；先确认曲线定义，比单纯限制播放速度更关键。

## 4.16.4 Foot IK：让姿势适应地面，但不替代胶囊体移动

FK 从父骨旋转求出脚的位置；IK 给出脚目标，再求大腿、小腿等关节。角色的碰撞与真实移动仍由胶囊体处理：

```text
Character Actor
└─ CapsuleComponent       碰撞与真实移动
   └─ SkeletalMesh
      └─ root
         └─ pelvis        姿势内的骨盆修正
```

上下台阶时，胶囊体可能把整个人抬高，一脚踩在高处、另一脚悬空。基本处理链是：检测左右脚下地面 → 选骨盆偏移使双腿尽量可达 → 用 Two Bone IK 解算腿 → 按地面法线调整脚踝。主要移动的是姿势中的 pelvis 或项目规定的 Mesh 偏移，不应随意下移真实碰撞胶囊体。

脚踝旋转还需要保留合理朝向，不能只有一个法线就唯一决定完整旋转。骨盆偏移也不能永远只取“更低脚”：IK 权重、最大腿长和支撑阶段都要参与判断。

### 4.16.4.1 虚拟骨、IK 骨与求解器

虚拟骨记录一个源骨空间下跟随目标骨的变换。例如源为 Pelvis、目标为 Foot_L，可以表达脚相对骨盆的关系。它不是只存一个相对位置向量，也不需要动画师额外为每个资源逐帧手工制作。[UE Virtual Bones](https://dev.epicgames.com/documentation/en-us/unreal-engine/virtual-bones-in-unreal-engine)

IK 骨是骨架中的辅助骨骼，常放在 root 附近的独立层级并烘焙目标运动。原笔记的持枪例子是把 `ik_hand_l` 烘焙成适合握枪的目标，使上肢修正有稳定参照；并非所有 IK 骨都必须直接以 Root 为父，是否有关键帧也取决于资源制作。

![虚拟骨与 IK 骨原笔记图](assets/image-6.png)

- Two Bone IK 利用两段长度和目标距离求关节，余弦定理是基础。
- FABRIK 适合较长链，通过末端到根、根到末端的来回调整逐步接近目标。
- Full Body IK 从指定 Root 与多个 Effector 联合解算，更适合相互影响的全身约束。

```text
Effector 常见信息：
目标骨骼、目标位置、目标旋转
位置/旋转权重及其他影响参数
```

Two Bone IK 还需要确定膝盖弯曲平面。我的 Pole Vector 做法：A 为大腿根、B 为膝盖、C 为脚踝，将 B 投影到 AC 得 P，以 `B-P` 为膝盖方向，再从 B 沿该方向延伸 **40 cm** 得目标。腿接近完全伸直时这个方向不稳定，需保留上一帧方向或备用方向；40 cm 是我的参数，不是求解器固定要求。

### 4.16.4.2 我遇到的实时检测抖动

最初持续更新 Foot IK，用曲线控制左右脚权重：支撑期高，摆动期降低或关闭，骨盆更参考支撑脚。但脚跨过多个台阶时，检测高度会突然变化，pelvis 跟着跳，相机若依赖它也会抖。

例如左脚从第 1 级跨到第 5 级，另一脚位于第 3 级，摆动脚下的射线可能先后扫过多级高度。若每帧都把地面命中点当最终脚目标，就会把地形的不连续直接传到姿势里。

平滑可以减轻抖动，却可能在上台阶时跟不上高度而穿模；这是需要联合设计的取舍，不是平滑与防穿模在数学上必然互斥。我复盘时提出过“目标 = 地面高度 + 原动画脚部抬高量”，用来保留摆动轨迹。这是改进想法，不代表已完整验证。

## 4.16.5 预测 IK：提前决定落点与跨越路径

我进一步尝试用动画相位预测落点，避免摆动期完全追随脚下实时地形。用到三组曲线：

1. 距离目标落脚时刻的剩余时间。
2. 摆动相/锁脚曲线：原约定 0 表示锁定、1 表示摆动或准备抬脚。
3. 左右脚落地时相对 Root 的位置。

原资源里还观察到“左脚开锁附近，右脚脚尖离地”等相位关系；这是该组动画的特征，不能移植成所有步态规律。

### 4.16.5.1 落点和路径计算

```text
预测落点 ≈ 当前 Root 世界位置
         + 当前速度 × 剩余时间
         + 按角色朝向转换后的落脚相对偏移
```

相对偏移的 X、Y 分量分别乘 forward/right，转换到世界空间；Z 的解释要与曲线生成空间保持一致。用预测落点做地面检测，取得高度、法线，再计算脚部和骨盆的目标偏移。

起步时保存当前脚位置与预测落点。从两端向中间做检测，收集跨越障碍的关键点到数组；摆动过程中按相位插值出需要额外抬脚的高度。原稿没有给出检测采样密度、完整曲线构造和碰撞保证，因此它是一种项目路径方案，不能称为已证明全程无穿模。

骨盆用弹簧插值调整，脚根据摆动相在路径跟随和锁定之间切换。这里“每帧更新”不等于“每帧重新贴地”：锁脚时应保持约定的支撑目标，摆动时保留计划路径。

### 4.16.5.2 曲线生成器与空间错误

我为循环动画写过剩余时间曲线生成器。以某只脚开始锁地的帧为周期参照，对下一次目标落地时刻计算剩余时间，再处理循环边界。读取时还必须结合 Play Rate；资源时间与真实时间不总相等。

最初用 `foot_r` 获取脚高度时发现数值几乎不变，换 IK 骨后才得到预期结果。原稿把原因记成“组件空间相对父骨”，这个术语有误：**相对父骨是局部骨空间，组件空间相对 SkeletalMeshComponent**。真正要核对的是当时 API 返回哪种空间、是否沿父链转换，以及 IK 骨为何恰好更符合采样需求，不能仅归因于骨名。

### 4.16.5.3 转向后预测怎样失效

我记录了两条处理方向：

- 保存预测时朝向，比较当前 forward 的偏角和角速度。突变较大时暂时不预测；可以修复时，保留当前状态，重算落点与路径，再平滑切换。
- 每帧重算候选落点，与起步时的预测点比较。若 XY 距离偏差超过 **15 cm**，标脏并重建路径数组。

15 cm 是当时的工程阈值。两种方法共同解决“旧预测已不适用”，不能在重建时把脚瞬移到新轨迹；移动平台、速度突变也应成为后续核对场景。

## 4.16.6 Stride Warping：改步幅，保留合理播放节奏

一段动画标定速度 200，实际运动速度变化很大时，单独提高 Play Rate 会连上半身摆动一起加速。我尝试把小范围差异交给步幅修正，大范围变化仍配合 Play Rate，并对 StrideScale 限制、平滑。

### 4.16.6.1 我的 Control Rig 小模块

为理解原理，我在 Control Rig 输入缩放倍数和前向向量，按脚步相位决定是否调整某只脚，再将计算结果写到对应虚拟骨。

若脚目标相对参考点的向量为 r、前向单位向量为 d、缩放为 s，只缩放前向分量可写成：

```text
r' = r + (s - 1) * dot(r, d) * d
```

原稿“算投影再加到骨骼上”缺少 `(s-1)`，会在比例为 1 时仍额外移动脚。源动画的 `ik_foot` 需要有与预期运动一致的数据，否则目标并不代表真实步幅。

### 4.16.6.2 引擎节点还处理腿长和骨盆

常见比例为：

```text
StrideScale = 角色移动速度 / 动画 Root Motion 速度
例如：450 / 300 = 1.5
```

要处理分母接近零和过大比例。节点根据 `StrideDirection` 调整 IK 脚目标，并结合 Pelvis、Thigh、FK Foot、IK Foot 等定义限制腿长，必要时调整骨盆；后续 Leg IK 再把肢体解到目标。[UE Pose Warping](https://dev.epicgames.com/documentation/en-us/unreal-engine/pose-warping-in-unreal-engine)

原笔记用迭代描述 pelvis solver：根据原腿长或目标距离，为左右腿各估计一个可行骨盆位置，加权后再迭代，误差足够小或达到次数上限停止。这个思路说明了为什么不能只改脚位置，但具体 UE 节点还含约束与参数，不能把“两个候选点取平均”当成精确源码复述。

输入中的 `LocomotionAngle` 与 `StrideDirection` 也要分清：不同 Warping 节点使用不同方向参数，不应仅按名字互换。

## 4.16.7 Distance Matching：让动画时间跟随实际距离

例如起步动画本帧预期前进 0.4 m，胶囊体实际前进 0.5 m，如果继续按固定播放速度，脚的支撑关系会错开。Distance Matching 用距离曲线反查动画时间，让姿势推进量贴合实际运动。

它与 Stride Warping 互补：前者改采样时间，后者改姿势空间里的步幅。曲线可以由带 Root Motion 的源动画生成，但运行时不一定再让该 Root Motion 同时驱动胶囊体，否则要防止重复应用位移。[UE Distance Matching](https://dev.epicgames.com/documentation/en-us/unreal-engine/distance-matching-in-unreal-engine)

### 4.16.7.1 两类节点对应不同距离语义

| 节点 | 输入含义 | 常见用途 |
| --- | --- | --- |
| `Distance Match To Target` | 距离目标还剩多少，配合 `DistanceToTarget`、`DistanceCurveName`、Sequence Evaluator | 停步、落地等具有明确目标的片段 |
| `Advance Time By Distance Matching` | 上次到现在实际走了多少，配合 `UpdateContext`、`DistanceTraveled`、`DistanceCurveName`、Sequence Evaluator | 起步、循环及连续移动的过渡段 |

起步时可以在速度从 0 变为正的瞬间保存 Actor Location；但增量推进节点需要的是本次更新位移，不应每帧重复塞入“从起点累计的总距离”。循环动画还需正确处理曲线/时间跨周期。

### 4.16.7.2 停步预测与曲线制作

`Predict Ground Movement Stop Location` 是动画运动库的辅助函数，用当前速度及 CMC 的摩擦、制动参数估算停点。它处理的是**停步**，原稿将其放在“起步剩余距离”下不准确；`v²/(2a)` 只适用于恒定减速度的简化模型，不能替代完整 CMC 参数。[官方函数说明](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Plugins/AnimationLocomotionLibraryRuntim-/UAnimCharacterMovementLibrary/PredictGroundMov-)

曲线生成要明确距离是到终点的有符号距离、从起点的位移，还是累计路径长度。原笔记“当前 Root 到初始 Root 的欧几里得距离”适用于某些单向运动，但转弯或回头会产生多个时间对应同一距离。遇到这种情况，应拆分阶段或加入方向/状态约束，单纯“再存一个变量”并不会让现成的一维查表节点自动解决歧义。

## 4.16.8 攀爬：检测目标，选择动作，再把真实移动交给 CMC

我的 Mantle 流程从跳跃输入进入 `trymantle`：

1. 根据当前速度优先选择检测方向，允许输入作一定修正；无速度时用角色朝向。用胶囊体 Sweep 检查前方障碍。
2. 结合障碍碰撞结果做高度与顶部检测，确认目标在可攀爬范围、顶部有足够站立空间。
3. 求目标落点、相对胶囊体底部的高度，以及攀爬类型。
4. 创建自定义 Root Motion Source，原笔记称 `mantlesource`，保存这次动作所需的数据。

原实现还有对齐目标后再次检测的步骤；实际应区分检测朝向与直接移动 Actor，避免检测尚未通过就不必要地改变角色状态。

### 4.16.8.1 攀爬前对齐

我当时在短暂 align 阶段直接 `SetActorRotation`，让抓边动作尽快稳定。缺点是角度差大时转向较硬。后续考虑按阈值分流：小差值直接设置，大差值做很短的快速插值。这是复盘优化方向，不描述成已经实现。

### 4.16.8.2 动画 Root Motion 与 Root Motion Source

动画 Root Motion 从动画根骨运动中提取位移；Root Motion Source 则向 `UCharacterMovementComponent` 提供程序生成的运动贡献。原笔记涉及 `USkeletalMeshComponent::TickAnimation()`、`UAnimInstance::ConsumeExtractedRootMotion()` 和 CMC 的 `TickComponent()` / `PerformMovement()`，可作为查源码的入口。

不应把它写成“上一帧 Mesh 必定先算好，本帧 CMC 再拿”：角色移动可能主动安排所需动画更新，线程和网络路径也会影响实际顺序。应追踪提取、消费、转换和应用的真实调用点，而不是根据缓存名称推断固有一帧延迟。

Root Motion Source 按优先级和 `AccumulateMode` 等规则参与运动，常见模式为 `Override`、`Additive`。来源之间以及与动画根运动如何交互，要看 CMC 实现，不能视为任意 Transform 无条件相加。[FRootMotionSource API](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/FRootMotionSource)

我的 `mantlesource` 原记录保存：

```text
Montage
MotionWarpingTimeRange
StartLocation / StartRotation
TargetLocation / TargetRotation
MontageStartTime
duration
```

这些是自定义源的数据设计，不是 `FRootMotionSource` 基类默认都带的字段。方案思路是从动作运动中取每段变化，再按时间分配起终点误差，生成交给 CMC 的运动；计算增量时要避免每帧重复叠加完整误差。

## 4.16.9 Motion Warping：在窗口内把动画根运动对齐目标

Motion Warping 处理已有动画根运动，在指定窗口内修正位移和旋转，使动作对齐场景目标。公开工作流常结合 Montage 和 Notify State；原稿“状态机混合必然导致引擎不知道改谁、结果崩溃”不成立，根运动本身可以按权重混合。选择 Montage 的实际好处是动作与时间窗口明确，组件能识别应修改的片段。[UE Motion Warping](https://dev.epicgames.com/documentation/unreal-engine/motion-warping-in-unreal-engine?lang=en-US)

### 4.16.9.1 三个对象的责任

| 对象 | 作用 |
| --- | --- |
| `UMotionWarpingComponent` | 管理命名 Warp Target，更新和维护 modifier |
| `UAnimNotifyState_MotionWarping` | 在动画中声明窗口及修改配置 |
| `URootMotionModifier` | 根据窗口、目标和剩余运动改写根运动增量 |

原笔记提到组件追踪 `FAnimMontageInstance` 的播放时间，这是相关工作流的源码线索，但版本扩展能力仍需核对。`ProcessRootMotionPreConvertToWorld` / `ProcessRootMotionPostConvertToWorld` 是阅读 CMC 根运动处理的重要入口；modifier 所处空间与绑定位置应以实现为准，不能把所有修改描述成“拿到世界向量后统一改写”。

概念链是：提取本次动画增量 → 消费/整理根运动 → 在规定转换阶段修正 → CMC 使用结果移动胶囊体。`FRootMotionMovementParams` 承载相关数据；Consume 会移走或清空已消费部分以避免重复应用，精确函数名依引擎版本和对象而定。

原稿的 5 cm 改成 8 cm 例子可保留为直觉：若目标要求在窗口结束前走更远，modifier 会重新分配剩余运动；不是无条件把每帧位移乘同一个系数。

### 4.16.9.2 我的接入步骤与类型笔记

先让攀爬蒙太奇具有可提取根运动，并把 Warping 窗口覆盖到需要对齐的动作阶段。播放前注册场景目标，窗口使用同名目标：

```cpp
MotionWarpingComponent->AddOrUpdateWarpTargetFromTransform("ClimbTop", Target);
PlayAnimMontage(MantleMontage);
```

常见公开配置中，Scale 按比例缩放根运动，Skew Warp 让窗口结束时的位置/旋转按配置对齐目标。原稿还记有 `Warp`、`_PrecomputedWarp`，这里保留为旧版、实验或项目扩展的核对关键词；没有源码版本证据，不能把它们列成当前 UE 的固定四分类，也不能保证某一种天然“更自然”。

窗口若只覆盖收手而不覆盖主要位移，难以有效匹配高度。通常需要源动画有根运动；某些版本/修改器对无位移片段有额外处理能力，也应单独核对，不能只凭插件名判断。

### 4.16.9.3 RMS、Warping 与我还没解决的问题

RMS 是提供运动来源的机制；Motion Warping 是修改动画运动使其对齐目标的机制。我的攀爬可以把目标修正写进自定义 Source，也可利用 Warping 的窗口和目标管理，但不能让两套修正重复施加相同误差。

原实践仍有一个表现问题：小平台能通过攀爬检测，但动画中某只手撑在障碍物外的空气。它说明“胶囊体能站上去、根运动能落准”还不足以保证双手接触合理。后续需要检查障碍宽度、双手支撑点和动作选择，必要时加入手部 IK；这里保留为未完成的改进点，而不是假装已经解决。
