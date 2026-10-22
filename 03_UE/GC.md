UE
# GC:
单独的垃圾回收原因：大型游戏里面对象关系非常复杂，如果手动delete，必须知道一个对象的创建，拥有，释放时机，稍稍错一点就会造成很大的问题。

### 对象：UObject
体系受到GC系统管理。
UE里面所有需要被引擎管理的对象，最终都希望继承 UObject。
```
GC垃圾回收
反射（知道自己有什么变量、函数）
序列化（保存/加载）
编辑器支持
```
#### UObject：
最基础的对象，内有：
```
UObject
{
    名字 Name

    所属 Outer

    类型 Class

    GC标记

}
```
#### 子类：
##### UClass类的信息
##### AActor:
##### APawn:
##### ACharacter
##### AActor:UActorComponent
不是所有东西都是Actor,很多功能应该拆出成单独的组件。
##### USceneComponent
##### 资源类
#### UObject图：
![alt text](image.png)

### GC的核心思想：可达性分析 Mark and Sweep（标记-清扫）
判断：这个对象还有没有人需要？

#### 从root开始
UE有一个Root Set 永远不会被 GC 的对象集合，如UWorld，加入到这里面，从而作为根开始向下标记。

#### 看引用对象：利用反射系统。
DFS/BFS向下扫描，引用到的对象进行一个标记。

#### sweep：
GUObjectArray中，没有被标记的，清除。

#### 为什么不用引用计数：
循环引用泄漏。
标记清扫天然解决这个问题：但如果循环引用的一方在root，不会删除，因为它还有用。

## GC控制：
![alt text](image-1.png)

### AddToRoot
把一个 UObject 直接加入 GC Root 集合，让 GC 永远认为它是可达对象

#### 使用时间： 全局管理器，没对象使用，但却想要一直存在
不一定一直要在root里面，但希望能手动控制


##### 删除：RemoveFromRoot

#### 问题：内存泄漏
局部函数里面加入后，删除不会自动删除，必须手动。


### TStrongObjectPtr addtoroot的上位替代

RAII管理

```
现代UE更推荐：
短生命周期：TStrongObjectPtr
长期：UPROPERTY()
```

#### 其它的几种指针：
##### UObject* 裸指针  只是C++指针，GC不知道
##### UPROPERTY() + UObject*
告诉Reflection系统："追踪这个引用"
##### TObjectPtr 
更安全：它不是直接保存裸地址。
支持虚拟化对象

##### TWeakObjectPtr 弱引用
eg：UI限时玩家，但不应该控制玩家的生命周期 所以用weak

##### TSoftObjectPtr

软引用，不一定加载对象，保存的不是：内存地址，而是：资源路径
LoadAsync()，用的时候再加载
## GC优化：
核心思路：减少GC工作量、降低GC停顿、减少垃圾产生、优化对象管理

### 增量GC：不要一次做完。拆成多个Frame
把mark和sweep拆开，分帧来做
### 并行GC：把一些任务放到工作线程上。

### 减少UObject数量：使用对象池，将对象重复使用
eg：子弹对象

### cluster：不逐个扫描，而是打成一个cluster一起处理
eg. Actor和components，生命周期一致，直接打包处理。

### 控制GC频率，合理生命周期设计等。

### 与LUA GC 对比
都采用可达性分析思想，但是实现细节完全不同。

##### UE：UObject + Root Set + Mark-Sweep

##### LUA：Lua VM 管理对象 + 三色标记 + 增量/分步回收

#### 对象：
Table，Function，Closure，Thread，String，UserData

#### 核心算法：三色标记算法

白色 White：未被发现，可能垃圾
灰色 Gray：已经发现，但是里面引用还没扫描
黑色 Black：自己和所有引用都扫描完成

```
GC开始
↓
Mark Root
↓
传播灰色对象
↓
所有可达对象变黑
↓
清理白色对象
↓
释放内存
```

##### Lua为什么不用一次性Mark
管理对象量大，需要增量GC，灰色：待处理任务队列

# 反射：Reflection System
### 为什么需要反射：
当编译后，类的大部分信息被丢失，运行时，对于对象，cpp不知道它有哪些类，名字是什么。而游戏引擎大量需要运行时信息，如编辑器编辑对象，GC需要知道对象里面都有什么，蓝图里面出现的FUNCTION节点是怎么来的
#### 反射：程序运行时，可以查询自身结构信息，并动态操作对象

## 原理：
UHT+宏标记+运行时元数据
### UHT：UnrealHeaderTool
扫描对应宏，生成代码，反射记录内部对象的偏移，从而在使用时读取。
```
生成：Player.generated.h
里面包含：
StaticClass(),类型信息描述,属性注册代码,函数注册代码
```
### 注册：
#### UCLASS:
```
UClass对象
Name:
APlayer
Super:
AActor
Properties:
nullptr
Functions:
nullptr
```

#### UPROPERTY
```
UPROPERTY()
int HP;
它不是简单注释。UHT看到：生成：
FIntProperty
名字:HP
偏移:xxx
Flags:
Edit
Visible
Replicated
```
### UFUNCTION:
```
UFUNCTION()
void Jump();
生成：UFunction Jump
里面保存：函数名字,参数列表,调用地址,标志
```
它不只是创建对象,还要把它加入 UE 的全局系统,如全局 UObject 管理数组。
但并不是代替对象，实际还会有游戏的对象，只是都是照着这张图纸写的，这份图纸叫做元数据，会在内存中单开辟一块空间，对象实例内会有一个指针指向对应的元数据类型。


### 属性同步与快照：

#### 属性同步：如血量武器等，常用于不常变化的，比较频率高
```
C++变量
↓
UPROPERTY(Replicated)
↓
UHT生成反射信息
↓
生成RepLayout
↓
服务器维护Shadow State
↓
每帧比较Property
↓
找到变化Property
↓
Bit序列化
↓
网络发送
↓
客户端RepLayout解析
↓
修改Property
↓
OnRep通知
```
####  快照：定时更新，对齐，如位置朝向，如果每帧比压力大，就定时一比。