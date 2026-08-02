// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "T_BaseCharacter.h"
#include "T_PlayerCharacter.generated.h"

class UCameraComponent;
class USpringArmComponent;
class UT_TraversalComponent;
class UT_GrabComponent;
class UT_AimingComponent;
class UMotionWarpingComponent;
class USkeletalMeshComponent;
class UT_PickUpComponent;

UCLASS()
class GASTESTDEMO1_API AT_PlayerCharacter : public AT_BaseCharacter
{
	GENERATED_BODY()

public:

	AT_PlayerCharacter();
	
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	virtual UAttributeSet* GetAttributeSet() const override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_PlayerState() override;
	
	UFUNCTION(BlueprintCallable, Category = "Camera|Traversal")
	void SetCameraCollisionEnabled(bool bEnabled);
	void SetTraversalCollisionEnabled(bool bEnabled);

	UFUNCTION(BlueprintPure, Category = "Weapon")
	USkeletalMeshComponent* GetEquippedWeaponMesh() const;

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void SetEquippedWeaponMesh(USkeletalMeshComponent* InWeaponMesh) { EquippedWeaponMesh = InWeaponMesh; }

	UT_PickUpComponent* GetPickUpComponent() const { return PickUpComponent; }

	bool bPreviousCameraCollisionEnabled = true;
	bool bTraversalCameraModeActive = false;
	ECollisionEnabled::Type PreviousTraversalCollisionEnabled = ECollisionEnabled::QueryAndPhysics;
	bool bTraversalCollisionDisabled = false;

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpringArmComponent> CameraBoom;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> FollowCamera;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UT_TraversalComponent> TraversalComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UT_GrabComponent> GrabComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UT_AimingComponent> AimingComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UMotionWarpingComponent> MotionWarpingComponent;

	UPROPERTY(Transient)
	TObjectPtr<USkeletalMeshComponent> EquippedWeaponMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UT_PickUpComponent> PickUpComponent;

};
