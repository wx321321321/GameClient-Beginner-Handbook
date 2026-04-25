# USkeleton

USkeleton 是 连接 SkeletalMesh 和 Animation 的核心资产，它管理动画使用的骨骼层级、Mesh 与 Skeleton 的骨骼对应关系，以及 Retargeting 相关能力

## 辅助结构体：

### FSkeletonToMeshLinkup
Skeleton 骨骼索引 和 SkeletalMesh 骨骼索引 之间的双向映射表
```cpp
struct FSkeletonToMeshLinkup
{
    TArray<int32> SkeletonToMeshTable;
    TArray<int32> MeshToSkeletonTable;
};
```
USkeleton 和 USkeletalMesh 虽然相关，但它们的骨骼数组不一定一模一样,如人、断臂人。

### EBoneTranslationRetargetingMode：枚举体，骨骼“平移分量”重定向模式
不同角色骨架比例不一样。同一套走路动画拿去给腿长的人和腿短的人会有问题：骨盆高度不对手,脚位置不自然,根部位移不协调，动画看起来拉伸或者压缩。
所以 UE 允许你按骨骼设置：这一根骨骼的 Translation，到底是更相信动画、还是更相信 Skeleton、还是按比例修正。
```cpp
UENUM()
namespace EBoneTranslationRetargetingMode{   
    enum Type : int{
        Animation,//直接使用动画数据中的平移
        Skeleton,//忽略动画里的平移，固定使用 Skeleton 自己参考姿态里的平移
        AnimationScaled,//使用动画平移，但按 Skeleton 比例缩放长度。
        AnimationRelative,//使用动画平移，并把当前 retarget pose 与原姿态的差值当成附加量叠加进去。
        OrientAndScale,//根据参考姿态，应用方向差和缩放差。强修正”的一种。不是简单只缩放长度，而是还考虑：朝向差异尺寸差异
        };
}
```
在 Evaluate 这帧骨骼局部姿势时，取每根骨骼 Translation 的那一刻，就会参考这个模式

### FBoneNode：BoneTree 里每个骨骼节点附带的 Skeleton 级设置
```cpp
USTRUCT()
struct FBoneNode
{
#if WITH_EDITORONLY_DATA
    FName Name_DEPRECATED;//已弃用
    int32 ParentIndex_DEPRECATED;//已弃用
#endif//默认的平移重定向模式是 Animation
    TEnumAsByte<EBoneTranslationRetargetingMode::Type> TranslationRetargetingMode;
};
```
现在更类似于附加信息描述，而默认位置、父节点位置都在参考姿势中


### FReferencePose：一组命名的参考姿态
```cpp
USTRUCT()
struct FReferencePose
{
    FName PoseName;//参考姿态的名字
    TArray<FTransform> ReferencePose;//整套骨骼的参考姿态变换数组。
#if WITH_EDITORONLY_DATA//这套 Reference Pose 是从哪个 SkeletalMesh 来的
    TSoftObjectPtr<USkeletalMesh> SourceReferenceMesh;
#endif
};
```

### FBoneReductionSetting：骨骼精简设置
```cpp
USTRUCT()
struct FBoneReductionSetting
{
    TArray<FName> BonesToRemove;
};
```
一个结构体，存的是需要移除的骨骼名字。常用于LOD设置，存一个这个数组，对应不同等级的LOD

### FNameMapping：名字到名字的映射
```cpp
USTRUCT()
struct FNameMapping
{
    FName NodeName;
    FName BoneName;
};
把某个系统里的“节点标识”映射到 UE 骨骼名。
eg：控制器名映射到骨骼名
做一个简单的控制系统，外部逻辑里用的是这种名字：
HeadCtrlLeftHandCtrlRightFootCtrl
但你角色 Skeleton 里的骨骼名字是：
headhand_lfoot_r，就需要这个结构体来映射
```
### FAnimSlotGroup：槽位数据
```cpp
 USTRUCT()
struct FAnimSlotGroup
{
    static const FName DefaultGroupName;
    static const FName DefaultSlotName;

    FName GroupName;
    TArray<FName> SlotNames;
};
```
### FVirtualBone：虚拟骨骼
```cpp
USTRUCT()
struct FVirtualBone
{
    FName SourceBoneName;
    FName TargetBoneName;
    FName VirtualBoneName;
};
```
 VirtualBoneNameHelpers：统一 Virtual Bone 的命名规则，协助命名


### TArray

### FName



## USkeleton：
