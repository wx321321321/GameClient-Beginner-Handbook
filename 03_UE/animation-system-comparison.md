# UE 与弥赛亚动画系统对比

[返回本章目录](README.md) · [返回手册首页](../README.md)

## 骨骼：

### 骨骼内容：

一块骨骼最需要的基本信息是：骨骼名，父骨骼，绑定姿势。

#### 弥赛亚： XML形式

#### UE UE资产

但二者读到内存中，都会是扁平数组。

#### 骨骼资产：

##### 弥赛亚：

.skeleton= 完整动画骨架        SkinSkeleton= 某张网格实际使用的骨骼

##### UE

USkeleton= 多个角色共享的动画骨架   USkeletalMesh.RefSkeleton= 这张具体网格自己的骨骼集合

完整动画骨架负责“算姿态”；网格骨架负责“挑出这张网格需要哪些骨骼”。
但mesh里面也要保存完整的skeleton，因为如socket，retarget等信息存在里面。

## UE动画系统：

### Skeleton	共享骨架的骨名、层级和统一骨骼编号

即骨骼文件。

### Mesh.RefSkeleton	这个具体Mesh的骨骼数组、父骨下标、参考姿态

即mesh内部，需要的所有骨骼，当然，mesh会有个指针指向完整的skeleton

### mesh：

#### FBoneContainer  负责当前Mesh、当前LOD所需骨骼的运行时映射

##### RequiredBones里面是所有需要计算的mesh骨骼列表。

##### FCompactPose：当前帧姿态，对应requiredBones里面的骨骼

##### CompactPoseParentBones：对应compactpose中的父骨骼坐标

##### BoneIndicesArray：CompactPose下标 → Mesh.RefSkeleton下标

##### CompactPoseToSkeleton[compactIndex] = skeletonIndex;

##### SkeletonToCompactPose[skeletonIndex] = compactIndex;

#### 计算：

从左到右处理compactpose，随后写入SkeletalMeshComponent角色实例的骨骼变换数组，交给GPU进行蒙皮

##### SkeletonToMeshTable：从uskeleton的下标到mesh的下标

创建SkeletonToCompactPose

##### MeshToSkeletonTable：与上面的相反

创建CompactPoseToSkeleton

### 动画：

动画文件其实是，一个AnimationTracks数组。

#### 内容：

```text
AnimationTracks[7] = {
    Name = "Hand_R",
    BoneTreeIndex = 42, //USkeleton内的下表
    InternalTrackData = Hand_R的所有关键帧
};
```

每一帧采样所有的animationtrack，从而算出对应骨骼的关键帧。

### 弥赛亚：

| UE | 弥赛亚 |
| --- | --- |
| `USkeleton` | `SkeletonData` |
| `Mesh.RefSkeleton` | `SkinSkeleton` |
| `FCompactPose` | `Pose::bones_` |
