# UE 运行时类型检查

[返回本章目录](README.md) · [返回手册首页](../README.md)

UE 的 `UObject` 类型检查依赖反射系统。先分清三个问题：某个 C++ 类型对应什么元数据、一个对象实际是什么类型、这个对象能否转换为目标类型。

## StaticClass 与 GetClass

`UMyObject::StaticClass()` 返回该反射类对应的 `UClass*`；`Object->GetClass()` 返回对象实际类型的 `UClass*`。`StaticClass()` 是生成代码提供的静态接口，不是每个实例里各存一份函数。

原笔记用 `type_info` 指代类型信息，容易与 C++ 标准 RTTI 混淆。标准库的 `std::type_info` 配合 `typeid` 使用；UE 的 `UClass`、`IsA`、`Cast` 是另一套机制，不能把二者实现混为一谈。

## IsA 判断继承关系

```cpp
if (Object && Object->IsA(AMyActor::StaticClass()))
{
    // Object 的实际类型是 AMyActor，或派生自 AMyActor。
}
```

概念上，`IsA` 判断实际类型的继承链中是否包含目标类。对 `UObject` 类继承而言，这是一条父类链，不需要把它描述为一般图结构的 DFS。引擎版本和编译配置可以用继承链缓存等方式加速，不能据此保证每次都线性向上遍历。

## Cast 在检查后返回目标指针

```cpp
AMyActor* Actor = Cast<AMyActor>(Object);
if (Actor)
{
    // 类型兼容，使用 Actor。
}
```

对常见的 UObject 到 UObject 转换，可以用“先检查类型兼容，再返回转换后的指针”理解；失败时返回 `nullptr`。源码还针对类标记和接口转换等情况分派实现，因此“固定调用一次 IsA，再 static_cast”只是教学简化。`ClassCastFlags` 也主要应放在这些转换快路径的上下文里理解。

`Cast` 不是悬空指针检测器，也不替代生命周期管理。指针是否仍指向有效对象，需结合 [GC 与对象引用](garbage-collection-and-reflection.md)处理。

官方接口：[UObjectBaseUtility::IsA](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/CoreUObject/UObjectBaseUtility/IsA)、[Cast](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/CoreUObject/Cast)。
