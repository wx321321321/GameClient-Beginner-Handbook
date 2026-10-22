# 第四章 从零设计射击项目

[返回手册首页](../README.md)

4.1—4.10 是新设计的教学射击项目 Arena：从一张测试地图、一把枪和两名玩家开始，逐步接入完整模块。它是设计教程，不是已经完成的可运行游戏。4.11—4.17 保留本人原项目记录，用来比较技术选择和真实问题。

## 阅读顺序

| 节号 | 内容 | 读完要能做到 |
| --- | --- | --- |
| 4.1 | [需求、范围与总体架构](shooter-scope-and-architecture.md) | 定义 Arena 的玩法闭环、模块边界和共享约定 |
| 4.2 | [运行循环、输入与角色移动](shooter-runtime-and-input.md) | 把设备输入变成固定步长的角色命令 |
| 4.3 | [武器状态机与弹道](shooter-weapons-and-ballistics.md) | 完成开火、换弹、散布、射线、投射物和穿透 |
| 4.4 | [伤害、属性与技能](shooter-combat-and-abilities.md) | 处理护甲、修饰器、技能、死亡与结算去重 |
| 4.5 | [联机与服务器权威](shooter-network-authority.md) | 组织预测、校正、插值、回溯和兴趣范围 |
| 4.6 | [动画、相机与音画表现](shooter-presentation.md) | 把事实状态转换成可回滚、可去重的表现 |
| 4.7 | [世界、碰撞与 AI](shooter-world-and-ai.md) | 设计地图交互、导航、感知与服务器 AI |
| 4.8 | [界面、数据与在线服务](shooter-ui-data-and-services.md) | 连接 HUD、配置、资源、存档、大厅和重连 |
| 4.9 | [性能、工具与交付](shooter-performance-and-tooling.md) | 建立预算、线程边界、资源检查和测试链 |
| 4.10 | [集成里程碑与验收](shooter-integration-milestones.md) | 把各模块接成一发子弹和一局游戏的完整流程 |
| 4.11 | [实践对照：原项目开火链路](shooting-pipeline.md) | 回看输入、Spell、Cue 与子弹任务的真实记录 |
| 4.12 | [实践对照：相机与射击方向](camera-and-shooting-direction.md) | 比较双相机、Placer、后座和弹道的项目约定 |
| 4.13 | [实践对照：属性与同步](attributes-and-sync.md) | 追踪原项目 modifier 和属性回调 |
| 4.14 | [实践对照：实体网络同步](network-synchronization.md) | 对照 Property、RPC、PoseSender 的已有方案 |
| 4.15 | [实践对照：AOI](aoi.md) | 理解网格、滞回与双向观察关系 |
| 4.16 | [实践对照：ALS 运动系统](als-locomotion.md) | 深入原实践的相机、IK、步幅、攀爬与根运动 |
| 4.17 | [实践对照：飞车动画与界面](qq-speed-animation-and-ui.md) | 保留动画演出、换色、C++/Lua 和业务时序经历 |

## 分阶段做出什么

- 读完 4.1—4.4：单机测试场能移动、开火、换弹、命中、扣血和重生。
- 读完 4.5—4.7：两端能加入同局，服务器裁定结果，加入动画、AI 和地图交互。
- 读完 4.8—4.10：补齐界面、会话、配置、资源检查、性能与弱网验收。

新设计统一使用米、秒和弧度，示例服务器为 60 Hz；具体例子另有说明时以文内为准。原项目的 33 ms、100 ms、厘米、Cue 和协议字段保留原语境，迁移时先核对单位与权威模型。可运行的算法实验见 [shooter-labs.cpp](../examples/course/shooter-labs.cpp)。
