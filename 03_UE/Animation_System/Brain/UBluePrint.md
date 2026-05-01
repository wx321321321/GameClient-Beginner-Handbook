# UBluePrint:
##### 编辑器里的“蓝图资产本体”
它负责保存蓝图的源数据、图、变量、组件模板、编译状态、调试状态、依赖关系、扩展点，以及和“编译产物类”之间的关系。
所以这个类非常“大”，因为它几乎把蓝图编辑器、编译器、调试器、资产系统都串起来了。
##### 打包后：
UBlueprint 这个蓝图资产本身是 editor-only，在 cooked assets 里并不存在。这意味着在打包后的运行时，通常不是带着 UBlueprint 资产进去现编，而是使用已经准备好的、可运行的类数据
## 定义：

### UCLASS(config=Engine, MinimalAPI)

这是一个 UE 可反射的类；它的配置项走 Engine 这一套 ini 配置域；这个类对模块外只导出最少的 API 信息

### public UBlueprintCore
UBlueprint 不是最底层的蓝图类，它是在 UBlueprintCore 之上的完整蓝图资产类型。
UBlueprintCore 更偏基础蓝图资产共性，UBlueprint 才是我们平时最常见的“普通蓝图资产”主体

### public IBlueprintPropertyGuidProvider：这个蓝图可以提供“属性名 ↔ GUID”的映射
```cpp
FindBlueprintPropertyNameFromGuid
FindBlueprintPropertyGuidFromName
```
蓝图属性重命名后，尽量还能追踪原属性身份
存档、重定向、版本兼容时更稳定
避免只靠名字导致的脆弱性
## 核心职责：
```cpp
1. 记录这个蓝图“继承谁、是什么类型”
2. 保存蓝图的图数据（EventGraph、FunctionGraph、MacroGraph 等）
3. 保存变量、接口、组件模板、Timeline 等源数据
4. 管理编译状态、编译模式、编译产物类
5. 管理调试状态、断点、正在调试的对象/世界
6. 管理依赖关系和编译缓存
7. 提供编辑器扩展点、事件广播、资产元数据
8. 管理和 GeneratedClass / SkeletonClass / CDO 的关系
```
## 核心信息：
### 父类：
这个蓝图最终生成的类，要继承哪个父类 eg：
```cpp
Actor 蓝图：ParentClass 可能是 AActor 或某个自定义 Actor 类
Widget 蓝图：可能是 UUserWidget
AnimBlueprint：可能是 UAnimInstance
普通 Object 蓝图：可能是 UObject
```
在blueprint中写对应关系，UE会GeneratedClass，这个生成的类是父类的子类。

### BlueprintType：这个蓝图属于哪种蓝图类型
```
普通蓝图
接口蓝图
宏库蓝图
函数库蓝图
数据-only 蓝图等
也就是说，蓝图不是只有一种形态。不同类型会决定：
能不能有组件
能不能有事件图
能不能实例化
能不能编译成某种类
```
### 编译与加载状态相关字段
####  uint8 bRecompileOnLoad:1;蓝图加载时是否需要重新编译
因为蓝图的编译产物类不是永远可信的。版本升级、节点废弃、依赖变化、父类变化后，可能要在加载时重新编译。
#### bHasBeenRegenerated：生成类在加载后是否已经完成过一次 regenerate
可以理解成“这次加载周期里有没有完成过首次重建”。
#### bIsRegeneratingOnLoad 当前是不是正处于加载时重建类的过程中
防止重入、重复编译、状态混乱。
#### CompileMode 编译这个蓝图时采用什么模式
比如更偏完整编译、字节码-only、某些特化编译路径。
#### Status
```
UpToDate
UpToDateWithWarnings
Dirty
Unknown
Error 一类状态
```
#### bBeingCompiled / bQueuedForCompilation
当前是否正在编译/是否已经排入编译队列
有时候你改一个蓝图，系统不会瞬间同步所有编译，而是排队、批量处理依赖。

### 编辑器显示与类选项相关字段

#### bIsNewlyCreated：这个蓝图是不是刚创建，还没被真正打开编辑过
编辑器可能据此决定首次打开行为。
#### bForceFullEditor 是否强制用完整蓝图编辑器打开，而不是 data-only 简化编辑器
很多 Blueprint 如果只是数据改动，可以用更轻量界面；但这个标志能强制开完整界面。
#### bRunConstructionScriptOnDrag
#### bRunConstructionScriptInSequencer：Sequencer 中是否执行 Construction Script
也就是你在序列器里操作时，构造脚本要不要参与。

#### ShouldCookPropertyGuidsValue：Cook 版本里要不要把 Blueprint 属性 GUID 一起打进去
### 蓝图显示名、描述、命名空间、分类
```
BlueprintDisplayName
BlueprintDescription
BlueprintNamespace
BlueprintCategory
HideCategories
这些主要是编辑器/UI/资产管理层数据：
显示给用户看的名字
内容浏览器悬停描述
命名空间
Palette 分类
需要隐藏的分类
它们不直接决定运行时逻辑，但对编辑器体验非常重要。
```
### 源代码层的“图”数据
#### SimpleConstructionScript 蓝图组件树/构造组件图的核心数据
```
一个 Actor 蓝图里你在 Components 面板加了：
Mesh
Camera
SpringArm
AudioComponent
这些不是凭空存的，而是通过 SCS 维护出来的模板结构。
组件面板
  ↓
SimpleConstructionScript
  ↓
生成组件模板 / 构造逻辑
```
#### UbergraphPagesUber Graph 页面集合
也就是 Event Graph 相关的大图页。
#### FunctionGraphs
蓝图里图形化实现的函数图集合
#### DelegateSignatureGraphs
委托签名图
#### MacroGraphs
这个蓝图里定义的宏图集合
#### IntermediateGeneratedGraphs：编译过程中生成/中间产出的图
这是编译器层的中间结果，不是用户直接编辑的主图。
#### EventGraphs：实际被编译的事件图集合
### 组件模板、Timeline、继承组件处理
### 变量、接口、分类、书签、断点等编辑器数据
### 扩展系统：UBlueprintExtension
### 调试相关：正在调试哪个对象/世界
### 事件广播：蓝图改了、编译了
### 
