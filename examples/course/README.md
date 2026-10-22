# 课程配套实验

先按正文手算输入与输出，再编译运行这些小程序。它们使用终端输出，不创建图形窗口。默认使用 C++17；Lua 交互程序另需 Lua 5.4 开发库。UE 和原项目专用代码片段需要相应引擎工程，不属于这里的独立程序。

## 对照章节选择实验

| 源码 | 对应章节 | 验证什么 | 成功输出或关键结果 |
| --- | --- | --- | --- |
| [intro-labs.cpp](intro-labs.cpp) | 1.1 | 分支、循环、按值传参与对象状态 | health=70、40、0；intro checks passed |
| [systems-labs.cpp](systems-labs.cpp) | 1.2、1.3、1.7、1.10 | 数值与缓存模型、文件偏移、生命周期、线程队列与发布 | systems labs passed |
| [cpp-language-labs.cpp](cpp-language-labs.cpp) | 1.4—1.6、1.8、1.9、1.12 | 指针、类、关键字、模板、STL、设计模式的 26 个实验函数 | 六组 [ok] 结果 |
| [network-labs.cpp](network-labs.cpp) | 1.11 | 长度前缀拆包、快照顺序、请求去重 | frames=abc,de position=3 ammo=0 |
| [lua-bridge.cpp](lua-bridge.cpp) | 1.13 | Lua 调 C++、C++ 调 Lua、参数检查与栈恢复 | lua damage=22 stack restored |
| [math-labs.cpp](math-labs.cpp) | 1.14、第二章 | 短角差、四元数、TRS、蒙皮、透视插值、射线包围盒 | angle=2；skin=11.5；u=1/3 |
| [animation-data-labs.cpp](animation-data-labs.cpp) | 3.3、3.4 | 权重、值快照、骨骼索引映射与父子平移 | animation data checks passed |
| [shooter-labs.cpp](shooter-labs.cpp) | 4.3—4.6、4.10 | 武器冷却与换弹、请求结果缓存、伤害去重、生命版本与时间 | Arena labs: weapon, combat, timing assertions passed. |

另有 [linkage-demo](linkage-demo/main.cpp) 的 `main.cpp`、[counter.cpp](linkage-demo/counter.cpp)、[counter.h](linkage-demo/counter.h)，对应 1.7 的多文件声明、定义和 ODR。

## 编译与运行

在仓库根目录，用已经配置的 C++ 编译器构建。例如支持 C++17 的 GCC/Clang：

```powershell
# 示例输出到已由 Git 忽略的 output 目录
New-Item -ItemType Directory -Force output/course | Out-Null
c++ -std=c++17 -Wall -Wextra -pedantic examples/course/intro-labs.cpp -o output/course/intro-labs.exe
./output/course/intro-labs.exe
```

将文件名换成表中相应源码即可。线程实验在部分 GCC/Clang 平台需加 `-pthread`；MSVC 使用已配置的开发者命令环境和 `/std:c++17 /EHsc`。如果已有 Zig，可用 `zig c++` 替代 `c++`，无须把某一编译器的绝对路径写进源码。

**不要定义 `NDEBUG`。** 这些实验使用 `assert` 检查行为，关闭断言会丢失验证；部分程序对此直接报编译错误。成功退出需要结合断言和实际输出理解，不能把只编译成功当作功能通过。布局大小、字节顺序和具体线程调度等输出随平台变化，按正文说明解释。

系统实验默认运行全部模块，也可以只运行一组：

```powershell
./output/course/systems-labs.exe hardware
./output/course/systems-labs.exe os
./output/course/systems-labs.exe memory
./output/course/systems-labs.exe threads
```

文件实验在自己新建的临时目录内写入测试文件，结束后清理自己创建的文件；不读取或修改你的其他资料。缓存实验是简化地址访问模型，不是对真实 CPU 的性能基准。

## 多文件编译：成功和失败都要解释

```powershell
c++ -std=c++17 examples/course/linkage-demo/main.cpp examples/course/linkage-demo/counter.cpp -o output/course/linkage-demo.exe
./output/course/linkage-demo.exe
```

成功结果为 `hits=2, doubled=4`。加 `-DSYSTEMS_DEMO_BAD_ODR` 后重新编译会在两个翻译单元中产生重复定义，预期链接失败；只编译 `main.cpp` 而不提供 `counter.cpp`，也预期因缺少定义而链接失败。具体错误文本取决于编译器。不要为消除错误随意把每个变量都改成 `static`，先解释链接所需的定义归属。

## Lua 交互程序的额外依赖

`lua-bridge.cpp` 包含 `lua.h`、`lauxlib.h` 和 `lualib.h`。需要 Lua 5.4 的头文件与相同目标平台的库；仅安装 Lua 命令行程序不一定附带开发文件。

```text
c++ -std=c++17 -I <Lua头文件目录> examples/course/lua-bridge.cpp <Lua静态库路径> -o lua-bridge
```

这里的尖括号路径必须替换为实际路径，系统库链接选项也按平台要求补齐。自行构建 Lua 时，将 Lua 源文件按 C 编译，静态库排除自带的 `lua.c`、`luac.c` 两个程序入口，再与 C++ 主程序链接。接口处使用 `extern "C"` 与 C 库匹配。

程序只执行源码内的教学脚本，检查 AddAmmo 的合法/非法输入，并调用 Lua 的伤害函数得到 22，最后确认栈恢复。它不连接游戏服务器，不加载个人 Lua 文件。

## 本轮验证范围

2026-10-22 使用 Windows x64 上的 Zig 0.15.2 / Clang 20.1.2 验证。完整结果见[本轮整理记录](../../docs/numbered-course-review.md)。课程实验验证的是所列纯逻辑，不覆盖 UE 引擎编译、真实联机、GPU 或平台账号服务。

[返回课程首页](../../README.md)
