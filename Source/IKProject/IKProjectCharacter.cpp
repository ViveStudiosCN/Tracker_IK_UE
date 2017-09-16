// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "IKProjectCharacter.h"
#include "Kismet/HeadMountedDisplayFunctionLibrary.h"
#include "Camera/CameraComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/Controller.h"
#include "Components/SkinnedMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "SteamVRFunctionLibrary.h"
#include "Kismet/KismetMathLibrary.h"
//#include "SteamVRFunctionLibrary.h"

//////////////////////////////////////////////////////////////////////////
// AIKProjectCharacter

AIKProjectCharacter::AIKProjectCharacter() :
	Foot_R_Index(0),
	Foot_L_Index(0),
	Foot_Waist_Index(0)
{

	auto Root = CreateOptionalDefaultSubobject<USceneComponent>(TEXT("Root"));
	if (Root)
	{
		this->SetRootComponent(Root);
	}

	IKMesh = CreateOptionalDefaultSubobject<USkeletalMeshComponent>(ACharacter::MeshComponentName);
	if (IKMesh)
	{
		IKMesh->AlwaysLoadOnClient = true;
		IKMesh->AlwaysLoadOnServer = true;
		IKMesh->bOwnerNoSee = false;
		IKMesh->MeshComponentUpdateFlag = EMeshComponentUpdateFlag::AlwaysTickPose;
		IKMesh->bCastDynamicShadow = true;
		IKMesh->bAffectDynamicIndirectLighting = true;
		IKMesh->PrimaryComponentTick.TickGroup = TG_PrePhysics;
		IKMesh->SetupAttachment(GetRootComponent());
		static FName MeshCollisionProfileName(TEXT("IKMesh"));
		IKMesh->SetCollisionProfileName(MeshCollisionProfileName);
		IKMesh->bGenerateOverlapEvents = false;
		IKMesh->SetCanEverAffectNavigation(false);

		IKMesh->SetupAttachment(Root);
	}

	VRScene = CreateOptionalDefaultSubobject<USceneComponent>(TEXT("VRScene"));
	if (VRScene)
	{
		VRScene->SetupAttachment(IKMesh);

		// Create a camera
		HMDCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("HMDCamera"));
		if (HMDCamera)
		{
			HMDCamera->SetupAttachment(VRScene); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
			HMDCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm
		}
	}



	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named MyCharacter (to avoid direct content references in C++)
}

//////////////////////////////////////////////////////////////////////////
// Input

void AIKProjectCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// Set up gameplay key bindings
	check(PlayerInputComponent);

	PlayerInputComponent->BindAxis("MoveForward", this, &AIKProjectCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AIKProjectCharacter::MoveRight);

	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "turn" handles devices that provide an absolute delta, such as a mouse.
	// "turnrate" is for devices that we choose to treat as a rate of change, such as an analog joystick
	PlayerInputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("TurnRate", this, &AIKProjectCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("LookUpRate", this, &AIKProjectCharacter::LookUpAtRate);

	// handle touch devices
	PlayerInputComponent->BindTouch(IE_Pressed, this, &AIKProjectCharacter::TouchStarted);
	PlayerInputComponent->BindTouch(IE_Released, this, &AIKProjectCharacter::TouchStopped);

	// VR headset functionality
	PlayerInputComponent->BindAction("ResetVR", IE_Pressed, this, &AIKProjectCharacter::OnResetVR);
}


void AIKProjectCharacter::OnResetVR()
{
	UHeadMountedDisplayFunctionLibrary::ResetOrientationAndPosition();
}

void AIKProjectCharacter::TouchStarted(ETouchIndex::Type FingerIndex, FVector Location)
{
		//Jump();
}

void AIKProjectCharacter::TouchStopped(ETouchIndex::Type FingerIndex, FVector Location)
{
		//StopJumping();
}

void AIKProjectCharacter::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	//AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void AIKProjectCharacter::LookUpAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	//AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

void AIKProjectCharacter::MoveForward(float Value)
{
	if ((Controller != NULL) && (Value != 0.0f))
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		AddMovementInput(Direction, Value);
	}
}

void AIKProjectCharacter::MoveRight(float Value)
{
	if ( (Controller != NULL) && (Value != 0.0f) )
	{
		// find out which way is right
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);
	
		// get right vector 
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		// add movement in that direction
		AddMovementInput(Direction, Value);
	}
}

void AIKProjectCharacter::StarIK()
{

}

void AIKProjectCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	FVector HeadLocation = FVector::ZeroVector;
	FRotator HeadRotator = FRotator::ZeroRotator;
	// 头显
	USteamVRFunctionLibrary::GetTrackedDevicePositionAndOrientation(0, HeadLocation, HeadRotator);

	FTransform HeadTran;
	HeadTran.SetLocation(HeadLocation);
	HeadTran.SetRotation(HeadRotator.Quaternion());

	// 获取Tracker设备ID
	TArray<int32> OutTrackersId;
	USteamVRFunctionLibrary::GetValidTrackedDeviceIds(ESteamVRTrackedDeviceType::Invalid, OutTrackersId);

	TArray<FTransform> TrackersTransform;
	// 读取Tracker数据
	for (auto TrackerID : OutTrackersId)
	{
		FTransform TrackerTran;
		FVector Location;
		FRotator Rotator;
		USteamVRFunctionLibrary::GetTrackedDevicePositionAndOrientation(TrackerID, Location, Rotator);
		TrackerTran.SetLocation(Location);
		TrackerTran.SetRotation(Rotator.Quaternion());
		TrackersTransform.Add(TrackerTran);
	}

	if (IsStarIK == false)
	{
		// 区分左右脚和腰部
		OldWaistTransform.SetLocation(FVector(0, 0, -999.0f));
		OldLeftFootTransform.SetLocation(FVector(0, 999.0f, 0));
		OldRightFootTransform.SetLocation(FVector(0, -999.0f, 0));

		int TrackerIndex = 0;
		for (auto TrackerIDTran : TrackersTransform)
		{
			/** 老腰部. 判读高度*/
			if (TrackerIDTran.GetLocation().Z > OldWaistTransform.GetLocation().Z)
			{
				Foot_Waist_Index = TrackerIndex;
				OldWaistTransform = TrackersTransform[Foot_Waist_Index];
			}
			/** 老左脚. 判读Y的小*/
			if (TrackerIDTran.GetLocation().Y < OldLeftFootTransform.GetLocation().Y)
			{
				Foot_L_Index = TrackerIndex;
				OldLeftFootTransform = TrackersTransform[Foot_L_Index];
			}
			/** 老右脚. 判读Y的大*/
			if (TrackerIDTran.GetLocation().Y > OldRightFootTransform.GetLocation().Y)
			{
				Foot_R_Index = TrackerIndex;
				OldRightFootTransform = TrackersTransform[Foot_R_Index];
			}
			// 自增
			TrackerIndex++;
		}
	}

	if (IsStarIK == true)
	{
		if (Foot_Waist_Index < TrackersTransform.Num())
		{
			NewWaistTransform.SetLocation(TrackersTransform[Foot_Waist_Index].GetLocation() - OldWaistTransform.GetLocation());
			NewWaistTransform.SetRotation(UKismetMathLibrary::NormalizedDeltaRotator(TrackersTransform[Foot_Waist_Index].GetRotation().Rotator(), OldWaistTransform.GetRotation().Rotator()).Quaternion());
		}
		if (Foot_L_Index < TrackersTransform.Num())
		{
			NewLeftFootTransform.SetLocation(TrackersTransform[Foot_L_Index].GetLocation() - OldLeftFootTransform.GetLocation());
			NewLeftFootTransform.SetRotation(UKismetMathLibrary::NormalizedDeltaRotator(TrackersTransform[Foot_L_Index].GetRotation().Rotator(), OldLeftFootTransform.GetRotation().Rotator()).Quaternion());
		}

		if (Foot_R_Index < TrackersTransform.Num())
		{
			NewRightFootTransform.SetLocation(TrackersTransform[Foot_R_Index].GetLocation() - OldRightFootTransform.GetLocation());
			NewRightFootTransform.SetRotation(UKismetMathLibrary::NormalizedDeltaRotator(TrackersTransform[Foot_R_Index].GetRotation().Rotator(), OldRightFootTransform.GetRotation().Rotator()).Quaternion());
		}

	}

}