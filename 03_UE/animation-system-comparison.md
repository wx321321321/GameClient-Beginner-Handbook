# UE 与弥赛亚动画系统对比

[返回本章目录](README.md) · [返回手册首页](../README.md)

这篇笔记围绕一个问题：动画里记录的某根骨骼，怎样映射到某张网格，并最终写进当前帧姿势？UE 部分使用公开类型说明；弥赛亚的 `.skeleton`、`SkinSkeleton`、`SkeletonData`、`Pose::bones_` 等名称来自原项目记录，具体字段和资源格式仍需对应版本源码核对。

## 骨架资产与网格骨架

一根骨骼最基本的信息包括名称、父骨骼索引和参考姿态。层级逻辑是一棵树，运行时则可以用扁平数组加父索引保存；“树结构”不意味着必须逐节点分配、用指针递归访问。

| 层次 | UE | 弥赛亚原项目记录 |
| --- | --- | --- |
| 可共享的动画骨架 | `USkeleton` | `.skeleton` / `SkeletonData`，原稿记录为 XML 资源 |
| 具体网格的骨架 | `USkeletalMesh` 的 `RefSkeleton` | `SkinSkeleton` |
| 当前求值的姿势 | `FCompactPose` | `Pose::bones_` |

这是一种职责对照，不表示类内部结构一一等同。共享骨架提供动画的关联基准，网格骨架提供这张网格的骨骼及参考姿态。UE 的网格还会引用相应 `USkeleton`；不能理解成每个 Mesh 都复制共享 Skeleton 的全部数据。

原稿把 Socket、Retarget 信息统一归到 Mesh 的完整 Skeleton 中，容易混淆：这些信息具体属于 Skeleton、Mesh 还是其他重定向资源，取决于信息种类与版本，不能用一个“完整骨架”概括。

## 同一根骨骼的三套索引

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

## 动画轨道如何进入当前姿势

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
