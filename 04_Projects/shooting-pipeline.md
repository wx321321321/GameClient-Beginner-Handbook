# 射击框架与开火链路

[返回本章目录](README.md) · [返回手册首页](../README.md)

这篇沿“左键按下 → 输入分发 → 开火检查 → Spell → 动画 Cue → 子弹任务 → 命中上报”梳理原项目。关键是区分**有开火意图**、**进入开火状态**与**真正产生一发攻击**，三者发生在不同节点。

`MEngine`、`GameInputController`、Spell、Graph、`AsyncFuncNode` 等均为原项目名称，下面保留原笔记的调用线索。仓库没有对应工程源码，具体拼写、回调签名及先后顺序仍需按实际版本核对。

## 从引擎输入到业务动作

### 引擎与脚本绑定层

游戏进程启动后，C++ 引擎初始化 Engine，以及 Gameplay、Player、World、Physics、Render 等子系统。Python 侧通过 `MEngine` 等绑定接口访问这些能力；不是 Python 天生不能调用 C++，而是两种运行时之间需要绑定层转换参数和调用。

```text
Engine
├─ Gameplay
│  ├─ Player：本地主控入口
│  └─ Scenario：当前世界（原稿写作 sceario，源码拼写待核对）
├─ 程序信息
├─ 帧与事件
├─ 窗口
└─ 脚本通信、文件与路径
```

原稿把一帧概括为读取输入、更新逻辑、动画、物理、场景，再提交渲染和显示。这个模型有助于理解“事件何时被消费”，但它不是所有引擎固定的串行顺序：渲染与显示可以流水并行，动画和移动也会有自己的依赖调度。

`MEngine.Player` 提供的相关入口包括：

```text
Player.BindEvent       输入事件绑定
Player.Camera          当前摄像机
Player.Navigator       导航、视角控制
Player.Manipulator     编辑器操作器
Player.PrincipleRole   当前主要控制/观察的场景角色
```

Python 注册输入回调后，C++ 输入层检测按键，调用绑定的脚本入口。`DesktopInput` 接收并广播原始输入，监听它的 `GameInputController` 再执行游戏级分发。

### 左键如何变成配置里的开火函数

```text
左键按下
→ DesktopInput 广播
→ GameInputController.OnMouseLDown
→ 记录鼠标左键状态
→ self.OnKeyDown(0, DesktopInput.MOUSE_L)
→ 记录通用按键状态
→ self.DoKeyUpDownFunc
```

通用按键处理先检查改键系统是否消费，再经过大厅、特殊玩法、UI 等分支，最后才查询动作配置。这样同一个按键可以在不同上下文里有不同含义。

原笔记记录的一个实际调用是：

```python
DoKeyResponseFunc(39, True, 257)
```

其中通过键值 `39` 查询缓存/配置，得到按下和松开的处理名，例如：

```text
OnPlayerAttackForMouseL
OnPlayerENdAttackForMouseL  // 原记录拼写，待源码确认
```

`True` 选择按下动作，再拼出 `PlayerInput_OnPlayerAttackForMouseL`。这时只确定函数名，还要按项目分发规则寻找实现对象：

1. `logic_input`：特殊玩法输入对象。
2. `contrller_player`：原笔记记录的当前角色入口，具体拼写待核对。
3. 观战玩家：要求配置中允许该分支。
4. `GameInputController` 自身。

所以左键在正常状态可能开枪，在载具里可能开炮，在观战时可能切换视角，也可能由全局输入逻辑消费。

## 开火检查：意图尚未等于子弹

`PlayerInput_OnPlayerAttackForMouseL` 先判断当前状态：濒死、操作 UI、禁用攻击等会阻止开火；持有道具时可能转为道具使用，使用后切枪，再重新走攻击流程。

通过检查后设置开火标记并通知手模。拿刀时可能先做命中预测，帮助手部 Graph 选择表现。随后根据姿态分流，调用 `OnPlayerAttackBegin()` 并通知客户端其他模块。

`OnPlayerAttackBegin()` 继续处理打药、救援、持枪等上下文，清理旧失败原因，再进入火器分支：

```text
OnPlayerFireBegin
→ 清理旧输入缓存
→ try_fire_start = True
→ 取得当前枪械
→ 判断枪械类别
→ _OnPlayerFire_Normal(fire_mode, True)
→ TryAttack()
```

## Spell：把一次战斗行为组织成生命周期

Spell 是原项目对可执行战斗行为的统一抽象。当前行为通过角色的 `spell_mgr.current_spell` 管理，记录的种类包括：

```text
gun_spell       普通枪械
shotgun_spell   霰弹枪
grenade_spell   投掷物
medicine_spell  药品
melee_spell     近战
```

周期接口把“开始动作”和“发生攻击”分开：

```text
SpellStart()      开始行为，启动动画/控制状态
SpellStrike()     真正产生一次攻击
SpellStop()       停止行为
SpellOnClear()    清理 Spell 实例
```

`TryAttack()` 根据当前 `main_spell` 找到 `code = gun_spell` 等行为代码，再分派到 `TryAttack_gun_spell`。成功创建后的 Spell 记录到当前行为管理器；不要把 `TryAttack` 本身理解成每次都直接生成子弹。

### 普通枪的一轮开火

```text
TryAttack_gun_spell
→ SpellStart：创建/启动攻击控制逻辑
→ Caster.PlayFIreAction（原记录拼写）
→ 通知 handmodel 与第三人称 model 的 jumptofire
→ handmodel.jumptofirestart 写 Graph 变量、投递事件
→ 下次 Graph Tick 消费 eventsqueue
→ 进入 fire_first / fire_loop
→ 动画到达出膛 Cue
→ Cue 分发到 Python
→ 驱动后座状态与 SpellStrike
```

Graph 变量还控制循环动画播放速度。Cue 到来时，通过 `current_spell` 找回 Spell Worker，扣弹、设置 `has_strike`，再让 `SpellStrike` 创建该发子弹任务。镜头 Placer 的 FSM 同时参与后座处理；普通枪为了避免本发后座影响本发初始方向，会在相应后座更新前缓存方向，详见[相机系统与射击方向](camera-and-shooting-direction.md)。

松开输入走停止链：

```text
EndAttack → stopspell → 停止动画/行为并清理相应状态
```

停止当前开火与是否立即清空连续射击序号，是两个独立问题。

## 射击字段与短暂停顿

原项目在 `CombatAvatar` 上维护这些字段，供武器、动画与后座逻辑读取：

```text
player.shoot_idx
player.last_shoot_idx
player.continuous_shoot_idx
player.need_reset_recoil_start_state
player._is_shooting
player.try_fire_start
```

原稿还出现 `self.is_shooting`；它与 `_is_shooting` 是属性封装还是不同字段，需查源码，不能整理时擅自合并。

| 时机 | 状态变化 |
| --- | --- |
| 角色创建 | 初始化射击字段 |
| `OnPlayerFireBegin` | `try_fire_start = True`，表示尝试开火 |
| 新一轮开火通过检查 | `RefreshShootIndexOnGunSpellStart()` 决定是否初始化射击序列 |
| `SpellStart` | `self.spell_start = True`，行为已启动，但不代表出膛 |
| 每个出膛 Cue | 更新本发、上一发和连续射击等计数 |
| 松开攻击 | 结束开火状态，并按延迟策略处理序号 |

原笔记给出的新序列初始化值保留如下：

```python
shoot_idx = 0
last_shoot_idx = -1
continuous_shoot_idx = 0
need_reset_recoil_start_state = True
self.is_shooting = True
```

松开时调用 `TryClearShootIdxOnEndAttack()`，读取武器配置 `ShootIdxResetDelayTime` 并设置定时器。超过延迟才执行序号重置；若短时间内再次开火，则可能沿用之前的子弹序列，但动画要重新进入开火流程。

这解释了原稿的矛盾：“松开时清零所有信息”不能与延迟重置同时成立。应分别核对即时关闭的状态标记、延迟清理的序号，以及重新开火时如何取消旧定时器。

## 子弹任务：协程按时间推进弹道

每次 `SpellStrike` 创建一个 `AsyncFuncNode`，保存 Generator、局部飞行状态与定时调度信息。这里的“异步”是原项目的协程调度，不表示自动新建一个线程。

```text
定时器到期
→ 主线程恢复对应 Generator
→ 计算本次飞行段及碰撞
→ yield，登记后续恢复时机
→ 主线程继续其他任务
```

原笔记记录：创建初期先做 **0.01 s / 10 ms** 的飞行计算，之后约每 **33 ms** 恢复一次。它与显示帧率不是同一个常量；实际弹道应使用真实经过的时间，需核对调度器如何补偿延迟。

任务保留的变量包括：

```text
bullet_fly_speed          当前标量速度
bullet_fly_speed_v3       当前速度向量
bullet_fly_range          最大射程
gravity                   重力
bullet_acceleration       速度衰减相关配置
start_pos                 当前分段起点
cur_fly_dist_v3           本段位移
cur_bullet_dist           累计飞行距离
cur_time                  上次更新时间
penetrate_handler         剩余穿透能力及处理状态
bullet_exclude_actor      已命中、后续需要排除的对象
```

### 每段碰撞怎样得到命中结果

当前分段用起点和终点构成线段，依次完成：

```text
对场景物理世界执行 AllRaycast
→ 对角色骨骼受击盒执行 Raycast
→ 合并结果，按距离从近到远排序
→ 判断材质、敌友、是否排除及能否继续穿透
→ 生成 ShootEntityResult
→ 汇总为 SpellResult
→ 上传给服务端校验和伤害处理
```

先排序再判断穿透很关键：同一段射线可能穿过多个目标，近处的阻挡会改变后续目标是否可命中。客户端算出的碰撞结果与服务端最终接受的伤害不是一回事。

## 客户端和服务器的职责边界

| 阶段 | 原笔记记录的参与方 |
| --- | --- |
| 原始左键与本地输入分发 | 客户端执行，不直接把鼠标事件交给服务器 |
| 本地 Graph 播放 | 客户端表现链 |
| 进入射击状态 | 通过 `CallServer('EnterIsShootingStatus', True)` 通知服务器 |
| Cue 到达与扣弹请求 | 客户端触发，服务器按协议校验相关信息 |
| 命中结果 | 客户端提交，服务器校验并处理最终伤害/同步 |

原稿的“服务器完全不参与”只适用于这里记录的输入和 Graph 调用路径，不能推导成服务器不检查攻击频率、弹药或行为合法性。具体校验范围尚无本地源码证明。

## 方向计算在整条链中的位置

普通枪先从相机与瞄准点生成 `BaseDirection`，再依据移动、姿态、连发和武器配置计算散布。原笔记中 `1.2 × 1.5 = 1.8` 是倍率相乘的例子，不代表所有武器固定使用这两个参数。

```text
randU → theta：绕准星方向的方位角
randV → phi：偏离准星中心的角度
```

这两个随机数决定偏向哪里、偏离多少；具体角度分布要看实现，不能简单称为随机半径。普通枪后续还可经历 HUD 瞄具修正、分段重力和穿透。

霰弹枪先确定整枪中心和主散布，再分别生成内环随机弹丸、外环均匀分布弹丸，并对外环增加独立扰动。原稿“r/2 圆环”是一个范围描述，实际应以 `ShotgunSpreadRandomAngle` 等配置为准。每颗弹丸独立检测，方向与数值例子集中放在[相机系统与射击方向](camera-and-shooting-direction.md)，避免在两篇中维护互相矛盾的版本。
