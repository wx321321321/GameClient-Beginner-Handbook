# 五章课程整理记录

本轮从 `baaabba` 的内容版本继续整理，目标是让正文按学习依赖逐步深入，并把系统、C++ 和射击项目连起来。整理前已在仓库外保存完整 Git 源码快照，原始笔记可通过历史提交回查。

## 课程结构与编号

五章共 42 篇：第一章 14 篇、第二章 2 篇、第三章 4 篇、第四章 17 篇、第五章 5 篇。原 31 篇全部保留，新增入门程序一篇和 Arena 射击设计十篇。

- 第一章先从能运行的小程序开始，经过硬件、操作系统，再进入指针、对象、关键字、编译内存、泛型、容器、并发、网络、设计、脚本与旋转数学。
- 第二章以数值例子贯通变换、蒙皮与渲染；第三章先讲对象与反射，再讲类型和动画数据流。
- 第四章 4.1—4.10 从零设计 Arena，4.11—4.17 保留原项目实践作为对照。
- 第五章保留原计划、训练规则和题单，补充课程阶段与可检查成果的对应关系。

课程顺序集中在 [course-map.json](../tools/course-map.json)。正文 H1 使用 `1.1` 等篇号，H2/H3/H4 使用对应的层级编号；每篇有上一节、下一节、章目录和首页导航。文件路径保持稳定。

## 每篇具体做了什么

| 节号 | 正文 | 本轮调整 |
| --- | --- | --- |
| 1.1 | [第一个程序](../01_CPP%26Computer_Basics/program-and-cpp-basics.md) | 新增完整扣血程序，逐步解释类型、初始化、分支、循环、函数、对象、整数除法和断言，给出输出与自查答案 |
| 1.2 | [组成原理](../01_CPP%26Computer_Basics/computer-architecture.md) | 串起源程序到硬件执行，补数值表示、字节观察、连续访问和简化缓存模型，区分模型与真实性能 |
| 1.3 | [操作系统](../01_CPP%26Computer_Basics/operating-systems.md) | 从系统职责进入文件，补文件偏移、定长记录与独立打开实例，明确标准库和 OS 的边界 |
| 1.4 | [指针与引用](../01_CPP%26Computer_Basics/pointers-and-references.md) | 先地址、别名、传参，再值类别、移动、转发和所有权；补按值/指针/引用、移动后状态、weak 存活实验 |
| 1.5 | [类与多态](../01_CPP%26Computer_Basics/classes-inheritance-and-polymorphism.md) | 先对象状态和构造析构，再继承、虚调用与虚继承，ABI 布局放到后面；补成员、析构顺序与派发例子 |
| 1.6 | [关键字与可调用对象](../01_CPP%26Computer_Basics/cpp-keywords-and-callables.md) | 通过转换、sizeof、不同行为的 static、闭包捕获解释关键字；分清编译期性质与运行时效果 |
| 1.7 | [编译与内存](../01_CPP%26Computer_Basics/memory-and-compilation.md) | 串起多文件编译链接、地址空间、对象生命周期；补 ODR 正负例、布局与容器存储实验 |
| 1.8 | [模板](../01_CPP%26Computer_Basics/templates.md) | 按实例化、推导、特化、返回类型和进阶约束重排，补数组长度、引用保留与 CRTP 等推演 |
| 1.9 | [STL 与算法](../01_CPP%26Computer_Basics/stl-and-algorithms.md) | 从容器访问需求出发，补 vector、map 插入、rehash、erase-remove、最短路径等实验与边界 |
| 1.10 | [线程与同步](../01_CPP%26Computer_Basics/processes-threads-and-synchronization.md) | 补 join、队列关闭并排空、分离读写与 fetch_add、release/acquire 发布及 async 策略实验 |
| 1.11 | [网络](../01_CPP%26Computer_Basics/computer-networks.md) | 增加半包/多包的长度前缀解码、乱序快照、开火请求去重，用实际输入逐步走状态 |
| 1.12 | [设计模式](../01_CPP%26Computer_Basics/design-patterns.md) | 用对象创建、武器策略、事件观察和断开订阅解释模式的目的、状态和生命周期 |
| 1.13 | [Lua](../01_CPP%26Computer_Basics/lua.md) | 补 C++ 捕获对照、协程返回顺序、Lua 调 C++ 参数检查、C++ 调 Lua 的栈恢复与热更新旧引用 |
| 1.14 | [旋转数学](../01_CPP%26Computer_Basics/rotations-and-quaternions.md) | 明确旋转约定，补跨 ±180° 短角差、90° 四元数旋转、等价符号插值 |
| 2.1 | [变换与蒙皮](../02_Graphics/transforms-and-skinning.md) | 手算缩放→旋转→平移，比较换序结果；用一维绑定姿势和权重得到蒙皮结果 11.5 |
| 2.2 | [渲染与 GPU](../02_Graphics/rendering-and-gpu.md) | 补透视正确插值、深度测试/写入与射线 AABB 的数值案例，说明成立前提 |
| 3.1 | [GC 与反射](../03_UE/garbage-collection-and-reflection.md) | 补强弱引用存活、按 FProperty 读取实例字段、Health 复制到表现回调的 UE 工程片段 |
| 3.2 | [类型检查](../03_UE/runtime-type-checking.md) | 补 StaticClass/GetClass 对照和拾取 Cast 场景，分清类型、对象有效性与业务权限 |
| 3.3 | [动画概览](../03_UE/animation-overview.md) | 用速度 3 推到跑步权重 0.5，补值快照交接；区分 Update/Evaluate 和战斗事件/动画表现 |
| 3.4 | [动画系统对照](../03_UE/animation-system-comparison.md) | 用具体数组解释 Mesh/Compact 映射和缺失索引，再手算父子局部到组件空间的位置 |
| 4.1 | [范围与架构](../04_Projects/shooter-scope-and-architecture.md) | 新增玩法范围、模块输入输出、唯一写入者、接口和开火总时序 |
| 4.2 | [循环与输入](../04_Projects/shooter-runtime-and-input.md) | 新增固定步长、积压、输入边沿、命令历史、实体句柄和角色碰撞移动 |
| 4.3 | [武器与弹道](../04_Projects/shooter-weapons-and-ballistics.md) | 新增武器 FSM、射速、换弹、散布、枪口遮挡、射线/飞行弹和穿透 |
| 4.4 | [战斗与技能](../04_Projects/shooter-combat-and-abilities.md) | 新增伤害身份、护甲、属性来源、Buff、技能事务、死亡与复活 |
| 4.5 | [网络权威](../04_Projects/shooter-network-authority.md) | 新增协议字段、去重、预测重放、插值、命中回溯、快照基线、AOI 与弱网测试 |
| 4.6 | [表现](../04_Projects/shooter-presentation.md) | 新增动画分层、IK、Root Motion、ADS、后座、音效特效池与预测确认/拒绝 |
| 4.7 | [世界与 AI](../04_Projects/shooter-world-and-ai.md) | 新增碰撞层、拾取、导航、感知记忆、行为选择、服务器 AI 与区域加载 |
| 4.8 | [界面与服务](../04_Projects/shooter-ui-data-and-services.md) | 新增 HUD 投影、配置资源、设置存档、登录大厅匹配、重连与外观保存 |
| 4.9 | [性能与工具](../04_Projects/shooter-performance-and-tooling.md) | 新增预算、分位数、关联日志、线程交接、资源制作、构建版本、故障注入和回放 |
| 4.10 | [集成验收](../04_Projects/shooter-integration-milestones.md) | 新增 M0—M5 路线、一发子弹的十步数据变化、接口不变量、排错表与扩展练习 |
| 4.11 | [原开火链](../04_Projects/shooting-pipeline.md) | 保留输入、Spell、Cue 和子弹任务，明确与 Arena 权威时钟设计的区别 |
| 4.12 | [原相机](../04_Projects/camera-and-shooting-direction.md) | 保留双相机、Placer、原参数和弹道，新增课程入口及坐标/单位转换提醒 |
| 4.13 | [原属性](../04_Projects/attributes-and-sync.md) | 保留五字段与回调，明确原公式未知，避免用新教学公式替代原工程事实 |
| 4.14 | [原网络同步](../04_Projects/network-synchronization.md) | 保留 Property/RPC/PoseSender，补具体插值 C++ 推演，并对照位置上报和输入模拟的区别 |
| 4.15 | [原 AOI](../04_Projects/aoi.md) | 保留网格、35/41 滞回和双向表，补有状态的 C++ 距离序列推演 |
| 4.16 | [ALS](../04_Projects/als-locomotion.md) | 保留全部实践和未解决问题，增加前置学习入口，说明复杂运动技术的接入阶段 |
| 4.17 | [飞车经历](../04_Projects/qq-speed-animation-and-ui.md) | 保留演出、换色与时序问题，接入 Lua 和界面服务的课程路线，不改写个人经历 |
| 5.1 | [阶段计划](../05_StudyPlan/phase-1.md) | 区分原复习重点和首次学习顺序，新增六阶段阅读范围与验收成果 |
| 5.2 | [转换速查](../05_StudyPlan/coding-reference.md) | 复读保留完整输入与前缀转换的既有说明，统一编号和导航 |
| 5.3 | [训练提示词](../05_StudyPlan/cpp-cs-interview-drill-prompt.md) | 复读保留出题、评分、追问与复测规则，统一编号和导航 |
| 5.4 | [算法题单](../05_StudyPlan/leetcode-practice.md) | 只调整标题层级编号与导航，保留 22 条待攻克、388 条分日题目及顺序 |
| 5.5 | [错题复盘](../05_StudyPlan/leetcode-review.md) | 保留真实状态分类与空白复盘入口，统一编号和导航，没有编造完成记录 |

## 重复内容如何处理

指针与引用集中解释所有权，类篇集中解释对象生命周期与派发，编译内存篇集中解释存储与链接；其他篇以短例子应用并指回主题。模板进阶放在推导基础之后。网络基础讲消息边界，项目网络讲权威状态与时间；两者相互引用。原项目专有 API、未确认公式和经验参数保留在实践篇，不复制成新教程的通用规则。

## 代码与资料保留

新增 [课程实验目录](../examples/course/README.md)，包含 8 个独立程序和 1 组多文件链接实验。正文中的纯 C++、UE 工程片段、原项目接口和逻辑示意注明适用环境。

原图、补充资源和旧独立复习源码继续保留。编号操作前后对照 386 个代码块、1804 处行内代码和题单 410 条有序记录，内容完全一致；编号只作用于正文标题与必要的导航/链接。

## 验证记录

- 使用 Zig 0.15.2 / Clang 20.1.2，在 Windows x64 以 C++17 编译并运行 7 个标准库实验和 Lua 交互程序。Lua 使用官方 5.4.9 C 源码构建的库。
- 多文件 ODR 正例运行得到 `hits=2, doubled=4`；重复定义和缺少定义两个负例按预期链接失败。
- 编号工具覆盖 42 篇、725 个标题，并迁移 8 处小节链接；第二遍检查为零改动。
- 标准实验中的断言需启用；所有关键行为以实际输出和断言验证，教学上刻意演示的 `sizeof` 不求值等编译警告按例子解释。

- 第四章 13 个标注可独立编译的 C++17 代码块逐个编译为目标文件，全部成功且无编译警告。
- 全仓 55 份 Markdown 的 524 处本地引用通过检查，没有缺失路径或标题锚点，代码围栏完整；43 张原图和 1 份 PDF 的文件哈希与整理前一致，原复习源码文本一致。
- 410 条算法题记录逐条比较，题号、内容及顺序均保留，包括 LCR 题号。整理工具不会把题目序号当作标题编号。
- 五本分册与一本合订本 EPUB 已重新生成：14/2/4/17/5/42 篇。ZIP、XML、资源、目录和书内跳转通过检查；合订本保留 386 个代码块、1804 处行内代码和 103 张表格。
- EPUB 中源码和目录 README 的引用转为“本册未收录”的仓库路径说明；没有真正的缺文件或失配标题警告。未进行 Apple Books 实机阅读测试。
- `git diff --check` 通过。生成的可执行文件、工具链、详细日志和测试中间文件留在仓库外备份目录；EPUB 在已忽略的 `output/AppleBooks_EPUB/`。

## 适用范围

Arena 是从零实施的设计教程，配套实验验证纯规则。仓库没有完整游戏工程；UE 片段未进行引擎工程编译，真实物理、GPU、在线服务、弱网联机和性能预算仍需读者在项目中按验收表验证。原实践依赖具体工程源码的部分继续保留“待核对”标记。
