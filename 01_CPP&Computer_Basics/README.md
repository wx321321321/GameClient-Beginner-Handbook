# C++ 与计算机基础

这一章收录 C++、计算机基础及游戏开发相关的学习笔记。按下面的专题查找，也可以依次阅读语言基础、标准库、运行机制和系统知识。

## C++ 语言与对象

| 笔记 | 主要内容 |
| --- | --- |
| [指针与引用](pointers-and-references.md) | 指针、引用、值类别、移动语义、完美转发、智能指针与 RAII |
| [C++ 关键字与可调用对象](cpp-keywords-and-callables.md) | 类型转换、`sizeof`、`static`、`inline`、Lambda、`std::function`、`union` |
| [类、继承与多态](classes-inheritance-and-polymorphism.md) | 类对象布局、生命周期、继承、虚函数、虚表与虚继承 |
| [模板](templates.md) | 模板实例化与特化、类型推导、SFINAE、`requires`、CRTP |

## 标准库、算法与设计

| 笔记 | 主要内容 |
| --- | --- |
| [STL 与常用算法](stl-and-algorithms.md) | 容器、迭代器、算法、适配器、分配器、数据结构与寻路算法 |
| [设计模式](design-patterns.md) | 单例、工厂、策略、观察者与事件系统 |
| [欧拉角与四元数](rotations-and-quaternions.md) | 旋转表示、万向节锁、插值与 UE 的变换数据 |

排序、查找和区间修改收录在 [STL 的算法与容器配合](stl-and-algorithms.md#算法与容器配合)中；原先空白的 `sort.md` 已移除。

## 内存、硬件与操作系统

| 笔记 | 主要内容 |
| --- | --- |
| [计算机组成原理](computer-architecture.md) | CPU、数值表示、存储层次、缓存一致性、指令、中断与异常 |
| [内存模型与编译管线](memory-and-compilation.md) | 地址转换、页表、编译链接、程序装入、内存分区与分配 |
| [操作系统](operating-systems.md) | 系统调用、内核、文件组织、文件共享与空闲空间管理 |
| [进程、线程与同步](processes-threads-and-synchronization.md) | 进程与线程、调度与通信、生命周期、锁与条件变量、死锁、原子操作、内存序与 UE 任务 |

组成原理从硬件解释缓存与指令，内存笔记串起地址转换、编译装入和对象存储；指针笔记解释所有权与 RAII，线程笔记进一步解释共享状态的同步。

## 网络与脚本

| 笔记 | 主要内容 |
| --- | --- |
| [计算机网络](computer-networks.md) | 网络分层、IP、TCP、UDP、KCP、协议选择与 RPC |
| [Lua](lua.md) | 变量与 table、闭包、元表、协程、C++ 交互、热更新与 GC |

## 图片与维护

- 图片统一保存在 [assets](assets/) 中，正文使用相对链接。
- 原 `image-11.png` 与 `image-12.png` 内容完全一致，已合并为 [空闲空间链表示意图](assets/free-space-linked-list.png)。
- 原未被正文引用的 `image-20.png` 保留为 [补充素材](assets/supplemental-20.png)。
- 文件名统一使用小写英文和连字符；每篇只保留一个一级标题。
- 新增内容时优先补充现有专题；新增独立笔记时同步更新本导航。

[返回手册首页](../README.md)
