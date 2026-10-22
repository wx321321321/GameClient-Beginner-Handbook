# 镜头




### 业务层：


## 调用链：

### 渲染底层：
##### 渲染管线内部写好了渲染逻辑：
```
主世界 Pass
读取 EngineCamera
绘制 DEFAULT RenderSet
HUD Sight Pass
从 OpticalSightCamera 读取 HudCamera
绘制 HUD_SIGHT RenderSet
```
所以，只要对两个Icamera对象：EngineCamera、HudCamera进行操作就好。
```
ICamera 是一个类，不是唯一的一台相机。S18正式战斗主链里主要有两个不同的 ICamera 实例。
world Camera
MEngine.GetGameplay().Player.Camera它负责渲染：地图角色建筑敌人世界特效,Controller通过 _ControlledCamera 指向它。
HUD Camera
genv.hud_camera它主要负责第一人称的：手枪瞄具第一人称附属模型,Controller通过 _AffiliatedCamera 指向它。渲染时，会把1p的手和枪打上不同的renderset，当3p镜头，隐藏1p手枪的set，渲染世界。
世界相机和 HUD 相机的位置、朝向完全一样，只是fov有差别
```

### space与相机管理器：
##### space是当前的游戏场景，根据模式而来。在创建了space后，space会创建一个camera逻辑对象，camera逻辑对象则是创建一个camera管理器，作为相机的核心。
```
CameraController
├─ LocateBlendTree
│  └─ FPS/TPS Placer 或 Blender
│
├─ SensorBlendTree
│  └─ Checker → TpsCollider → CameraCollider
│
├─ PostProcessorBlendTree
│  └─ CameraShaker
│
├─ Cameraruntimeinfo*mCurrentCamInfo
├─ ICamera* ControlledCamera;
└─ ICamera* AffiliatedCamera;
```
controlledcamera指向世界相机，affiliatercamera指向HUD相机。

#### cameraruntimeinfo：相机管理器内部计算的数据载体。
```
它使用五个主要字段：
target	镜头观察的目标点
direction	从镜头位置指向 target 的向量，包含距离
roll	镜头侧倾
世界相机 FOV
HUD相机 FOV
最重要的一点
RuntimeInfo 没有 position。
镜头位置由 Controller 计算：
```
相机管理器自上而下更新，写入currentcaminfo中，最后的处理结束后，写入到ControlledCamera和AffiliatedCamera中。

#### placer：分为cpp原生的placer和python包装后，带有s18逻辑的placer
##### Python 包装器负责什么
```
它负责理解游戏业务：
• 当前武器及武器配置
• ADS、换弹、开火状态
• Recoil 状态机
•Jitter、RollShake、BurstShake
• 玩家鼠标输入
创建并挂载Easing、Mover、Blender
读取角色、手模、骨骼等业务对象
• 把结果写给底层C++ Placer
```
##### C++ Placer 负责什么
```
C++对象负责底层几何计算：
text
保存 _Direction
保存 _Roll
保存 _Fov
保存 Target、offset
消费 CameraEasing/Mover
每帧计算 target和direction
写入 CameraRuntimeInfo
```
它回答一个问题：暂时不考虑穿墙、震屏和最终写入，这一帧镜头理论上应该在哪里、朝哪里？生成本帧的 CameraRuntimeInfo
```
不同玩法的几何算法不同：
FpsPlacer：镜头固定在眼睛附近
TpsPlacer：镜头围绕角色
AimPlacer：持续对准指定目标
FollowPlacer：跟随另一个 Camera/Controller
Orbit/PivotPlacer：围绕中心旋转
BonePlacer：跟随骨骼
FreeviewPlacer：自由镜头
FramePlacer：读取固定 CameraFrame
```
#### 核心变量：position，target，direction：
```
FPS 想让镜头留在眼睛位置，所以先算：cameraPosition = 眼睛位置 + offset
玩家转头时改变 direction，然后配套改变：target = cameraPosition + direction
因此转头过程中：旋绕世界

TPS 想让镜头围绕角色，所以先算：
target = 角色附近的观察点
玩家旋转镜头时改变 direction，Controller 得到：cameraPosition = target - direction
因此旋转过程中：围绕玩家
```
#### 修改工具：Placer 每帧 Tick 时推进并消费 Easing/Mover，再结合输入和自身机位规则生成 CameraRuntimeInfo(cpp内部)
```
CameraBlendTree
└─ TpsPlacer
   ├─ _Easing
   └─ _Mover`
```
![alt text](image-1.png)
```
CameraEasing	给 Placer 提供随时间变化的值
Mover	产生连续移动结果(速度)
```



#### LocateBlendTree：下面挂载placer
```
CameraBlendTree
└─ TpsPlacer
   ├─ _Easing
   └─ _Mover`
```
placer是一种几何层的计算，一颗blendtree下大概率只有一个placer，但偶尔会有两个，如fps到倒下的过程中，就需要同时挂两个placer，对结果进行过渡。
placer每一帧会tick，结合身上的easing和mover来对最终状态进行一个修改，从而输出计算结果写入runtimeinfo中。

#### SensorBlendTree：进行碰撞的检测。

#### PostProcessorBlendTree ：后处理
其实就是在这下面挂shaker，tick计算出一个值，加到runtimeinfo上。

#### 最后将runtimeinfo里面的计算结果，分别写入到两个icamera对象中。

# 子弹方向设计方向：

## 思路：
#### 普通子弹：
先生成一条基础方向，再在一个圆锥里随机偏一次；之后还会经过瞄具 HUD 修正和重力分段，所以命中时的 hit_dir 可能已不同于初始方向

#### 散弹枪：
霰弹枪：先取一条中心方向，再生成“内圈随机弹丸 + 外圈等角度环形弹丸”；当前实现每颗弹丸独立飞行和穿透，但没有普通枪那套重力下坠

## 流程：
### 1.确定这次开火交给谁处理
普通枪：GunSpellWorker
霰弹枪：ShotgunSpellWorker
### 2.短暂预热后正式开火，发出通知。

#### 3.1对于普通枪，在进行后座逻辑tick前，先缓存当前的镜头方向。
原因：如果先后座逻辑，造成的后座会影响这颗子弹的方向。所以每一次开火事件都要缓存当前方向。


### 4.1 确定射线穿过屏幕的哪个点：
##### 如果是普通射击：直接用屏幕中心（屏幕中心是一个二维坐标，EngineCamera 用这个二维坐标生成三维射线）
##### 如果是开瞄准镜射击：有个aimbone，其实就相当于对camera的位置进行了个偏移变换。
计算：得到相对于camera位置，aimbone的位置，对camera透视投影，算出aimbone在XY上的偏移，加到中心点上。

其实就是：镜头方向决定能看到的视野范围，而普通墙永远打在这片视野的中心点，而开镜会有偏移

### 5.1 转换成射线方向
开镜，会叠加镜片的瞄准方向。
eg：加了四倍镜八倍镜后，普通射击还是一样，但开镜不一样，会有一点偏移。



### 6.1 确定散布锥：
现在已经得到无散布方向，确定散布角度：
ADS：当前不产生散布。
普通射击：从移动、姿态、连续开火中计算出来角度。
```
武器维护状态：
LastScatterAngle 积累的散布
ShootScatterAngleMin   // 最小散布
ShootScatterAngleMax   // 最大散布
```
AllScatterRatio =移动/姿态倍率× 瞄准状态倍率× ADS倍率× 天赋或配件修正
```
调整本次允许的最小、最大散布：
currentMin =ShootScatterAngleMin × AllScatterRatio
currentMax =ShootScatterAngleMax × AllScatterRatio
```
#### SIPS子弹散射每开一枪增加的扩散值角度
SIPS不改变当前这一发
SIPS增加的是下一发使用的散布状态
eg ：SIPS为0.8，散布范围为1-10，第一枪，积累散布为0，所以散布为1，第二颗子弹为0.8,，所以散布为1......

### 7.1 求子弹的真正方向：
#### 随机数：
U = GunRandom.NextDouble();决定往哪个方位偏
```
U = 0    → theta = 0°
U = 0.25 → theta = 90°
U = 0.5  → theta = 180°
U = 0.75 → theta = 270°
```
V = GunRandom.NextDouble();决定圆周上的角度
```
rawAngle = acos(2V - 1)%2（假设最大偏移为2）
```
算出来一个偏离basedir的方向

### 8.1 分段重力与命中处理：
```
散布、重力弹道、分段射线、客户端看到的命中
→ 客户端计算
是否接受这次命中、最终伤害、扣血
→ 服务端处理
```
#### 分段重力：根据帧率得到t将抛物线分段。
##### 初始信息：
```
D₀ = (0, 0, 1)
V₀ = (0, 0, 800)
G  = (0, -20, 0)
Δt = 0.01
```
##### 第一段计算：
```
V₀ × Δt= (0, 0, 8)+½GΔt²= 0.5 × (0,-20,0) × 0.0001= (0,-0.001,0)
合起来：
ΔP₀ = (0, -0.001, 8)
意思是第一段：
向前飞8个世界单位向下掉0.001个世界单位
第一段方向：
segmentDir₀ =normalize(0, -0.001, 8)
然后客户端沿这个方向做第一段碰撞检测。
同时更新速度：
V₁ = V₀ + GΔt
   = (0,0,800) + (0,-0.2,0)
   = (0,-0.2,800)
```
##### 第二段：同理
每一段客户端都会检查：
场景碰撞，角色骨骼碰撞，穿透目标
如果命中了带命中信息，服务器校验，计算伤害。

##### 停止：
```
撞到不可继续穿透的目标。
穿透能力耗尽。达到最大射程。
子弹速度降到接近零。
玩家或武器被销毁。
超过保护上限 512 段
```
真正会受帧率影响的是碰撞精度：高帧率：很多短射线逼近抛物线 .低帧率：少量长射线逼近抛物线

### 3.2 对于霞弹枪 ：没有缓存，直接读取镜头方向
相机后坐或 Easing 如果在这之前已经写入 EngineCamera，就会影响本次霰弹中心方向

### 4.2 根据配置 决定一枪生成多少颗 pellet，以及多少颗属于内圈、多少颗属于外圈
全在配置里面。

### 5.2 计算整枪的主散布半角 α  只调用一次。
和普通枪一样。
决定外圈pellet的主圆环半径，参与缩放内圈pellet的随机范围

### 6.2 从配置中读取比例，计算内圈范围

### 7.2 内圈偏移：
每颗都会重新取自己的 U、V，不是所有内圈 pellet 共用同一个随机偏移
随机的。

### 8.2 外圈偏移：
先把pellet 均匀分布在圆周上，且距离圆心偏移固定，都用 α 。
再给每一颗子弹加一个随机扰动：读配置ShotgunSpreadRandomAngle，再像之前普通枪一样，算角度，算偏移，给每颗子弹单独加上。

### 9.2 视觉表现：
对每颗子弹射线检测，800m内如果碰到东西，就在物体上，否则500m触发后续视觉。
