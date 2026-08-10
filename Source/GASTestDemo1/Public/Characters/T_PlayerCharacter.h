// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "T_BaseCharacter.h"
#include "Inventory/T_InventoryItemHandler.h"
#include "T_PlayerCharacter.generated.h"

class UCameraComponent;
class UAnimMontage;
class USpringArmComponent;
class UT_TraversalComponent;
class UT_GrabComponent;
class UT_AimingComponent;
class UMotionWarpingComponent;
class USkeletalMeshComponent;
class UT_PickUpComponent;
class UT_InventoryComponent;
class UT_ItemDefinition;

UCLASS()
class GASTESTDEMO1_API AT_PlayerCharacter : public AT_BaseCharacter, public IT_InventoryItemHandler
{
	GENERATED_BODY()

public:

	AT_PlayerCharacter();
	
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	virtual UAttributeSet* GetAttributeSet() const override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_PlayerState() override;
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	
	UFUNCTION(BlueprintCallable, Category = "Camera|Traversal")
	void SetCameraCollisionEnabled(bool bEnabled);
	void SetTraversalCollisionEnabled(bool bEnabled);
	void SetInvincibilityCollisionEnabled(bool bInvincible);

	UFUNCTION(BlueprintPure, Category = "Weapon")
	USkeletalMeshComponent* GetEquippedWeaponMesh() const;

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void SetEquippedWeaponMesh(USkeletalMeshComponent* InWeaponMesh) { EquippedWeaponMesh = InWeaponMesh; }

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Weapon")
	bool bHasPistolGun{false};

	UFUNCTION(BlueprintPure, Category = "Weapon")
	bool HasPistolGun() const { return bHasPistolGun; }

	UT_PickUpComponent* GetPickUpComponent() const { return PickUpComponent; }

	UFUNCTION(BlueprintPure, Category="Inventory")
	UT_InventoryComponent* GetInventoryComponent() const { return InventoryComponent; }

	UFUNCTION(Client, Reliable)
	void ClientNotifyHitConfirmed();

	virtual bool CanUseInventoryItem_Implementation(UT_ItemDefinition* ItemDefinition) override;
	virtual bool UseInventoryItem_Implementation(UT_ItemDefinition* ItemDefinition) override;
	virtual bool CanEquipInventoryItem_Implementation(UT_ItemDefinition* ItemDefinition) override;
	virtual bool EquipInventoryItem_Implementation(UT_ItemDefinition* ItemDefinition) override;
	virtual bool UnequipInventoryItem_Implementation(UT_ItemDefinition* ItemDefinition) override;

	bool bPreviousCameraCollisionEnabled = true;
	bool bTraversalCameraModeActive = false;
	ECollisionEnabled::Type PreviousTraversalCollisionEnabled = ECollisionEnabled::QueryAndPhysics;
	bool bTraversalCollisionDisabled = false;
	bool bInvincibilityCollisionActive = false;

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

	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Animation")
	TObjectPtr<UAnimMontage> EquipPistolMontage;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UT_PickUpComponent> PickUpComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UT_InventoryComponent> InventoryComponent;

	UPROPERTY(Transient)
	TObjectPtr<AActor> EquippedInventoryActor;

};
