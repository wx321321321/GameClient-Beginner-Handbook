# UE 动画系统概览

[返回本章目录](README.md) · [返回手册首页](../README.md)

UE动画系统的核心，是把角色状态转换成最终骨骼姿势基础资源包括 Skeleton、SkeletalMesh 和 AnimSequence。运行时由 AnimInstance/Animation Blueprint读取角色速度、加速度、是否落地、朝向等数据，再在 AnimGraph中通过状态机、BlendSpace和动画混合生成基础姿势。

## 动画系统依赖：

- USkeleton
- USkeletalMesh
- USkeletalMeshComponent

## 动画系统资源：

- UAnimSequenceBase
- UAnimSequence
- UBlendSpace1D
- UBlendSpace
- UAimOffsetBlendSpace/UAimOffsetBlendSpace1D
- UPoseAsset
- UBlendProfile

## 动画系统运行大脑：

- UAnimBlueprint
- UAnimInstance
- UAnimBlueprintGeneratedClass
- FAnimInstanceProxy
- AnimGraph

## 动画系统关键节点：

- FAnimNode_Base
- SequencePlayer / SequenceEvaluator
- BlendSpacePlayer
- ApplyAdditive
- LayeredBoneBlend
- Slot
- StateMachine
- Inertialization
- LinkedAnimLayer

## 动画系统事件与通知

- UAnimNotify
- UAnimNotifyState
- UAnimMontage
- Root Motion 与Source

## 骨骼控制与节点：

- FAnimNode_SkeletalControlBase
- FAnimNode_ModifyBone
- FAnimNode_TwoBoneIK
- FAnimNode_Fabrik
- FAnimNode_ModifyCurve

## 角色动画相关：

- ACharacter
- UCharacterMovementComponent

## 其他模块与后续阅读

- ControlRig
- IKRig
- IK Retargeter
- 多线程更新
