// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.


#include "AnimGraphNode_ThreeBoneIK.h"
#include "IKProject.h"
#include "AnimNodeEditModes.h"
#include "AnimationCustomVersion.h"

// for customization details
#include "PropertyHandle.h"
#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"

#define LOCTEXT_NAMESPACE "A3Nodes"

/////////////////////////////////////////////////////
// FTwoBoneIKDelegate

class FThreeBoneIKDelegate : public TSharedFromThis<FThreeBoneIKDelegate>
{
public:
	void UpdateLocationSpace(class IDetailLayoutBuilder* DetailBuilder)
	{
		if (DetailBuilder)
		{
			DetailBuilder->ForceRefreshDetails();
		}
	}
};

TSharedPtr<FThreeBoneIKDelegate> UAnimGraphNode_ThreeBoneIK::TwoBoneIKDelegate = NULL;

/////////////////////////////////////////////////////
// UAnimGraphNode_ThreeBoneIK


UAnimGraphNode_ThreeBoneIK::UAnimGraphNode_ThreeBoneIK(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

FText UAnimGraphNode_ThreeBoneIK::GetControllerDescription() const
{
	return LOCTEXT("ThreeBoneIK", "Three Bone IK");
}

FText UAnimGraphNode_ThreeBoneIK::GetTooltipText() const
{
	return LOCTEXT("AnimGraphNode_ThreeBoneIK_Tooltip", "The Three Bone IK control applies an inverse kinematic (IK) solver to a 3-joint chain, such as the limbs of a character.");
}

FText UAnimGraphNode_ThreeBoneIK::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	if ((TitleType == ENodeTitleType::ListView || TitleType == ENodeTitleType::MenuTitle) && (Node.IKBone.BoneName == NAME_None))
	{
		return GetControllerDescription();
	}
	// @TODO: the bone can be altered in the property editor, so we have to 
	//        choose to mark this dirty when that happens for this to properly work
	else //if (!CachedNodeTitles.IsTitleCached(TitleType, this))
	{
		FFormatNamedArguments Args;
		Args.Add(TEXT("ControllerDescription"), GetControllerDescription());
		Args.Add(TEXT("BoneName"), FText::FromName(Node.IKBone.BoneName));

		// FText::Format() is slow, so we cache this to save on performance
		if (TitleType == ENodeTitleType::ListView || TitleType == ENodeTitleType::MenuTitle)
		{
			CachedNodeTitles.SetCachedTitle(TitleType, FText::Format(LOCTEXT("AnimGraphNode_IKBone_ListTitle", "{ControllerDescription} - Bone: {BoneName}"), Args), this);
		}
		else
		{
			CachedNodeTitles.SetCachedTitle(TitleType, FText::Format(LOCTEXT("AnimGraphNode_IKBone_Title", "{ControllerDescription}\nBone: {BoneName}"), Args), this);
		}
	}
	return CachedNodeTitles[TitleType];
}

void UAnimGraphNode_ThreeBoneIK::CopyNodeDataToPreviewNode(FAnimNode_Base* InPreviewNode)
{
	FAnimNode_ThreeBoneIK* TwoBoneIK = static_cast<FAnimNode_ThreeBoneIK*>(InPreviewNode);

	// copies Pin values from the internal node to get data which are not compiled yet
	TwoBoneIK->EffectorLocation = Node.EffectorLocation;
	TwoBoneIK->JointTargetLocation = Node.JointTargetLocation;
}

void UAnimGraphNode_ThreeBoneIK::CopyPinDefaultsToNodeData(UEdGraphPin* InPin)
{
	if (InPin->GetName() == GET_MEMBER_NAME_STRING_CHECKED(FAnimNode_ThreeBoneIK, EffectorLocation))
	{
		GetDefaultValue(GET_MEMBER_NAME_STRING_CHECKED(FAnimNode_ThreeBoneIK, EffectorLocation), Node.EffectorLocation);
	}
	else if (InPin->GetName() == GET_MEMBER_NAME_STRING_CHECKED(FAnimNode_ThreeBoneIK, JointTargetLocation))
	{
		GetDefaultValue(GET_MEMBER_NAME_STRING_CHECKED(FAnimNode_ThreeBoneIK, JointTargetLocation), Node.JointTargetLocation);
	}
}

void UAnimGraphNode_ThreeBoneIK::CustomizeDetails(class IDetailLayoutBuilder& DetailBuilder)
{
	// initialize just once
	if (!TwoBoneIKDelegate.IsValid())
	{
		TwoBoneIKDelegate = MakeShareable(new FThreeBoneIKDelegate());
	}

	EBoneControlSpace Space = Node.EffectorLocationSpace;
	const FString TakeRotationPropName = FString::Printf(TEXT("Node.%s"), GET_MEMBER_NAME_STRING_CHECKED(FAnimNode_ThreeBoneIK, bTakeRotationFromEffectorSpace));
	const FString MaintainEffectorPropName = FString::Printf(TEXT("Node.%s"), GET_MEMBER_NAME_STRING_CHECKED(FAnimNode_ThreeBoneIK, bMaintainEffectorRelRot));
	const FString EffectorBoneName = FString::Printf(TEXT("Node.%s"), GET_MEMBER_NAME_STRING_CHECKED(FAnimNode_ThreeBoneIK, EffectorSpaceBoneName));
	const FString EffectorLocationPropName = FString::Printf(TEXT("Node.%s"), GET_MEMBER_NAME_STRING_CHECKED(FAnimNode_ThreeBoneIK, EffectorLocation));

	if (Space == BCS_BoneSpace || Space == BCS_ParentBoneSpace)
	{
		IDetailCategoryBuilder& IKCategory = DetailBuilder.EditCategory("IK");
		IDetailCategoryBuilder& EffectorCategory = DetailBuilder.EditCategory("EndEffector");
		TSharedPtr<IPropertyHandle> PropertyHandle;
		PropertyHandle = DetailBuilder.GetProperty(*TakeRotationPropName, GetClass());
		EffectorCategory.AddProperty(PropertyHandle);
		PropertyHandle = DetailBuilder.GetProperty(*MaintainEffectorPropName, GetClass());
		EffectorCategory.AddProperty(PropertyHandle);
		PropertyHandle = DetailBuilder.GetProperty(*EffectorBoneName, GetClass());
		EffectorCategory.AddProperty(PropertyHandle);
	}
	else // hide all properties in EndEffector category
	{
		TSharedPtr<IPropertyHandle> PropertyHandle = DetailBuilder.GetProperty(*EffectorLocationPropName, GetClass());
		DetailBuilder.HideProperty(PropertyHandle);
		PropertyHandle = DetailBuilder.GetProperty(*TakeRotationPropName, GetClass());
		DetailBuilder.HideProperty(PropertyHandle);
		PropertyHandle = DetailBuilder.GetProperty(*MaintainEffectorPropName, GetClass());
		DetailBuilder.HideProperty(PropertyHandle);
		PropertyHandle = DetailBuilder.GetProperty(*EffectorBoneName, GetClass());
		DetailBuilder.HideProperty(PropertyHandle);
	}

	Space = Node.JointTargetLocationSpace;
	bool bPinVisibilityChanged = false;
	const FString JointTargetSpaceBoneName = FString::Printf(TEXT("Node.%s"), GET_MEMBER_NAME_STRING_CHECKED(FAnimNode_ThreeBoneIK, JointTargetSpaceBoneName));
	const FString JointTargetLocation = FString::Printf(TEXT("Node.%s"), GET_MEMBER_NAME_STRING_CHECKED(FAnimNode_ThreeBoneIK, JointTargetLocation));

	if (Space == BCS_BoneSpace || Space == BCS_ParentBoneSpace)
	{
		IDetailCategoryBuilder& IKCategory = DetailBuilder.EditCategory("IK");
		IDetailCategoryBuilder& EffectorCategory = DetailBuilder.EditCategory("JointTarget");
		TSharedPtr<IPropertyHandle> PropertyHandle;
		PropertyHandle = DetailBuilder.GetProperty(*JointTargetSpaceBoneName, GetClass());
		EffectorCategory.AddProperty(PropertyHandle);
	}
	else // hide all properties in JointTarget category except for JointTargetLocationSpace
	{
		TSharedPtr<IPropertyHandle> PropertyHandle = DetailBuilder.GetProperty(*JointTargetSpaceBoneName, GetClass());
		DetailBuilder.HideProperty(PropertyHandle);
	}

	const FString EffectorLocationSpace = FString::Printf(TEXT("Node.%s"), GET_MEMBER_NAME_STRING_CHECKED(FAnimNode_ThreeBoneIK, EffectorLocationSpace));
	const FString JointTargetLocationSpace = FString::Printf(TEXT("Node.%s"), GET_MEMBER_NAME_STRING_CHECKED(FAnimNode_ThreeBoneIK, JointTargetLocationSpace));

	// refresh UIs when bone space is changed
	TSharedRef<IPropertyHandle> EffectorLocHandle = DetailBuilder.GetProperty(*EffectorLocationSpace, GetClass());
	if (EffectorLocHandle->IsValidHandle())
	{
		FSimpleDelegate UpdateEffectorSpaceDelegate = FSimpleDelegate::CreateSP(TwoBoneIKDelegate.Get(), &FThreeBoneIKDelegate::UpdateLocationSpace, &DetailBuilder);
		EffectorLocHandle->SetOnPropertyValueChanged(UpdateEffectorSpaceDelegate);
	}

	TSharedRef<IPropertyHandle> JointTragetLocHandle = DetailBuilder.GetProperty(*JointTargetLocationSpace, GetClass());
	if (JointTragetLocHandle->IsValidHandle())
	{
		FSimpleDelegate UpdateJointSpaceDelegate = FSimpleDelegate::CreateSP(TwoBoneIKDelegate.Get(), &FThreeBoneIKDelegate::UpdateLocationSpace, &DetailBuilder);
		JointTragetLocHandle->SetOnPropertyValueChanged(UpdateJointSpaceDelegate);
	}
}

FEditorModeID UAnimGraphNode_ThreeBoneIK::GetEditorMode() const
{
	return AnimNodeEditModes::AnimNode;
}

void UAnimGraphNode_ThreeBoneIK::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	Ar.UsingCustomVersion(FAnimationCustomVersion::GUID);

	const int32 CustomAnimVersion = Ar.CustomVer(FAnimationCustomVersion::GUID);

	if (CustomAnimVersion < FAnimationCustomVersion::RenamedStretchLimits)
	{
		// fix up deprecated variables
		Node.StartStretchRatio = Node.StretchLimits_DEPRECATED.X;
		Node.MaxStretchScale = Node.StretchLimits_DEPRECATED.Y;
	}
}
#undef LOCTEXT_NAMESPACE
