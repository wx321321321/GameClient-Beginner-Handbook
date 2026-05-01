## Comparision:
### UAnimBlueprint:动画蓝图资产类型
在 Content Browser 里看到的那个：ABP_Player本质上就是 UAnimBlueprint 资产。
描述的是“动画逻辑定义”，包括：
Event Graph
AnimGraph
状态机定义
动画节点定义
变量定义
蓝图函数图
层/接口相关定义
#### 像是动画逻辑的源文件

### UAnimBlueprintGeneratedClass：UAnimBlueprint 编译后的类结果
普通蓝图编译后会变成某个 UBlueprintGeneratedClass。
动画蓝图也一样，只不过它是更专门的：
UAnimBlueprintGeneratedClass
AnimGraph 里的各个节点，在编译后会变成 UAnimInstance / 相关类里的成员结构布局。
##### 什么时候起作用：
当一个 SkeletalMeshComponent 要创建 AnimInstance 时，通常会基于这个 GeneratedClass 去实例化对应 AnimInstance。
```
AnimBlueprint 资产本身不能直接拿来跑
得先有 GeneratedClass
再实例化成 UAnimInstance
```
### UAnimInstance 是：运行时真正挂在 SkeletalMeshComponent 上的动画实例对象
每个角色、每个 MeshComponent 都可能有自己一个 UAnimInstance。
#### 作用：
##### （1）保存动画变量：变量你在 AnimBP 里看到，运行时就存在某个 UAnimInstance 实例里
##### （2）处理动画更新入口:Event Graph 里更新变量，本质就是在 UAnimInstance 这一层更新。
```cpp
NativeInitializeAnimation
NativeUpdateAnimation
BlueprintInitializeAnimation
BlueprintUpdateAnimation
```
##### （3）管 Montage、通知、状态机运行时状态
```cpp
当前 Montage 是否在播
播到哪一节 Section
状态机当前在哪个 state
状态过渡还剩多少时间
当前 active curve 是什么
```
##### （4）作为 AnimGraph 的拥有者:
AnimGraph 虽然是图逻辑，但它的运行时节点数据、变量、状态，一般都是挂在 UAnimInstance 及其 proxy 上的。
```cpp
CharacterMovement / Pawn
    ↓
UAnimInstance 读取速度、落地状态等
    ↓
AnimGraph 计算最终 Pose
```

### FAnimInstanceProxy:UAnimInstance 的运行时动画计算代理
#### 存在原因：
（1）UAnimInstance 是 UObject，太“重”
```cpp
UObject 世界里有：
GC
反射
蓝图调用
线程限制
但动画求值希望：
更轻量
更线程安全
更适合并行执行
```
（2）动画评估常常想离开游戏线程
这时不能到处直接碰 UObject / Actor / 蓝图对象，所以需要一个“代理层”：
```
游戏线程：
    UAnimInstance 更新变量
        ↓
把必要数据同步给 Proxy
动画线程/并行任务：
    用 Proxy 驱动节点更新和求值
```
#### 主要任务：
```
保存动画求值所需的线程安全数据
持有 AnimNode 执行上下文
作为节点 Update / Evaluate 的核心数据承载体
配合并行动画更新/求值
```
### AnimGraph：动画蓝图里的 Pose 计算图
把输入变量、状态机、动画节点、混合逻辑、IK 等，最终算成一个 Pose。
```
Speed
bIsFalling
AimPitch
    ↓
StateMachine
    ↓
BlendSpace
    ↓
Slot
    ↓
Layered Blend Per Bone
    ↓
IK
    ↓
Output Pose
```

### Event Graph
```
负责：

更新变量
读角色状态
做逻辑判断

例如：

取速度
取是否落地
算方向
改一些布尔值
```