// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Camera/CameraComponent.h"
#include "IKProjectCharacter.generated.h"


UCLASS(config=Game)
class AIKProjectCharacter : public APawn
{
	GENERATED_BODY()

public:
	AIKProjectCharacter();

	/** VRScene. */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = IK)
	USceneComponent* Root;

	/** IKMesh. */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category=IK)
	USkeletalMeshComponent* IKMesh;

	/** VRScene. */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = IK)
	USceneComponent* VRScene;

	/** HMDCamera. */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category= IK)
	UCameraComponent* HMDCamera;

	/** 是否开启IK. */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = IK)
	bool IsStarIK;

	/** 校准捕捉器. */
	UFUNCTION(BlueprintCallable, Category = IK)
	virtual void CalibrationTraker() {};

	/** 启动匹配. */
	UFUNCTION(BlueprintCallable, Category = IK)
	virtual void StarIK();

	/////////////////////////////////////////////////////////////////////
	//
	////////////////////////////////////////////////////////////////////
	/** 头部. */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = IK)
	FTransform NewHeadTransform;

	/** 新腰部. */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = IK)
	FTransform NewWaistTransform;

	/** 新左脚. */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = IK)
	FTransform NewLeftFootTransform;

	/** 新右脚. */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = IK)
	FTransform NewRightFootTransform;

	/** 老腰部. */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = IK)
	FTransform OldWaistTransform;

	/** 老左脚. */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = IK)
	FTransform OldLeftFootTransform;

	/** 老右脚. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = IK)
	FTransform OldRightFootTransform;


protected:

	/** Resets HMD orientation in VR. */
	void OnResetVR();

	/** Called for forwards/backward input */
	void MoveForward(float Value);

	/** Called for side to side input */
	void MoveRight(float Value);

	/** 
	 * Called via input to turn at a given rate. 
	 * @param Rate	This is a normalized rate, i.e. 1.0 means 100% of desired turn rate
	 */
	void TurnAtRate(float Rate);

	/**
	 * Called via input to turn look up/down at a given rate. 
	 * @param Rate	This is a normalized rate, i.e. 1.0 means 100% of desired turn rate
	 */
	void LookUpAtRate(float Rate);

	/** Handler for when a touch input begins. */
	void TouchStarted(ETouchIndex::Type FingerIndex, FVector Location);

	/** Handler for when a touch input stops. */
	void TouchStopped(ETouchIndex::Type FingerIndex, FVector Location);

protected:
	// APawn interface
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	virtual void Tick( float DeltaSeconds );
	// End of APawn interface

	int Foot_R_Index;
	int Foot_L_Index;
	int Foot_Waist_Index;
};

