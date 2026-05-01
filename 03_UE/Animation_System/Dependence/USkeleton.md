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




## USkeleton：骨骼资产

#### 父类：
UObject：一个普通 UE 资产对象，有反射、序列化、GC、编辑器资产生命周期
IInterface_AssetUserData：它可以挂自定义用户数据。
IInterface_PreviewMeshProvider：编辑器里 Skeleton 资产打开后看到的预览模型有关

### 成员：
#### BoneTree：主要是记载每块骨骼的平移重定向方式
TArray<struct FBoneNode> BoneTree;
#### ReferenceSkeleton：
FReferenceSkeleton ReferenceSkeleton;是参考骨骼的详细信息：
##### FMeshBoneInfo：原始资源里的骨骼信息
骨骼名 Name  父骨骼索引 ParentIndex  导出名等信息
##### TArray<FMeshBoneInfo> RawRefBoneInfo;记录所有原始骨骼信息
##### TArray<FTransform> RawRefBonePose;对应骨骼的初始变换
FTransform：平移、缩放作为vector保存，旋转作为四元数保存。
##### TArray<FMeshBoneInfo> FinalRefBoneInfo;最终骨骼表（初始+虚拟骨骼）
##### TArray<FTransform> FinalRefBonePose;  最终骨骼表里每根骨骼的参考姿势
##### TMap<FName, int32> RawNameToIndexMap;原始名字查找表
##### TMap<FName, int32> FinalNameToIndexMap;最终名字查找表
##### mutable TArray<int32> CachedEndOfBranchIndicesRaw：缓存每根 Raw 骨骼对应“分支末尾”的索引
##### TArray<FBoneIndexType> RequiredVirtualBones:哪些虚拟骨骼是需要参与构建/计算的
##### TArray<FVirtualBoneRefData> UsedVirtualBoneData:虚拟骨骼计算时需要用到的辅助数据
FVirtualBoneRefData:里面存着当前虚拟骨骼序号、头骨骼序号、尾骨骼序号。
#### FGuid Guid： 资源的GUID
#### FGuid VirtualBoneGuid：虚拟骨变化单独用一个 Guid 跟踪，不想因为只改了 Virtual Bone 就把整个 Skeleton 的主 Guid 搞脏。

#### Virtual Bones：存所有的虚拟骨骼
TArray<FVirtualBone> VirtualBones;
在 USkeleton 里有一整套管理接口：
```cpp
AddNewVirtualBone
AddNewNamedVirtualBone
RemoveVirtualBones
RenameVirtualBone
HandleVirtualBoneChanges
GetVirtualBones
GetVirtualBoneGuid
```

#### Compatible Skeletons：兼容骨骼
TArray<TSoftObjectPtr<USkeleton>> CompatibleSkeletons：这个 Skeleton 可以接受哪些别的 Skeleton 作为动画来源。
使用TSoftObjectPtr原因：不希望一个 Skeleton 把所有兼容 Skeleton 全部硬加载进内存。
bool bUseRetargetModesFromCompatibleSkeleton:如果当前动画来自兼容 Skeleton，平移 retarget 模式是否优先使用“源 Skeleton”的设置，而不是当前 Skeleton 自己的设置。
以及一系列管理接口。

#### Sockets：挂点系统
TArray<TObjectPtr<class USkeletalMeshSocket>> Sockets;记录所有socket
#### Retarget Sources：参考姿势库
TMap<FName, FReferencePose> AnimRetargetSources;
#### Curve MetaData：曲线数据库相关
#### Blend Profiles：融合配置中心：
可复用的 Skeleton 级混合配置资产内嵌对象：Blend Profile 常见用途是：每根骨骼混合权重不同，某些骨骼融合更快，某些骨骼融合更慢
#### Slot Groups：Montage Slot 管理中心
TArray<FAnimSlotGroup> SlotGroups;
TMap<FName, FName> SlotToGroupNameMap;//快速查找组

#### 预览器相关
预览网格、动画等


#### AnimationNotifies / Marker Names：动画语义登记
TArray<FName> AnimationNotifies;这个是编辑器缓存，用来记录 Skeleton 相关动画里见过的 Notify 名。
TArray<FName> ExistingMarkerNames;这个用于 Sync Marker。也就是动画同步系统里的 marker 名称缓存


#### Linkup Cache：Skeleton 和 Mesh 骨骼索引映射缓存
FTransactionallySafeRWLock SkinnedAssetLinkupCacheLock;
TMap<TObjectKey<USkinnedAsset>, TUniquePtr<FSkeletonToMeshLinkup>> SkinnedAssetLinkupCache;



## 一些附加知识：
### GUID：
128位：
```
    uint32 A;
    uint32 B;
    uint32 C;
    uint32 D;
```
FName 很适合“按名字访问”，但它不是稳定身份 ID。


UE 里很多“需要长期稳定跟踪”的东西，会额外配 GUID。比如UBlueprint里最典型的场景就是：
蓝图变量 / 属性 / 节点 / 成员引用
如果一个蓝图变量改名了：只靠 Name，旧引用容易断
有 GUID，就可以知道“虽然名字变了，但还是原来那个成员”
### FName：是 UE 用来做“高效名字标识”的轻量类型
FName 会把名字放进全局名字表（name table）一类的结构里，实例本身更像是在保存索引/编号，而不是每次都完整保存一份字符串
```
FName BoneName = TEXT("pelvis");
触发：
"pelvis"
  ↓
去全局名字表里查
  ↓
如果已经有这个名字，就复用那个条目
如果没有，就新建一个条目
  ↓
FName 里保存“这个名字条目的索引/引用” + 实例编号
```
FName 比较快，是因为引擎可以直接检查它们的 index values 是否匹配，而不需要逐字符比较
#### FName：
用于标识符、名字、查找键。
强项是高效比较和作为 key 使用。
不适合做复杂字符串编辑，也不适合直接做面向用户的显示文本。

#### FString：
用于真正的字符串内容处理。
适合拼接、截取、搜索、格式化。

#### FText：
用于给用户显示的文本。
重点是本地化、UI 展示、多语言

注意：FString -> FName 的转换是有损的，因为 FName 是 case-insensitive。也就是说它不适合把“大小写敏感文本”当普通字符串那样处理

### TMap：
```
TMap<FName, int32> Map;
Map.Add(TEXT("pelvis"), 0);
Map.Add(TEXT("head"), 5);
Map.Find(TEXT("head"));
```
对 "head" 求哈希值
哈希值映射到某个 bucket
看这个 bucket 对应链上有哪些元素
用 operator== 比较 Key
找到后返回 Value 地址
### TArray：UE 版 std::vector


### UE指针：

#### UObject体系：
##### TSoftObjectPtr<T>：指向某个 UObject/资产的软引用
它不要求对象当前已经加载到内存，可以先只记住“它是谁”，等需要的时候再加载。Soft Pointer 本质上是对象/资产路径的字符串表示，并且内部还会额外保存一个 Weak Pointer 缓存。原因：因为如果对象已经加载了，每次都靠路径去解析对象，会比较慢。所以内部会缓存一个“当前内存里那个对象”的弱引用。
没加载时，主动加载：
```
USkeletalMesh* Mesh = PreviewMesh.LoadSynchronous();//或者用 FStreamableManager / LoadAsync 做异步加载
```
##### UObject*
一个普通原始指针语义，不自带“软加载”“弱引用”“类型安全类引用”这些额外能力，在 UE5 里都改成了 TObjectPtr，并建议在 UCLASS/USTRUCT 的 UObject 属性和容器里优先使用 TObjectPtr<T> 而不是原始 T*
##### TObjectPtr<T>：UE5 里更“引擎感知”的 UObject 成员指针写法
更适合 UE5 的对象系统、反射、GC、增量 GC 等演进方向，当前应该已经在内存里的对象
##### TWeakObjectPtr<T>：
指向这个 UObject，但不想因此把它保活，只能弱引用已经在内存里的对象。
##### TStrongObjectPtr<T>
获取一个对 UObject 的强引用计数，在这个 guard/作用域内防止它被 GC
##### TSoftClassPtr<T>
TSoftClassPtr 是对 FSoftObjectPtr 的模板包装，指向的是类，不是对象实例

#### SmartPointer：是给 普通 C++ 对象 用的
![](E:/GameClient-Beginner-Handbook/images/03/SmartPointer.png)

### RetargetManager
已被替代：如果你是 UE5 新项目，尤其是不同体型、不同骨架、Mixamo/Mannequin/MetaHuman 这类跨角色复用，优先用 IK Rig / IK Retargeter。这是官方当前主推的重定向工具链