# 3.2 UE 运行时类型检查

<!-- course-navigation:start -->
[上一节：3.1 UE 垃圾回收与反射](garbage-collection-and-reflection.md) · [本章目录](README.md) · [下一节：3.3 UE 动画系统概览](animation-overview.md) · [手册首页](../README.md)
<!-- course-navigation:end -->

UE 的 `UObject` 类型检查依赖反射系统。先分清三个问题：某个 C++ 类型对应什么元数据、一个对象实际是什么类型、这个对象能否转换为目标类型。

## 3.2.1 StaticClass 与 GetClass

`UMyObject::StaticClass()` 返回该反射类对应的 `UClass*`；`Object->GetClass()` 返回对象实际类型的 `UClass*`。`StaticClass()` 是生成代码提供的静态接口，不是每个实例里各存一份函数。

原笔记用 `type_info` 指代类型信息，容易与 C++ 标准 RTTI 混淆。标准库的 `std::type_info` 配合 `typeid` 使用；UE 的 `UClass`、`IsA`、`Cast` 是另一套机制，不能把二者实现混为一谈。

### 3.2.1.1 同一个对象，静态类型和实际类型可以不同

假设 `ABotCharacter` 继承 `AMyCharacter`，以下为 UE 工程片段，Bot 已由有效的 World 生成且仍存活：

```cpp
AMyCharacter* Character = Bot; // Bot 的类型为 ABotCharacter*
UClass* DeclaredClass = AMyCharacter::StaticClass();
UClass* ActualClass = Character->GetClass();
// DeclaredClass 描述 AMyCharacter；ActualClass 描述 ABotCharacter。
```

变量声明限制“通过这个表达式能调用什么”，不会切掉对象的派生部分。`Character->GetClass()==AMyCharacter::StaticClass()` 只匹配精确类型，不能替代“属于此类或其派生类”的判断。

## 3.2.2 IsA 判断继承关系

```cpp
if (Object && Object->IsA(AMyActor::StaticClass()))
{
    // Object 的实际类型是 AMyActor，或派生自 AMyActor。
}
```

概念上，`IsA` 判断实际类型的继承链中是否包含目标类。对 `UObject` 类继承而言，这是一条父类链，不需要把它描述为一般图结构的 DFS。引擎版本和编译配置可以用继承链缓存等方式加速，不能据此保证每次都线性向上遍历。

## 3.2.3 Cast 在检查后返回目标指针

```cpp
AMyActor* Actor = Cast<AMyActor>(Object);
if (Actor)
{
    // 类型兼容，使用 Actor。
}
```

对常见的 UObject 到 UObject 转换，可以用“先检查类型兼容，再返回转换后的指针”理解；失败时返回 `nullptr`。源码还针对类标记和接口转换等情况分派实现，因此“固定调用一次 IsA，再 static_cast”只是教学简化。`ClassCastFlags` 也主要应放在这些转换快路径的上下文里理解。

`Cast` 不是悬空指针检测器，也不替代生命周期管理。指针是否仍指向有效对象，需结合 [GC 与对象引用](garbage-collection-and-reflection.md)处理。

### 3.2.3.1 沿一次拾取交互使用类型检查

```cpp
// UE 工程片段：OverlapActor 来自当前有效的重叠查询结果。
if (APickupActor* Pickup = Cast<APickupActor>(OverlapActor))
{
    Pickup->RequestPickup(); // 项目接口：提交请求，服务器继续验证距离/权限
}
```

碰到墙壁时 Cast 失败，碰到 `APickupActor` 的派生弹药箱时成功。转换成功只证明类型关系，不证明玩家有权限拾取，也不证明物品尚未被其他人领取；类型检查、生命周期检查和业务校验必须分别成立。

## 3.2.4 学习检查：精确相等、继承兼容与业务许可

给定 `AActor → APickupActor → AAmmoPickup`，依次判断 `AAmmoPickup` 对象的 GetClass 相等、IsA 和 Cast 结果。随后解释：为什么即使三个检查都符合预期，服务端仍可能拒绝拾取？

官方接口：[UObjectBaseUtility::IsA](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/CoreUObject/UObjectBaseUtility/IsA)、[Cast](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/CoreUObject/Cast)。
