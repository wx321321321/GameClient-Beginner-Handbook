# 网络同步：
核心思想：同ID实体镜像+AOI管理可见范围+Property同步状态+RPC传递行为+独立的位置同步通道


## 同ID实体镜像：
战斗中，服务器有一个权威的 CombatAvatar，客户端也创建一个同类型、同 EntityID 的代理对象。
它们不是同一个对象，只是通过网络保持部分数据一致。
```
服务器 CombatAvatar
        ↓ 网络同步
客户端 CombatAvatar / PlayerCombatAvatar
```
主控玩家通常是 PlayerCombatAvatar，其他玩家是普通客户端 CombatAvatar。

## AOI
服务器不会把全地图所有实体都发给每个客户端。
玩家进入某个实体的 AOI 范围时：
```
发送实体创建信息
→ 客户端创建对应 Entity
→ 下发该客户端有权看到的初始 Property
```
离开 AOI 后，客户端代理实体会执行 on_out_sight，然后销毁。
所以 ALL_CLIENTS 一般是指：所有能够看到这个实体的客户端，不是全服务器所有人。

## Property 负责同步“状态”

### 注册时：
```
Property("hp", 100, Property.ALL_CLIENTS)
Property("ammo", 30, Property.OWN_CLIENT)
```
注册 Entity 类时，PropertyMetaClass 会把属性名、默认值、同步标记注册到原生 asiocore

### 服务器修改：
```
服务器修改 Property
→ asiocore 发现属性变化
→ 编码 EntityID + 属性索引/路径 + 操作 + 新值
→ 发给符合 flag 的客户端
→ 客户端修改自己的对应 Property
→ ClientEntity.on_setattr
→ 调用 _on_set_hp 等回调
→ 刷新血条、模型等表现
```

### 同步类型：
```
SERVER_ONLY
只存在服务器，不发客户端

OWN_CLIENT
只发给这个实体所属的主控客户端

ALL_CLIENTS
发给 AOI 内能够看到该实体的客户端

SPECTATOR_*
发给对应的观战客户端

PERSISTENT
表示还需要持久化，不等同于网络同步范围
```

## RPC:同步行为
如攻击等

## 位置：高频同步通道
```
主控客户端先即时表现
→ 上传移动结果
→ 服务器检查并保存
→ 广播给其他玩家
→ 必要时服务器拉回主控客户端
```
### 详细步骤：

#### 1.A客户端本地移动：
```
输入系统
→ 角色移动逻辑
→ CharacterController
→ 碰撞、重力、Graph、根运动
→ Entity最终Transform
```
角色每一帧本地更新位置，但posesendercomponent按照固定频率上传：如33ms一次，人多的时候频率可能下降。
所以，服务器和B只知道A上传的离散的采样点，根据这个采样点来做一个预测
##### eg：
A:
```
A第0帧：0m，发送P0
A第1帧：1m
A第2帧：2m，发送P2
A第3帧：3m
A第4帧：4m，发送P4
A第5帧：5m
A第6帧：6m，发送P6
```
网络需要两帧，所以B收到的：
```
B第2帧：收到P0，内容是“A第0帧在0m”
B第4帧：收到P2，内容是“A第2帧在2m”
B第6帧：收到P4，内容是“A第4帧在4m”
```
B在第4帧已经有两个点，但他不会直接跳到P2，而是故意播放A更早的时间线，比如说2帧（帧率不稳定，所以游戏一般维护一个缓冲区间）
```
问题：
A第0帧：0m，发送P0
A第1帧：0m
A第2帧：1m，发送P2
A第3帧：2m
A第4帧：1m，发送P4
```
B不知道A到过2m的位置。

#### 2.posesender工作：
每帧更新完位置调用PoseSender
→ 累计发送时间
→ 到达发送间隔
→ 读取最终位置和yaw
→ 判断是否变化
→ 调用area发送回调