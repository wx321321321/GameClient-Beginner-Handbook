# UE动画系统
总的来说，UE动画系统的作用，是根据角色当前状态，在运行时持续算出这一帧最终骨骼姿势并输出到 Skeletal Mesh中。
想要掌握UE动画系统，要对源码进行一个学习，因此这里将UE动画系统拆分开来：

## 动画系统依赖：

#### USkeleton
#### USkeletalMesh
#### USkeletalMeshComponent

## 动画系统资源： 
#### UAnimSequenceBase
#### UAnimSequence
#### UBlendSpace1D
#### UBlendSpace
#### UAimOffsetBlendSpace/UAimOffsetBlendSpace1D
#### UPoseAsset
#### UBlendProfile
## 动画系统运行大脑： 
#### UAnimBlueprint
#### UAnimInstance
#### UAnimBlueprintGeneratedClass
#### FAnimInstanceProxy
## 动画系统关键节点：
#### FAnimNode_Base
#### SequencePlayer / SequenceEvaluator
#### BlendSpacePlayer
#### ApplyAdditive
#### LayeredBoneBlend
#### Slot
#### StateMachine
#### Inertialization
#### LinkedAnimLayer
## 动画系统事件与通知
#### UAnimNotify
#### UAnimNotifyState
#### UAnimMontage
#### Root Motion 与Source
## 骨骼控制与节点：
#### FAnimNode_SkeletalControlBase
#### FAnimNode_ModifyBone
#### FAnimNode_TwoBoneIK
#### FAnimNode_Fabrik
#### FAnimNode_ModifyCurve
## 角色动画相关：
#### ACharacter
#### UCharacterMovementComponent
## ControlRig
## IKRig
## IK Retargeter
## 多线程更新
