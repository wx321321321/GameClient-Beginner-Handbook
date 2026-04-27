# USkeletalMeshComponent
UE 里用于承载和驱动骨骼网格的组件。它不只是显示模型，还负责把 Skeletal Mesh 实例化出来，结合动画蓝图或动画资源更新骨骼姿态，完成蒙皮渲染，并且可以和物理、碰撞、布料以及 LOD/动画预算优化一起工作

### FAnimationEvaluationContext：一次动画评估（evaluation）过程中，所有中间数据的容器
#### 目标：这次是在给谁算
```
AnimInstance
PostProcessAnimInstance
SkeletalMesh
```
#### 骨骼变换缓存：
当前帧骨骼结果
缓存副本
根骨平移
#### 曲线 / 属性数据
```
Curve
CachedCurve
CustomAttributes
CachedCustomAttributes
```
#### 本次评估控制开关
```
bDoInterpolation 这帧要不要插值
bDoEvaluation 要不要真正做求值
bDuplicateToCacheBones 要不要把结果复制到 cache
bDuplicateToCacheCurve
bDuplicateToCachedAttributes
bForceRefPose 要不要强制参考姿态
```
## USkeletalMeshComponent
### 组成：
#### 1.资源与动画实例管理：
```
USkeletalMesh* SkeletalMeshAsset; 当前显示的骨骼网格资源。
TSubclassOf<UAnimInstance> AnimClass;给角色指定的 AnimBP class。
UAnimInstance* AnimScriptInstance;真正运行中的主动画实例。这是运行时对象，不是类。
UAnimInstance* PostProcessAnimInstance;后处理动画实例。主动画算完以后，再来一层后处理。
TSubclassOf<UAnimInstance> OverridePostProcessAnimBP;
EAnimationMode::Type AnimationMode; 决定谁驱动当前姿态：
FSingleAnimationPlayData AnimationData;单节点播放模式时用的播放数据。
```
#### 2.动画求值结果缓存
```
TArray<FTransform> BoneSpaceTransforms;每根骨骼的局部空间变换。
FVector RootBoneTranslation;根运动
FBlendedHeapCurve AnimCurves; 这一帧动画曲线结果。
TArray<FTransform> CachedBoneSpaceTransforms;;每根骨骼的局部空间变换缓存。
TArray<FTransform> CachedComponentSpaceTransforms;每根骨骼的组件空间变换缓存、这是更接近最终用于蒙皮/物理的数据。
FBlendedHeapCurve CachedCurve;
UE::Anim::FMeshAttributeContainer CachedAttributes;
UE::Anim::FMeshAttributeContainer CustomAttributes;这一帧自定义骨骼属性结果
UE::Anim::FMeshAttributeContainer AttributesArray[2];
FAnimationEvaluationContext AnimEvaluationContext;并行动画求值时的核心上下文
```

#### 3.动画更新主链路函数
```
InitAnim 创建 / 重建 AnimScriptInstance 创建后处理实例 初始化动画树 广播 OnAnimInitialized
TickAnimation 动画系统的上层 Tick 入口。调用各个 anim instance 的 tick/update
TickAnimInstances 主实例linked instance post process instance的更新调用
TickPose 开始推进这一帧骨骼姿态
RefreshBoneTransforms 把本帧动画求值得到的结果，正式刷新到组件自己的骨骼缓存里。
DispatchParallelEvaluationTasks  如果当前允许并行，就把动画 update/evaluate 扔到任务系统。
ParallelAnimationEvaluation 真正执行并行动画求值。 
CompleteParallelAnimationEvaluation CompleteParallelAnimationEvaluation(bool bDoPostAnimEvaluation)
PerformAnimationProcessing 更完整的处理入口。可以只 update，也可以 update + evaluate。
EvaluateAnimation 真正做动画图评估，产出：Pose、Curve、Attributes
PostAnimEvaluation 动画求值结束后做收尾
FinalizeBoneTransform 骨骼结果正式 finalize。这里也会触发 OnBoneTransformsFinalized 之类的委托
FinalizeAnimationUpdate FinalizeAnimationUpdate()
```
```
一帧里大概是：
TickComponent
  -> TickPose
       -> TickAnimation
            -> TickAnimInstances
  -> RefreshBoneTransforms
       -> DispatchParallelEvaluationTasks
          或 DoParallelEvaluationTasks_OnGameThread
       -> EvaluateAnimation / PerformAnimationProcessing
       -> 结果写入 AnimEvaluationContext
       -> CompleteParallelAnimationEvaluation
       -> PostAnimEvaluation
       -> FinalizeBoneTransform
       -> FinalizeAnimationUpdate
```

#### 4.骨骼需求骨头集合：RequiredBones
```
TArray<FBoneIndexType> RequiredBones;
TArray<FBoneIndexType> FillComponentSpaceTransformsRequiredBones;
TSharedPtr<FBoneContainer> SharedRequiredBones;
```
不是每帧所有骨头都必须算。某个 LOD 下，真正需要的骨头集合，会被整理成 RequiredBones。
这样做的目的：降低动画计算量、降低组件空间变换填充量、配合 LOD / 物理 / 曲线优化

#### 5.物理系统：
这个骨骼组件如何和 PhysicsAsset / 刚体 / 约束 / ragdoll 交互？
```
动画先算姿态
UpdateKinematicBonesToAnim 把需要的骨骼推给 physics
physics sim 运行
EndPhysicsTickComponent
PerformBlendPhysicsBones
最终骨骼结果更新回组件
```

#### 6.布料系统
```
用当前骨骼姿态更新 cloth 输入
更新 cloth 碰撞源
启动并行布料模拟
模拟结束后 writeback 结果
最终参与渲染
```
#### 7.优化与开关位

#### 8.曲线 / MorphTarget / Attribute 系统
##### 曲线
动画曲线可以驱动：Morph Target、材质参数、逻辑值
##### MorphTargetCurves
这是组件级覆盖曲线。可以覆盖动画里同名 Morph 曲线。
##### Attribute
这是骨骼上的自定义属性系统。可以按 bone name + attribute name 去取。

#### 9.Linked Anim / Layer / PostProcess
一个 USkeletalMeshComponent 不一定只跑一个 AnimInstance。
可以有：
```
主实例 AnimScriptInstance
Linked Anim Instances
Layer linked instances
PostProcessAnimInstance
```
所以更新流程为：
```
主实例算完
→ linked layer 跑
→ post process 再跑
→ 最后合成
```
#### 10.Tick 函数与组件生命周期
```
OnRegister
OnUnregister
OnCreatePhysicsState
OnDestroyPhysicsState
InitializeComponent
TickComponent
BeginPlay
RegisterComponentTickFunctions
RegisterEndPhysicsTick
RegisterClothTick
```

### 一次动画更新路径：以Anim Blueprint为例：

```
TickComponent
-> TickPose
    -> TickAnimation
        -> TickAnimInstances
                -> AnimInstance / Proxy Update   // 更新状态、推进时间、更新各种驱动条件
-> RefreshBoneTransforms
    -> 组织 Evaluate / Processing
    -> Proxy / AnimGraph 真正求值
    -> 在这里产出 Pose / Curve / Attribute
    -> 也在这套 processing 里提取 Root Motion delta
    -> 写入 AnimEvaluationContext / Proxy / AnimInstance 缓存
    -> CompleteParallelAnimationEvaluation
    -> PostAnimEvaluation
    -> FinalizeBoneTransform
-> 后续由角色移动/组件逻辑去 Consume / 应用 Root Motion
```

#### 1.查看本次更新的驱动来源：
根据 AnimationMode，也就是UE中的下拉选项
```
AnimationBlueprint：走 Anim Blueprint / UAnimInstance
AnimationSingleNode：直接播单个动画资源
AnimationCustomMode：你自己接管，Engine 尽量不替你重建默认动画实例
```
#### 2.Update —— 先更新状态，不是立刻出骨骼
先让动画系统知道“角色这帧应该怎么播”，但这时还不是最终骨骼结果。
```
更新速度、方向、是否跳跃、是否落地
更新状态机
更新 BlendSpace 输入
更新 Montage 状态
更新各种动画节点的内部状态
```
由动画实例主导，更新各种变量，让这一帧“应该怎么播”先定下来

#### 3.Evaluate：真正算出姿态、曲线、属性
```
计算 Bone Transforms
计算 Anim Curves
计算 Custom Attributes
```
USkeletalMeshComponent：发起/调度/提交结果
FAnimInstanceProxy：执行入口
AnimGraph 节点：真正递归算 pose
结果存入AnimEvaluationContext：本次求值工作区

#### 4.并行还是主线程执行？
满足条件时，尽量把动画 update / evaluate 放到 worker thread；不满足时，就只能回到 game thread。
#### 5.RefreshBoneTransforms()
前面动画系统算出了结果，现在要把这些结果正式落到 USkeletalMeshComponent 自己身上。
```
5.1 更新骨骼变换缓存：把评估结果写进组件自己的骨骼缓存里。
5.2 更新 Component Space / Cached Space：让后续系统都能读到这帧最终姿态。
5.3 Finalize Bone Transforms 创建RegisterOnBoneTransformsFinalizedDelegate()，并且明确写着这个回调会在 FinalizeBoneTransform() 里被调用。
```
#### 6.Finalize 之后，会触发哪些可观察事件（即委托）
#### 7.根据设置，把骨骼结果同步给 Physics
动画得到的骨骼姿态，要不要继续推给物理世界里的刚体 / 骨骼体。
#### 8.布料模拟相关
#### 9.Physics 结束后，再跑 EndPhysics Tick
#### 10.渲染线程/渲染数据需要读取这帧最终骨骼结果，去驱动 Skeletal Mesh 顶点蒙皮。

### FAnimInstanceProxy：UAnimInstance 对应的一个代理对象。
UAnimInstance 是 UObject，偏游戏线程、蓝图实例、运行时逻辑对象。但动画系统底层要做高性能 update/evaluate，甚至要并行，所以不能让底层到处直接抱着 UAnimInstance 乱跑。
于是 UE 做了一个代理层：把动画实例中“给底层动画系统执行用”的那部分能力和数据，放到 Proxy 里。
UObject 不适合随便跨线程访问
Proxy 更适合作为动画系统内部执行对象

### AnimGraph：动画蓝图编译后，对应的那一棵动画节点树里的节点
真正递归计算这一帧的 pose。