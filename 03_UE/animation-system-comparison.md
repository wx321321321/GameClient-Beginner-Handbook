# 3.4 UE 与弥赛亚动画系统对比

<!-- course-navigation:start -->
[上一节：3.3 UE 动画系统概览](animation-overview.md) · [本章目录](README.md) · [下一节：4.1 从零规划 Arena 射击项目](../04_Projects/shooter-scope-and-architecture.md) · [手册首页](../README.md)
<!-- course-navigation:end -->

这篇笔记围绕一个问题：动画里记录的某根骨骼，怎样映射到某张网格，并最终写进当前帧姿势？UE 部分使用公开类型说明；弥赛亚的 `.skeleton`、`SkinSkeleton`、`SkeletonData`、`Pose::bones_` 等名称来自原项目记录，具体字段和资源格式仍需对应版本源码核对。

## 3.4.1 骨架资产与网格骨架

一根骨骼最基本的信息包括名称、父骨骼索引和参考姿态。层级逻辑是一棵树，运行时则可以用扁平数组加父索引保存；“树结构”不意味着必须逐节点分配、用指针递归访问。

| 层次 | UE | 弥赛亚原项目记录 |
| --- | --- | --- |
| 可共享的动画骨架 | `USkeleton` | `.skeleton` / `SkeletonData`，原稿记录为 XML 资源 |
| 具体网格的骨架 | `USkeletalMesh` 的 `RefSkeleton` | `SkinSkeleton` |
| 当前求值的姿势 | `FCompactPose` | `Pose::bones_` |

这是一种职责对照，不表示类内部结构一一等同。共享骨架提供动画的关联基准，网格骨架提供这张网格的骨骼及参考姿态。UE 的网格还会引用相应 `USkeleton`；不能理解成每个 Mesh 都复制共享 Skeleton 的全部数据。

原稿把 Socket、Retarget 信息统一归到 Mesh 的完整 Skeleton 中，容易混淆：这些信息具体属于 Skeleton、Mesh 还是其他重定向资源，取决于信息种类与版本，不能用一个“完整骨架”概括。

## 3.4.2 同一根骨骼的三套索引

UE 动画处理中至少要区分：

```text
Skeleton 索引：共享骨架中的位置
Mesh 索引：当前网格 RefSkeleton 中的位置
Compact Pose 索引：本次求值所需骨骼的紧凑位置
```

三者的数字不一定相同。一个动画轨道记录 `Hand_R` 在共享骨架中的索引 42，不代表当前网格的第 42 个元素，也不代表当前姿势的第 42 个元素。

`FBoneContainer` 为当前资产和所需骨骼组织运行时映射。`RequiredBones` 与当前 LOD 等条件有关，还需要满足骨骼父链等求值依赖。`FCompactPose` 保存这次求值所需骨骼的局部变换，不是每帧把完整骨架复制后再随意删元素。

| 原笔记中的映射名 | 应理解的方向或内容 |
| --- | --- |
| `BoneIndicesArray` | Compact Pose 索引 → 当前资产的 Mesh Pose 索引 |
| `CompactPoseParentBones` | 每个 Compact Pose 骨骼的父骨骼索引，不是父骨骼“坐标” |
| `CompactPoseToSkeleton` | Compact Pose 索引 → Skeleton 索引；具体成员名可能为 `CompactPoseToSkeletonIndex` |
| `SkeletonToCompactPose` | Skeleton 索引 → Compact Pose 索引；未参与本次求值的骨骼可能没有有效映射 |
| `SkeletonToMeshTable` | 原笔记对 Skeleton → Mesh 映射的称呼 |
| `MeshToSkeletonTable` | 上一映射的反向关系 |

最后两项保留为阅读源码时的搜索线索，不承诺每个 UE 版本都使用这两个成员名。应优先使用类型明确的索引和转换接口，避免混用裸 `int`。官方 [`FBoneContainer` API](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/FBoneContainer)列出了骨骼集合、父索引和转换接口。

## 3.4.3 动画轨道如何进入当前姿势

### 3.4.3.1 用数组走一遍映射

假设本次求值保留 Mesh 骨骼 0、2、5，分别放在 Compact Pose 的 0、1、2。这里假设所需父链已包含在集合里，数字只是教学数据。

```cpp
#include <array>
constexpr std::array<int, 3> compactToMesh{0, 2, 5};
constexpr std::array<int, 6> meshToCompact{0, -1, 1, -1, -1, 2};
// Mesh 5 -> Compact 2；Mesh 3 -> -1，表示本次没有对应姿势元素。
```

向 Mesh 5 写姿势前先查到 Compact 2，再检查结果有效；直接访问 `compactPose[5]` 会越过只有 3 个元素的数组。LOD 改变后，映射也可能改变，缓存的 Compact 索引必须随所需骨骼集合一起更新。真实代码应使用引擎的类型化索引和转换 API。

再看局部变换如何变成组件空间位置。先只保留一维平移，父节点按前序排列：

```cpp
std::array<int, 3> parent{-1, 0, 1};
std::array<float, 3> local{10, 2, 3};
std::array<float, 3> component{};
for (int i = 0; i < 3; ++i) {
    component[i] = parent[i] < 0 ? local[i] : component[parent[i]] + local[i];
}
// 结果为 10、12、15；末端的局部 3 是相对父骨骼，不是相对组件原点。
```

这个例子只允许平移，完整 TRS 必须组合父子变换，不能直接相加旋转角和缩放。组件空间姿势得到后，仍需结合逆绑定变换才能用于蒙皮，不能把这里的 15 直接当作顶点位移。可运行版本见[动画数据实验](../examples/course/animation-data-labs.cpp)。

原笔记使用下面的结构描述动画轨道。它是便于理解的逻辑示意，不是 UE 通用的实际结构体定义：

```text
AnimationTracks[7] = {
    Name = "Hand_R",
    BoneTreeIndex = 42, // USkeleton 中的索引
    InternalTrackData = Hand_R 的所有关键帧
};
```

一次采样的大致数据关系是：

```text
动画时间
→ 采样所需轨道的平移、旋转、缩放
→ 根据轨道和骨架映射找到目标骨骼
→ 写入 FCompactPose 的局部姿势
→ 与其他姿势混合，执行骨骼控制
→ 沿父子层级计算组件空间变换
→ 更新 USkeletalMeshComponent 的姿势数据
→ 构造蒙皮变换并交给 GPU
```

不是所有压缩格式都逐帧遍历原始 `AnimationTracks` 数组；轨道压缩、缺失轨道的参考姿态回填、LOD 和活动分支都会影响实际工作量。这里要抓住的是**采样产生局部姿势，索引映射决定姿势写到哪里，层级组合决定骨骼最终在哪里**。

原稿“从左到右处理 CompactPose”可理解为按满足父子依赖的骨骼顺序遍历，但不能据此把动画采样、姿势混合、组件空间转换和蒙皮上传合成一个步骤。变换与逆绑定矩阵的关系见[平移、旋转、缩放与蒙皮](../02_Graphics/transforms-and-skinning.md)。

读完自查：Mesh 3 不在当前 LOD 的 Compact Pose 里时，为什么不能硬写数组？一维例子把根节点从 10 改成 20 后，组件空间位置应为 20、22、25，但其余局部值仍为 2、3。能解释这两点，再去查真实引擎的映射类型和动画压缩代码。
