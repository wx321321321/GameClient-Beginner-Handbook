# 逐篇内容整理记录

整理日期：2026-10-22。正文整理以 `44c7b3f` 为对照基线；更早的目录调整和原路径见[目录整理记录](organization.md)，首次保存的手册原稿位于 `14f58bc`。

这次逐篇通读 31 篇正文，先列出原稿的知识点、例子和项目细节，再调整讲解顺序、合并重复段落、补齐缺失的前提，并修正能够核对的错误。代码片段随解释一起检查；不把原稿缩成提纲，也不把复盘时提出的想法改写成已完成的项目成果。

## C++ 与计算机基础：13 篇

| 笔记 | 内容调整 |
| --- | --- |
| [指针与引用](../01_CPP%26Computer_Basics/pointers-and-references.md) | 按地址与别名、值类别、移动与转发、所有权与 RAII 组织；区分移动转换和实际移动，修正空对象调用与 weak_ptr 的说法 |
| [关键字与可调用对象](../01_CPP%26Computer_Basics/cpp-keywords-and-callables.md) | 将类型转换、存储与链接、内联、调用封装分开；修正继承侧向转换和 inline 的保证，保留 Lambda、function、union 等例子 |
| [类、继承与多态](../01_CPP%26Computer_Basics/classes-inheritance-and-polymorphism.md) | 从对象布局和生命周期进入继承、虚调用与虚继承；区分隐藏、重载、覆盖，以及标准规则与常见 ABI 实现 |
| [模板](../01_CPP%26Computer_Basics/templates.md) | 按实例化、推导、特化、约束和 CRTP 展开；修复示例声明，区分偏特化、重载、SFINAE 与 requires |
| [STL 与常用算法](../01_CPP%26Computer_Basics/stl-and-algorithms.md) | 按容器访问需求、失效规则、迭代器要求、区间算法和数据结构组织；保留红黑树、堆、空间树、编码与寻路，修正容量、rehash、erase-remove 和启发式代价 |
| [设计模式](../01_CPP%26Computer_Basics/design-patterns.md) | 用问题、接口和调用过程解释单例、工厂、策略、观察者及事件系统；补齐退订与生命周期边界，区分通知与网络 RPC |
| [欧拉角与四元数](../01_CPP%26Computer_Basics/rotations-and-quaternions.md) | 区分旋转顺序、角度跨界和万向节锁；补齐单位四元数、双覆盖、插值及 FTransform 的边界 |
| [计算机组成原理](../01_CPP%26Computer_Basics/computer-architecture.md) | 从 CPU 与表示方式进入存储层次、MESI、地址转换、指令和中断；保留数值、缓存状态演算，修正浮点特殊值和硬件泛化结论 |
| [内存模型与编译管线](../01_CPP%26Computer_Basics/memory-and-compilation.md) | 串起虚拟地址、TLB、缺页、编译、链接、装入、运行时布局和对象构造；区分 ELF/PE、存储与生命周期、分配函数与 new 表达式 |
| [操作系统](../01_CPP%26Computer_Basics/operating-systems.md) | 沿系统调用与文件读取组织；区分文件内容的记录结构和磁盘块分配，解释 inode、链接、权限及四类空闲管理方法 |
| [进程、线程与同步](../01_CPP%26Computer_Basics/processes-threads-and-synchronization.md) | 先讲资源和调度，再讲任务生命周期、锁、条件变量、死锁、CAS 和内存序；保留万人场景与 UE 任务讨论，修正 joinable、jthread 和缓存同步的混淆 |
| [计算机网络](../01_CPP%26Computer_Basics/computer-networks.md) | 按寻址、IP、TCP、UDP、KCP、游戏消息和 RPC 组织；将消息分帧放回 TCP，保留包头、握手、队列和乱序编号示例 |
| [Lua](../01_CPP%26Computer_Basics/lua.md) | 按语言规则、作用域、闭包、元表、协程、C API、热更新和 GC 组织；保留十槽回调问题，明确捕获变量、版本和执行状态，修正 _ENV、table 遍历和字符串驻留的概括 |

## 图形学：2 篇

| 笔记 | 内容调整 |
| --- | --- |
| [变换与蒙皮](../02_Graphics/transforms-and-skinning.md) | 从 TRS 与旋转插值进入绑定姿态、逆绑定和线性蒙皮；说明单位四元数前提与 Slerp 最短路径，解释糖果纸效应及改进方案 |
| [渲染管线与 GPU](../02_Graphics/rendering-and-gpu.md) | 先讲坐标与流水线，再讲渲染路径、几何查询、光照阴影、抗锯齿及 GPU；保留高处光源实践与全部配图，修正 AABB 条件、光栅化/深度测试、MSAA 及 Tile 的不同含义 |

## Unreal Engine：4 篇

| 笔记 | 内容调整 |
| --- | --- |
| [动画系统概览](../03_UE/animation-overview.md) | 分清资源、生成类、运行时实例、代理和动画节点，再接入通知、根运动、控制节点与任务依赖 |
| [动画系统对比](../03_UE/animation-system-comparison.md) | 先定义骨架和索引空间，再比较 UE 与弥赛亚的数据流；保留轨道 7、SkeletonData、Pose::bones_ 等具体线索 |
| [垃圾回收与反射](../03_UE/garbage-collection-and-reflection.md) | 分开 GC 可达性、指针所有权、反射生成和属性复制；修正 Outer 保活、裸指针和智能指针适用范围，保留 HP、Jump 与 Lua GC 对照 |
| [运行时类型检查](../03_UE/runtime-type-checking.md) | 以 UClass、StaticClass / GetClass、IsA、Cast 串起流程，纠正 UE 反射元数据与 std::type_info 混用 |

## 项目实践：7 篇

| 笔记 | 内容调整 |
| --- | --- |
| [ALS](../04_Projects/als-locomotion.md) | 按角色/动画/移动、相机、地面步态、Foot IK、预测 IK、Stride Warping、Distance Matching、攀爬与 Motion Warping 组织；保留 C++ 重构、曲线生成器、40 cm/15 cm 参数和撑空问题，区分已做与拟改 |
| [射击框架](../04_Projects/shooting-pipeline.md) | 按输入分发、开火检查、Spell/Cue、散布、异步弹道、命中和服务端职责组织；保留函数、参数与数值例子，指出 ShootIdx 延迟重置流程的原稿矛盾 |
| [相机与射击方向](../04_Projects/camera-and-shooting-direction.md) | 串起两路相机、三棵混合树、Placer 与 RuntimeInfo，再解释采样、瞄具、散布、弹道和霰弹；保留无法确认的原公式并说明核对条件 |
| [网络同步](../04_Projects/network-synchronization.md) | 按实体镜像、AOI、Property、RPC 和位置通道展开；保留 flag、回调、33 ms 与逐帧采样例子，分清网络延迟、插值缓冲和外推 |
| [AOI](../04_Projects/aoi.md) | 明确主体、观察者和反向关系，再解释网格更新与 active/stable；保留 100 ms、网格及 35/41 滞回参数 |
| [属性与同步](../04_Projects/attributes-and-sync.md) | 从属性归属、modifier、服务端修改到客户端回调，另节比较帧/状态/属性同步；保留字段及操作代号映射 |
| [QQ 飞车](../04_Projects/qq-speed-animation-and-ui.md) | 分开 Gamebryo、外骨骼接管、结算 TextKey、FOV 与换色数据流；保留回调、协议和实际故障，修正 FOV 公式、Lua C 调用及 RPC 对比 |

## 学习与面试：5 篇

| 笔记 | 内容调整 |
| --- | --- |
| [第一阶段计划](../05_StudyPlan/phase-1.md) | 把每日节奏、专题进度与六天手写检查分开；保留原任务量，说明题单容量与每日目标的关系 |
| [LeetCode 清单](../05_StudyPlan/leetcode-practice.md) | 增加重点题分类和执行方式；22 条重点记录及 388 条日程题目逐条保留，Day 10–60 内部顺序不变 |
| [LeetCode 复盘](../05_StudyPlan/leetcode-review.md) | 原稿无实际记录；整理可填写的复盘结构与原因分类，不补造完成状态或解题经历 |
| [面试提示词](../05_StudyPlan/cpp-cs-interview-drill-prompt.md) | 分开范围、轮次、出题、评分和重测规则；保留 15/20 题约束，明确“只出题”和错题复测例外，避免流程冲突 |
| [代码速查](../05_StudyPlan/coding-reference.md) | 保留六种解析函数和返回类型，补齐 to_string、pos、base、异常及可核对的小例子 |

## 保留与核对边界

- 原图、独立 C++ 示例与参考 PDF 保留；正文中的教学片段会因修正错误而变化，旧版本可通过基线提交对照。
- LeetCode 题单表示计划与待复习内容，不等同于已完成记录。
- UE 文档与接口随版本变化；私有工程的具体实现、Gamebryo 版本、调用顺序和配置必须结合对应源码。文中将通用机制与原项目记录分开。
- 仍待工程核对的重点包括：ShootIdx 延迟重置、散布原公式、普通枪/霰弹的相机采样顺序、弹道子步、网络权威与可靠性、ALS 曲线单位与根运动调用时序。
- 技术修订所依据的标准草案、Lua 手册、RFC、引擎和平台文档放在对应正文附近。未附完整工程的片段不宣称在该工程中运行通过。

## 完成核验

- 31 篇均有原内容清单与新旧稿对照；对 C++ 类、指针、STL、网络、Lua、旋转和学习计划另做交叉阅读，修正复核发现的 KCP 接收缓存条件遗漏。
- 42 个 Markdown 文件的一级标题、标题层级、代码围栏以及 221 个本地链接和锚点检查通过。
- 原有 43 种独立图片的 SHA-256 全部匹配；独立 C++ 示例和参考 PDF 与整理前备份逐字节一致。
- 22 条重点题记录和 Day 10–60 的 388 条日程题目，题号、题名和顺序逐项一致。
- `git diff --check` 通过。代码片段已人工核对；当前环境未找到 C++ 编译器，未进行整篇片段的编译验证。
- 三个 EPUB 分册与合订本重新构建，分别含 13、2、4、19 篇正文；表格、字面量 `\n`、来源链接和小节跳转已验证。没有缺失文件、失配锚点或结构校验错误。未收录章节的引用转成带仓库路径的文字提示；合订本仍只覆盖前三个分区。未进行阅读器界面测试。

[返回手册首页](../README.md)
