# USkeletonMesh：
是绑定到层级骨骼上的几何体，骨骼可以驱动它做动画变形
USkeletalMesh 是资源资产。
USkeletalMeshComponent 是运行时组件实例。

### 父类：
```
class USkeletalMesh : 
public USkinnedAsset, 属于可蒙皮资源体系，核心是顶点受骨骼驱动变形
public IInterface_CollisionDataProvider, 说明它能提供碰撞三角网格数据
public IInterface_AssetUserData, 它可以挂用户自定义附加数据
public INodeMappingProviderInterface 它可以提供“节点映射数据”，和动画/绑定映射有关
```

## SourceModels：编辑器里的源模型数据
TArray<FSkeletalMeshSourceModel> SourceModels;

### FSkeletalMeshSourceModel：某一个 LOD 的源模型数据容器，每个 LOD 对应一份源模型描述
主要作用：对源模型的 MeshDescription进行管理、备用Skin Weight Profile（皮肤权重）、顶点色信息，以及一些常用的接口，如bounds、定点数等。
```
FSkeletalMeshSourceModel (每个LOD一份)
        ├── 统计缓存
        ├── 兼容旧数据逻辑
        ├── MeshDescription访问接口
        └── USkeletalMeshDescriptionBulkData
```
```cpp
TObjectPtr<USkeletalMeshDescriptionBulkData> MeshDescriptionBulkData;
```
#### MeshDescription：真正顶点相关信息的存储结构。
持有并管理：
```cpp
顶点 Vertices
顶点实例 VertexInstances
UV
Edge
Triangle
Polygon
PolygonGroup
各类属性集 Attributes
各类拓扑索引器 Indexers
顶点位置 VertexPositions
三角形-边-多边形-组之间的连接关系
```
Vertex 和 VertexInstance：一个几何点在不同三角形，uv法线等不同，所以把这些信息绑在顶点实例上。VertexInstance 记录的是：某一次顶点实例，实际对应的是哪个 Vertex
Indexer：快速回答“这个元素和谁相邻/相连”
FMeshDescriptionBulkData：负责管理上述存储结构的序列化内容：FEditorBulkData BulkData
源数据里，每个顶点的骨骼影响权重是 MeshDescription 上的 skin weight attributes，在运行时会打包进 FSkinWeightVertexBuffer

## SkeletalMeshRenderData：运行时真正拿来渲染的数据，其实就是把“这一层 LOD 需要渲染和蒙皮的一切核心 Buffer”都打包起来
TUniquePtr<FSkeletalMeshRenderData> SkeletalMeshRenderData;内部实际上是LOD渲染时数据：
```
TIndirectArray<FSkeletalMeshLODRenderData> LODRenderData;
```
```
FMeshDescription
FMeshDescriptionBulkData
FSkeletalMeshSourceModel
```
它们更偏：导入编辑器源模型描述可编辑拓扑
FSkeletalMeshLODRenderData是编辑器源数据经过构建，变成运行时渲染数据，更偏：
运行时、渲染线程、GPU Buffers、已构建好的 LOD 渲染格式

#### TArray<FSkelMeshRenderSection> RenderSections：这个 LOD 被分成了多少个渲染 section
一个 Section 一般对应：一段连续索引，一个材质槽，一组 Bone Map，一块独立的渲染批次
#### FMultiSizeIndexContainer MultiSizeIndexContainer：这一层 LOD 的三角形顶点索引数据

#### FStaticMeshVertexBuffers StaticVertexBuffers：静态骨骼经典属性

#### FSkinWeightVertexBuffer SkinWeightVertexBuffer：每个顶点受哪些骨骼影响、对应权重是多少
#### ClothVertexBuffer 布料模拟相关的顶点映射数据
#### MorphTargetVertexInfoBuffers Morph Target 在这一层 LOD 的 GPU 友好访问数据
哪些顶点会被某个 Morph 影响，它们的 delta 是多少，GPU 上怎么快速取这些数据
Morph：管理表情 / 面部形变 / 膨胀收缩 / 角色细节变形
#### FSkinWeightProfilesData SkinWeightProfilesData：备用权重集合
#### VertexAttributeBuffers 额外的顶点属性渲染数据
除了标准 Position / UV / Normal / SkinWeight 之外，还有其他顶点属性需要渲染时使用
#### uint32 BuffersSize;
#### Ray Tracing 相关：光线追踪用的几何数据

#### 逆绑定矩阵：挂在 SkinnedAsset / SkeletalMesh 这一层
### 骨骼指针相关
TObjectPtr<USkeleton> Skeleton：关联骨骼
FReferenceSkeleton RefSkeleton;自己的骨骼参考结构
前者是“绑定到哪套 Skeleton 资产”，后者是“我自己这张 Mesh 的参考骨骼结构

### 其它

#### bulk存储：
在 UE 里，BulkData，它通常不是一个逻辑概念名词，这部分数据体积比较大，不适合像普通小字段那样跟 UObject 的常规属性混在一起序列化，而是单独按“大块数据”方式存和读。
特点：体量大、需要单独管理、可能需要按需加载
