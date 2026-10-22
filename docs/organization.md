# 目录整理记录

整理日期：2026-10-22。

整理前先完整备份本地工作目录并逐文件校验 SHA-256，再将手册相关原稿提交为 `14f58bc` 并推送到 `origin/main`。本次整理以文件归类、导航、命名和排版为主，保留原有技术内容与代码。

## 目录职责

| 目录 | 存放内容 |
| --- | --- |
| `01_CPP&Computer_Basics/` | C++、计算机基础及现有旋转数学笔记 |
| `02_Graphics/` | 图形学、变换与蒙皮、渲染 |
| `03_UE/` | UE 对象、类型检查与动画系统 |
| `04_Projects/` | 游戏项目实践笔记 |
| `05_StudyPlan/` | 学习计划、练习记录与面试资料入口 |
| `examples/` | 可编译的学习源码 |
| `resources/` | 独立 PDF、参考图片 |
| `tools/` | 笔记导出脚本及依赖说明 |
| `output/` | 可重新生成的导出文件，由 Git 忽略 |
| `.local-archive/` | 与手册无关的本地资料与临时产物，由 Git 忽略 |

## 旧文件名对照

### C++ 与计算机基础

下列文件仍位于 `01_CPP&Computer_Basics/`：

| 原文件 | 当前文件 |
| --- | --- |
| `ComputerOrganizationandArchitecture.md` | `computer-architecture.md` |
| `computenet.md` | `computer-networks.md` |
| `Design_Pattern.md` | `design-patterns.md` |
| `keywords.md` | `cpp-keywords-and-callables.md` |
| `lua.md` | `lua.md` |
| `Memory_Management&Compile_Pipeline.md` | `memory-and-compilation.md` |
| `Multi-State.md` | `classes-inheritance-and-polymorphism.md` |
| `os.md` | `operating-systems.md` |
| `other.md` | `rotations-and-quaternions.md` |
| `Pointer&Reference.md` | `pointers-and-references.md` |
| `STL.MD` | `stl-and-algorithms.md` |
| `Template.md` | `templates.md` |
| `Thread&Lock.md` | `processes-threads-and-synchronization.md` |
| `sort.md` | 原文件为空；排序入口已归入 STL 笔记 |

### 图形学与 UE

| 原路径 | 当前路径 |
| --- | --- |
| `02_Graphics/Generial.md` | 概述并入 `02_Graphics/README.md` |
| `02_Graphics/quarter&eula.md` | `02_Graphics/transforms-and-skinning.md` |
| `02_Graphics/render_pipeline.md` | `02_Graphics/rendering-and-gpu.md` |
| `03_UE/General_View.md` | `03_UE/animation-overview.md` |
| `03_UE/GC.md` | `03_UE/garbage-collection-and-reflection.md` |
| `03_UE/RTTI.md` | `03_UE/runtime-type-checking.md` |
| `03_UE/compare.md` | `03_UE/animation-system-comparison.md` |

### 项目与学习计划

| 原路径 | 当前路径 |
| --- | --- |
| `project/als.md` | `04_Projects/als-locomotion.md` |
| `project/AOI.MD` | `04_Projects/aoi.md` |
| `project/attr.md` | `04_Projects/attributes-and-sync.md` |
| `project/camera.md` | `04_Projects/camera-and-shooting-direction.md` |
| `project/net.md` | `04_Projects/network-synchronization.md` |
| `project/qqfeiche.md` | `04_Projects/qq-speed-animation-and-ui.md` |
| `project/shoot.md` | `04_Projects/shooting-pipeline.md` |
| `StudyPlan/sp-firstphase.md` | `05_StudyPlan/phase-1.md` |
| `StudyPlan/Leetcode.md` | `05_StudyPlan/leetcode-practice.md` |
| `StudyPlan/LeecodeRecode.md` | `05_StudyPlan/leetcode-review.md`，原为空文件，现提供复盘模板 |
| `code.md` | `05_StudyPlan/coding-reference.md` |
| `cpp-cs-interview-drill-prompt.md` | `05_StudyPlan/cpp-cs-interview-drill-prompt.md` |
| `output/cpp-os-review-2026-09-05.cpp` | `examples/cpp-os-review-2026-09-05.cpp` |
| `output/pdf/` | `resources/pdf/` |

## 图片与内容处理

- 各章配图集中到自己的 `assets/`，同步修正正文引用。
- C++ 分区的 `image-11.png` 与 `image-12.png` 完全相同，合并为一份；原稿可从整理前提交恢复。
- 根目录 `images/01/` 的两张图与 C++ 分区原有图片相同，已将根目录副本放入本地归档；独立的 `SmartPointer.png` 保留在 `resources/images/`。
- 保留未被正文引用的独立图片，避免误删待补充素材。
- 每篇使用一个一级标题，修正标题跳级和正文里误粘贴的换行符；保留代码块与协议示例中的合法转义字符。
- LeetCode Day 10 按现有内容标为 4 题，粘连的题目条目拆开；不补造练习记录。
- EPUB 导出脚本同步新文件名，排除导航 README，并补充依赖与使用说明。

## 本地归档与恢复

`.local-archive/` 按用途保留 `projects/`、`personal/`、`documents/`、`tmp/`、`installers/`、`build/`、`duplicate-assets/` 和 `scratch/`。这些资料及其运行环境没有上传到手册仓库，原始文件保留在本地；具体移动记录见归档内的 README。

归档里的独立项目如需继续运行，可先还原其原始目录；虚拟环境若依赖原绝对路径，应还原目录或重新创建环境。不要把归档中的运行依赖混入手册提交。

整理前的手册原稿可从 Git 提交 `14f58bc` 查阅；完整本地备份还包括被 Git 忽略的资料、文件哈希清单和整理前 Git bundle。

## 整理后验证

- 41 个 Markdown 文件均有唯一一级标题，代码围栏闭合，标题无跳级；143 个本地链接及相关锚点检查通过。
- 原有 46 个图片文件中的 43 种独立图片内容全部保留，归档和去重前后通过 SHA-256 对照。
- 原学习源码和参考 PDF 与备份逐字节一致，原笔记代码块也完成了对照检查。
- 三个 EPUB 分册与合订本实际构建通过，分别包含 13、2、4、19 篇正文，构建报告无警告。检查覆盖归档、XML、图片引用和目录链接；未进行阅读器界面操作。

[返回手册首页](../README.md)
