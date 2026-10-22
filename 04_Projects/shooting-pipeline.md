# 射击框架与开火链路

[返回本章目录](README.md) · [返回手册首页](../README.md)

## 帧：一帧是游戏主循环完整执行一次

```
读取输入
→ 更新游戏逻辑 Update/Tick
→ 更新动画
→ 更新物理
→ 更新场景
→ 提交渲染
→ 显示画面
```

## shoot pipeline

### 一.左键按下->引发gameinput_controller模块启动->调用最终开火函数

#### 1.引擎的作用：

（1）启动游戏进程，os分配空间， 执行已经编译好的弥赛亚C++引擎代码，初始化Engine对象。
（2）初始化Gameplay，Player，World，Physics，Render等子系统
（3） 创建MEgine，创建py端的引擎对象，给py代码逻辑提供功能绑定层。（客户端逻辑不能直接调用cpp的函数，作为中间接口，并提供一些功能的扩展）
（4）有些逻辑直接使用MEngine的相关函数。

```
Engine
--Gameplay         
        ----player：主控
        ----sceario：当前世界
--程序信息
--帧与事件
--窗口
--脚本通信、文件与路径
```

#### 2.事件绑定与触发

##### MEngine.Player：本地玩家总控制入口

```
Player.BindEvent       输入事件
Player.Camera          当前摄像机
Player.Navigator       导航、视角控制
Player.Manipulator     编辑器操作器
Player.PrincipleRole   当前主要控制/观察的场景角色
```

##### 将按下鼠标左键等一堆事件，在py侧通过接口绑定到player对应函数上。

##### cpp检测到鼠标左键按下，找到对应py回调，调用函数。

##### 触发GameInputController.OnMouseLDown

#### 3.通用分发

##### DesktopInput：负责接收并广播引擎原始输入

使用观察者模式，将左键按下事件，传递给监听他的gameinputcontroller

##### GameInputController:py侧游戏的输入分发器

##### 函数做了什么：

```
记录左键状态（鼠标专用状态）
调用 self.OnKeyDown(0, DesktopInput.MOUSE_L)->记录通用按键状态->调用self.DoKeyUpDownFunc
```

##### self.DoKeyUpDownFunc 通用按键处理流程：

改键系统是否消费->大厅->特殊玩法->UI->查询配置->调用处理函数。

#### 4.处理函数 DoKeyResponseFunc(39, True, 257)

##### 查询配置表：

根据键值39，计算cache，去配置表内查对应的func表如：

```
OnPlayerAttackForMouseL
OnPlayerENdAttackForMouseL
```

因为是true，选第一个，随后进行一个拼接，得到如：PlayerInput_OnPlayerAttackForMouseL的函数名

##### 查询实现对象：目前只知道函数名，但不知道它的层次，比如开火应该是角色层，面板显示是ui层，所以挨个查：

```
logic_input:预留的特殊玩法输入接收对象。
contrller_player:当前真正的角色对象
观战玩家：需配置存在（上一步配置表里面存在对应的标志位）
gameinputcontroller自身：
```

eg：同样是左键开火：

```
正常状态：角色开枪
特殊状态：载具开炮
观战状态：执行观战操作（切换视角）
全局状态：输入控制器自己处理
```

### 二.开火前检查：

#### 1.PlayerInput_OnPlayerAttackForMouseL：开火判定函数：

判定条件，当前状态下是否能开火：

```
濒死状态不能攻击
正在持有道具：跳转道具使用相关函数->使用后切枪->切回来之后重新走链路
操作UI不能开火
是否禁止开火
```

通过了所有检查：打一个开火标记，随后，通知给手摸,此时手部已经有了对应的表现

##### 特殊处理：手模且拿刀：因为是否击中的手部动作不同，会做一个预测，来通知手graph的动作

##### 根据姿态分流  确定调用函数self.OnPlayerAttackBegin()

##### 通知客户端其他模块。

#### 2.OnPlayerAttackBegin()

依旧处理一些情况：如打药，救援，拿枪等，并清理旧失败原因，调用FIre

#### 3.OnPlayerFireBegin

```
OnPlayerFireBegin
→ 清理旧输入缓存
→ try_fire_start = True
→ 取得当前枪械
→ 判断枪械类别
→ 普通枪进入_OnPlayerFire_Normal(fire_mode, True)->TryAttack()
```

### 三.开火

#### spell： 对可执行战斗行为的统一抽象,存放在角色的spell_mgr.current_spell中

```
可以把它理解为项目自己实现的“通用技能/行为系统”：
gun_spell       普通枪械
shotgun_spell   霰弹枪逻辑
grenade_spell   投掷物
medicine_spell  药品
melee_spell     近战
```

里面提供完整的周期函数：

```
SpellStart()      # 开始攻击，启动动画
SpellStrike()     # 真正产生一发攻击
SpellStop()       # 停止攻击
SpellOnClear()    # 清理Spell实例
```

#### TryAttack():选择对应spell,找到执行函数

```
TryAttack
→ 使用当前main_spell
→ 查到code = gun_spell
→ 找到TryAttack_gun_spell
存放在角色的spell_mgr.current_spell中
```

#### TryAttack_gun_spell：

##### （1）.spell.SpellStart()：创建攻击控制器，还没有子弹。

```
通知动画开始开火：
调用Caster.PlayFIreAction,同时通知handmodel和第三人称model的jumptofire
handmodel.jumptofirestart：写变量，给graph发事件，放入graph的eventsqueue中，等下一次graphtick处理
graph处理后，进入fire_first和fire_loop，并通过变量来控制循环动画的播放速度。
```

##### (2)graph一边，到达了出膛CUE：

graph产生spell event cue 分发给python回调触发分发事件。

##### （3）Cue 同时驱动镜头和 SpellStrike

```
使camera.placer的FSM转换状态，施加后座影响
通过current_spell找回spellworker，扣一发弹药，标记has_strike，调用SpellStrike
```

##### （4） SpellStrike创建这一发子弹任务

子弹任务以协程的方式，进行一个计算，如果有命中信息，发送到服务器。

##### （5） 松开鼠标后：逆链路

```
调用EndAttack->stopspell->清理
```

### 四、角色身上的字段：

一些字段挂在combatavatar身上，用于读取：

```
player.shoot_idx
player.last_shoot_idx
player.continuous_shoot_idx
player.need_reset_recoil_start_state
player._is_shooting
player.try_fire_start
```

#### 1.角色创建时初始化

#### 2.左键按下，进入onplayerfirebegin时，

try_fire_start设为真

#### 3.通过开火检查时，调用RefreshShootIndexOnGunSpellStart()，意为新一轮射击,随后创建spell

```
shoot_idx = 0
last_shoot_idx = -1
continuous_shoot_idx = 0
need_reset_recoil_start_state = True
self.is_shooting = True
```

#### 4.SpellStart()

self.spell_start = True 意为当前要开火，但可能子弹还没到

#### 5.每次射击cue到达：

更新上述信息

#### 6.松开左键时：

将这些信息清零。

### 五.短暂停下

#### 1.松开时会调用TryClearShootIdxOnEndAttack()

读取武器配置里面的ShootIdxResetDelayTime，设置定时器

#### 2,超过定时器，执行清零逻辑

#### 3.没超过定时器，子弹序列继续沿用，但动画已经结束。会重新之前的流程。

#### 子弹命中计算：

每一次spellstrike，会创建一个AsyncFuncNode A  里面记录着当前信息以及执行逻辑（判断到上次检测期间是否子弹命中），以及对应的timer，只有在timer到时后，才后进行对应逻辑的计算。
利用的是协程：

```
某个定时器到期
→ 主线程恢复对应Generator
→ 计算一小段弹道
→ 再次yield
→ 主线程继续做其他事情
```

##### AsyncFuncNode：子弹对象，保存Generator（局部信息如上一次时间，方向，当前速度，命中信息）

##### 创建子弹初期，会进行一个0.01s的预测飞行计算。

##### 命中信息：命中后生成SpellResult→ SpellResult交给伤害系统，服务器进行校验。

## 服务器做了什么

### 左键输入和意图判断

服务器完全不参与

### 进入开火状态：

#### TryAttack_gun_spell 通过后：

调用：CallServer('EnterIsShootingStatus', True)，用于状态同步。

### 调用spell函数，播放 Graph

服务器完全不参与

### Cue到达后：

扣弹信息，服务器进行校验

### 异步子弹命中：

命中信息，服务器校验，同步

## shoot direction

如何判断一颗子弹的最终方向：

### 步枪：

#### 纯正方向：镜头到屏幕中心

Basedirection

#### 散射：

在之前的基础上，通过读取枪支配置中，当前状态、连发次数，对应的散射参数，获得结果，如1.2,1.5，最终的散射半径就是1.8
所以两次随机：第一次随机，定下圆上的方向，第二次随机，随机一个半径。从而得到最终的射击方向。

```
两次随机数：
randU → theta：绕准星方向旋转的方位角
randV → phi：偏离准星中心的角度
```

### 散弹枪：

#### 纯正方向：镜头到屏幕中央。

#### 散射1：

根据散射参数，计算出现有的散射倍数，是内环，外环共同的参数。

#### 内环： 对于不同子弹，分别做一次类似于步枪普通子弹的散射。

#### 外环：先将子弹在外环上均匀分布，随后每个子弹做一个r/2圆环的扰动

## 设计命中判断：

### 1.在射击管线中，每次cue到达后，会创建个异步事件+定时器

#### 异步事件：记录当前子弹的信息，以及判断一段的命中。

```
bullet_fly_speed          # 当前标量速度
bullet_fly_speed_v3       # 当前速度方向和大小
bullet_fly_range          # 最大射程
gravity                   # 重力
bullet_acceleration       # 速度衰减
start_pos                 # 当前分段起点
cur_fly_dist_v3           # 本次分段位移
cur_bullet_dist           # 已累计飞行距离
cur_time                  # 上次更新时间
penetrate_handler         # 剩余穿透能力
bullet_exclude_actor      # 已经命中过、需要排除的对象
```

命中函数：根据起点，终点，连线与世界求命中

##### 求命中方法：

```
对场景物理世界做一次AllRaycast
对角色骨骼受击盒做一次Raycast
合并所有结果，按照距离从近到远排序
依次判断材质、敌友和穿透
生成ShootEntityResult
```

##### 命中后：

构造spellresult，上传给服务器，服务器校验。

### 定时器：只有当过了一定时间，才会允许命中函数的计算

开始时，会算一个10ms的， 随后，每33ms让命中函数调用一次
