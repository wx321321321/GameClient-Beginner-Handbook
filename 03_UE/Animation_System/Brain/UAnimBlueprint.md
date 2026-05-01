# UAnimBlueprint
相对于普通蓝图，进行适应于动画的改进。
###  把蓝图正式和 Skeleton 绑定起来了
####  TargetSkeleton：目标骨骼
####  bIsTemplate：这个 AnimBlueprint 可以作为“模板动画蓝图”存在，而不绑定具体 Skeleton。

### 把编译结果从普通蓝图类，升级成动画专用生成类链
```
GetAnimBlueprintGeneratedClass()
ENGINE_API class UAnimBlueprintGeneratedClass* GetAnimBlueprintGeneratedClass() const;
GetAnimBlueprintSkeletonClass()
ENGINE_API class UAnimBlueprintGeneratedClass* GetAnimBlueprintSkeletonClass() const;
```
UAnimBlueprint 不再只是走通用 UBlueprintGeneratedClass 逻辑，
它明确接入了 UAnimBlueprintGeneratedClass 这条专用编译产物链。
```
相比普通 UBlueprint：
UBlueprint
   ↓
UBlueprintGeneratedClass
动画蓝图这里变成了：

UAnimBlueprint
   ↓
UAnimBlueprintGeneratedClass
编译结果里会带上动画专属内容，比如：
AnimNode 布局
动画状态机编译结果
层/节点绑定
调试数据
动画类默认数据
```
#### GetBlueprintClass() const override：返回动画蓝图专用生成类
#### IsValidForBytecodeOnlyRecompile() const override { return false; }
通用蓝图有些时候允许更轻量的 bytecode-only 重编译。
但这里动画蓝图明确返回：false
动画蓝图不能只当普通字节码逻辑重编译，它需要更完整的编译流程。
因为其中还有
```
AnimGraph
AnimNode 结构
状态机
资产播放器
层和 linked graph
```
###  补上动画蓝图专有运行/优化能力：
####  Groups：动画同步组（sync groups）
把动画同步组信息直接存入动画蓝图中
```
FindOrAddGroup
```
####  bUseMultiThreadedAnimationUpdate：允许这个动画蓝图把 native update、blend tree、montage、asset player 等更新放到 worker thread。
####  bWarnAboutBlueprintUsage：当 AnimGraph 从图里调用 Blueprint 时，编译器给警告。
AnimGraph 更像“姿势计算区”，UE 希望它尽量只做纯粹的动画计算；
普通 Blueprint 调用更像“游戏逻辑区”，里面可能会访问 Actor、Component、World、Character Blueprint 之类的对象，而这些访问很多只适合在 Game Thread 上做。Property Access 这个功能就是 Epic 专门拿来在 Animation Blueprint 里安全读取这些通常只能在 Game Thread 上访问的数据的。
#### ：bEnableLinkedAnimLayerInstanceSharing inked Animation Layer 是否按 AnimClass 共享实例，而不是按每个 AnimInstance 都各来一份
### 对 UBlueprint 的“能力开关”做动画专用重写
#### Anim Layers 是动画蓝图专属能力，不属于通用蓝图。
### 引入动画资产 override 体系
### 增强调试能力，专门支持动画图调试 
### 和 AnimGraph 节点生态打通了：

#### friend class FAnimBlueprintCompilerContext;它有专门的动画蓝图编译器上下文。
#### UAnimBlueprint 已经和 AnimGraph 节点绑定系统直接耦合了。
UBlueprint 是“通用蓝图资产容器”，而 UAnimBlueprint 把它升级成了“以 Skeleton 为核心、以 AnimBlueprintGeneratedClass 为编译产物、以 AnimGraph/AnimLayer/资产Override/PoseWatch/预览叠加为专用能力”的动画蓝图资产。

## Anim Layers：动画层 给 AnimBP 做接口化和模块
Anim Layers 是 AnimBP 的分层和模块化机制，通常通过 Animation Layer Interface 定义层，在 AnimBP 里实现，在 AnimGraph 用 Linked Anim Layer 节点接入，运行时还能通过 Link Anim Class Layers 动态切换实现。它主要用来把 Locomotion、UpperBody、FullBody 这种动画逻辑解耦，便于复用和替换
eg:
对于角色，有locomotion和上半身的动作，如持枪，跑、捂头、举手等动作，就先实现locomotion，然后link Anim Layer（upper），随后做四个动画蓝图都继承公共upper的接口，在mesh的蓝图中，根据状态切换：Get Mesh -> Link Anim Class Layers(ABP_Upper_Unarmed)
是否覆盖根据对应的动画蓝图中动画层的实现逻辑来，都有input-output