// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.


#include "AnimNode_ThreeBoneIK.h"
#include "IKProject.h"
#include "AnimationRuntime.h"
#include "TwoBoneIK.h"
#include "Animation/AnimInstanceProxy.h"

DECLARE_CYCLE_STAT(TEXT("ThreeBoneIK Eval"), STAT_ThreeBoneIK_Eval, STATGROUP_Anim);


/////////////////////////////////////////////////////
// FAnimNode_ThreeBoneIK

FAnimNode_ThreeBoneIK::FAnimNode_ThreeBoneIK()
	: EffectorLocation(FVector::ZeroVector)
	, JointTargetLocation(FVector::ZeroVector)
	, bTakeRotationFromEffectorSpace(false)
	, bMaintainEffectorRelRot(false)
	, bAllowStretching(false)
	, StretchLimits_DEPRECATED(FVector2D::ZeroVector)
	, StartStretchRatio(1.f)
	, MaxStretchScale(1.2f)
	, EffectorLocationSpace(BCS_ComponentSpace)
	, JointTargetLocationSpace(BCS_ComponentSpace)
	, bendNormalStrategy(BendNormalStrategy::followTarget)
{
}

void FAnimNode_ThreeBoneIK::GatherDebugData(FNodeDebugData& DebugData)
{
	FString DebugLine = DebugData.GetNodeName(this);

	DebugLine += "(";
	AddDebugNodeData(DebugLine);
	DebugLine += FString::Printf(TEXT(" IKBone: %s)"), *IKBone.BoneName.ToString());
	DebugData.AddDebugItem(DebugLine);

	ComponentPose.GatherDebugData(DebugData);
}

void FAnimNode_ThreeBoneIK::EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms)
{

	return;

	SCOPE_CYCLE_COUNTER(STAT_ThreeBoneIK_Eval);

	check(OutBoneTransforms.Num() == 0);

	// 骨骼容器
	const FBoneContainer& BoneContainer = Output.Pose.GetPose().GetBoneContainer();

	// Get indices of the lower and upper limb bones and check validity.
	bool bInvalidLimb = false;

	// 得到Pose索引 （最末端）
	FCompactPoseBoneIndex IKBoneCompactPoseIndex = IKBone.GetCompactPoseIndex(BoneContainer);
	// 高分支的索引 （最末端的父亲）
	FCompactPoseBoneIndex UpperLimbIndex(INDEX_NONE);
	// 低分支的索引 （最末端的父亲的父亲）
	const FCompactPoseBoneIndex LowerLimbIndex = BoneContainer.GetParentBoneIndex(IKBoneCompactPoseIndex);
	if (LowerLimbIndex == INDEX_NONE)
	{
		bInvalidLimb = true;
	}
	else
	{
		UpperLimbIndex = BoneContainer.GetParentBoneIndex(LowerLimbIndex);
		if (UpperLimbIndex == INDEX_NONE)
		{
			bInvalidLimb = true;
		}
	}
	const bool bInBoneSpace = (EffectorLocationSpace == BCS_ParentBoneSpace) || (EffectorLocationSpace == BCS_BoneSpace);
	//////////////////////////////////////////////////////////////////////////
	// 不用效果点控制
	//const bool bInBoneSpace = (EffectorLocationSpace == BCS_ParentBoneSpace) || (EffectorLocationSpace == BCS_BoneSpace);
	//const int32 EffectorBoneIndex = bInBoneSpace ? BoneContainer.GetPoseBoneIndexForBoneName(EffectorSpaceBoneName) : INDEX_NONE;
	//const FCompactPoseBoneIndex EffectorSpaceBoneIndex = BoneContainer.MakeCompactPoseIndex(FMeshPoseBoneIndex(EffectorBoneIndex));

	//if (bInBoneSpace && (EffectorSpaceBoneIndex == INDEX_NONE))
	//{
	//	bInvalidLimb = true;
	//}

	//// If we walked past the root, this controlled is invalid, so return no affected bones.
	//if( bInvalidLimb )
	//{
	//	return;
	//}
	//////////////////////////////////////////////////////////////////////////

	// Get Local Space transforms for our bones. We do this first in case they already are local.
	// As right after we get them in component space. (And that does the auto conversion).
	// We might save one transform by doing local first...
	const FTransform EndBoneLocalTransform = Output.Pose.GetLocalSpaceTransform(IKBoneCompactPoseIndex);

	// Now get those in component space... 得到三根骨骼的矩阵
	FTransform LowerLimbCSTransform = Output.Pose.GetComponentSpaceTransform(LowerLimbIndex);
	FTransform UpperLimbCSTransform = Output.Pose.GetComponentSpaceTransform(UpperLimbIndex);
	FTransform EndBoneCSTransform = Output.Pose.GetComponentSpaceTransform(IKBoneCompactPoseIndex);

	// Get current position of root of limb. 
	// All position are in Component space. 得到三根骨骼的位置
	const FVector RootPos = UpperLimbCSTransform.GetTranslation();
	const FVector InitialJointPos = LowerLimbCSTransform.GetTranslation();
	const FVector InitialEndPos = EndBoneCSTransform.GetTranslation();

	if (IsInitLength == false)
	{
		LowerLimbLength = (InitialJointPos - InitialEndPos).Size2D();
		UpperLimbLength = (RootPos - InitialJointPos).Size2D();

		IsInitLength = true;
	}

	//////////////////////////////////////////////////////////////////////////
	// 不用效果点
	//// Transform EffectorLocation from EffectorLocationSpace to ComponentSpace.
	//FTransform EffectorTransform(EffectorLocation);
	//FAnimationRuntime::ConvertBoneSpaceTransformToCS(Output.AnimInstanceProxy->GetComponentTransform(), Output.Pose, EffectorTransform, EffectorSpaceBoneIndex, EffectorLocationSpace);
	//////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////
	// 不用约束点
	//// Get joint target (used for defining plane that joint should be in).
	//FTransform JointTargetTransform(JointTargetLocation);
	//FCompactPoseBoneIndex JointTargetSpaceBoneIndex(INDEX_NONE);

	//if (JointTargetLocationSpace == BCS_ParentBoneSpace || JointTargetLocationSpace == BCS_BoneSpace)
	//{
	//	int32 Index = BoneContainer.GetPoseBoneIndexForBoneName(JointTargetSpaceBoneName);
	//	JointTargetSpaceBoneIndex = BoneContainer.MakeCompactPoseIndex(FMeshPoseBoneIndex(Index));
	//}

	//FAnimationRuntime::ConvertBoneSpaceTransformToCS(Output.AnimInstanceProxy->GetComponentTransform(), Output.Pose, JointTargetTransform, JointTargetSpaceBoneIndex, JointTargetLocationSpace);

	//FVector	JointTargetPos = JointTargetTransform.GetTranslation();

	//// This is our reach goal.
	//FVector DesiredPos = EffectorTransform.GetTranslation();
	//////////////////////////////////////////////////////////////////////////

	// IK solver
	UpperLimbCSTransform.SetLocation(RootPos);
	LowerLimbCSTransform.SetLocation(InitialJointPos);
	EndBoneCSTransform.SetLocation(InitialEndPos);

	//////////////////////////////////////////////////////////////////////////
	//AnimationCore::SolveTwoBoneIK(UpperLimbCSTransform, LowerLimbCSTransform, EndBoneCSTransform, JointTargetPos, DesiredPos, bAllowStretching, StartStretchRatio, MaxStretchScale);
	//////////////////////////////////////////////////////////////////////////

	// Update transform for upper bone.
	{
		// Order important. First bone is upper limb.
		OutBoneTransforms.Add( FBoneTransform(UpperLimbIndex, UpperLimbCSTransform) );
	}

	// Update transform for lower bone.
	{
		// Order important. Second bone is lower limb.
		OutBoneTransforms.Add( FBoneTransform(LowerLimbIndex, LowerLimbCSTransform) );
	}

	//////////////////////////////////////////////////////////////////////////
	//// Update transform for end bone.
	//{
	//	// only allow bTakeRotationFromEffectorSpace during bone space
	//	if (bInBoneSpace && bTakeRotationFromEffectorSpace)
	//	{
	//		EndBoneCSTransform.SetRotation(EffectorTransform.GetRotation());
	//	}
	//	else if (bMaintainEffectorRelRot)
	//	{
	//		EndBoneCSTransform = EndBoneLocalTransform * LowerLimbCSTransform;
	//	}
	//	// Order important. Third bone is End Bone.
	//	OutBoneTransforms.Add(FBoneTransform(IKBoneCompactPoseIndex, EndBoneCSTransform));
	//}

	//// Make sure we have correct number of bones
	//check(OutBoneTransforms.Num() == 3);
	//////////////////////////////////////////////////////////////////////////
}

bool FAnimNode_ThreeBoneIK::IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones)
{
	// if both bones are valid
	return (IKBone.IsValid(RequiredBones));
}

FVector FAnimNode_ThreeBoneIK::GetBendDirection(FVector IKPosition, FVector bendNormal, FTransform& InOutLowerLimbTransform, FTransform& InOutUpperLimbTransform)
{
	FVector direction = IKPosition - InOutUpperLimbTransform.GetLocation();
	if (direction == FVector::ZeroVector) return  FVector::ZeroVector;

	float directionSqrMag = direction.SizeSquared();
	float directionMagnitude = (float)FMath::Sqrt(directionSqrMag);

	float x = (directionSqrMag + UpperLimbLength*UpperLimbLength - LowerLimbLength*LowerLimbLength) / 2.0f / directionMagnitude;
	//float y = (float)FMath::Sqrt(FMath::Clamp(UpperLimbLength*UpperLimbLength - x * x, 0.0f));

	//FVector yDirection = FVector::CrossProduct(direction, bendNormal);
	//return  FVector(direction, bendNormal, FVector::CrossProduct(direction, yDirection)) * FVector(0, y, x);
	return direction;
}

void FAnimNode_ThreeBoneIK::InitializeBoneReferences(const FBoneContainer& RequiredBones)
{
	IKBone.Initialize(RequiredBones);

	//if (IKBone1 == null || IKBone2 == null || IKBone3 == null || target == null) {
	//	Debug.LogError("bone or target empty, IK aborted");
	//	return;
	//}

	//bone1 = new Bone{ trans = IKBone1 };
	//bone2 = new Bone{ trans = IKBone2 };
	//bone3 = new Bone{ trans = IKBone3 };
	//bone1.length = Vector3.Distance(bone1.trans.position, bone2.trans.position);
	//bone2.length = Vector3.Distance(bone2.trans.position, bone3.trans.position);

	//Vector3 bendNormal = defaultBendNormal;
	////if (bendNormal == Vector3.zero) bendNormal = Vector3.forward;
	//bone1.Initiate(bone2.trans.position, bendNormal);
	//bone2.Initiate(bone3.trans.position, bendNormal);

	IsInitLength = false;
}

// 结算3骨骼IK
void FAnimNode_ThreeBoneIK::SolveThreeBoneIK(FTransform& InOutEndBoneTransform, FTransform& InOutLowerLimbTransform, FTransform& InOutUpperLimbTransform, const FVector& TragetLoaction)
{
	//clamp target if distance to target is longer than bones combined
	// 固定住目标位置 如果距离到目标位置长度这两个骨骼之间
	FVector actualTargetPos;
	float overallLength = (TragetLoaction - InOutUpperLimbTransform.GetLocation()).Size2D();

	if (overallLength > UpperLimbLength + LowerLimbLength) 
	{
		actualTargetPos = InOutUpperLimbTransform.GetLocation() + (TragetLoaction - InOutUpperLimbTransform.GetLocation()).GetSafeNormal() * (UpperLimbLength + LowerLimbLength);
		overallLength = UpperLimbLength + LowerLimbLength;
	}
	else
		actualTargetPos = TragetLoaction;

	//calculate bend normal
	//you may need to change this based on the model you chose
	FVector bendNormal = FVector::ZeroVector;
	switch (bendNormalStrategy)
	{
	case BendNormalStrategy::followTarget:
		bendNormal = -FVector::CrossProduct(actualTargetPos - InOutUpperLimbTransform.GetLocation(), TragetLoaction.ForwardVector);
		break;
	case BendNormalStrategy::rightArm:
		bendNormal = -FVector::UpVector;
		break;
	case BendNormalStrategy::leftArm:
		bendNormal = FVector::UpVector;
		break;
	case BendNormalStrategy::head:
		bendNormal = InOutUpperLimbTransform.Rotator().Vector().GetSafeNormal();
		break;
	default:
		//Debug.LogError("Undefined bendnormal strategy: " + bendNormalStrategy);
		break;
	}

	//calculate bone1, bone2 rotation
	//Vector3 bendDirection = GetBendDirection(actualTargetPos, bendNormal);

	//// Rotating bone1
	//bone1.trans.rotation = bone1.GetRotation(bendDirection, bendNormal);

	//// Rotating bone 2
	//bone2.trans.rotation = bone2.GetRotation(actualTargetPos - bone2.trans.position, bone2.GetBendNormalFromCurrentRotation(defaultBendNormal));
	////bone2.trans.rotation = bone2.GetRotation(actualTargetPos - bone2.trans.position, Quaternion.AngleAxis(debugAngle, target.forward)* target.up);

	//bone3.trans.rotation = target.rotation;
}