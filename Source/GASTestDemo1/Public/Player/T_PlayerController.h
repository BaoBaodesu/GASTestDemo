// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "T_PlayerController.generated.h"

class UInputMappingContext;
class UInputAction;
class UT_InventoryComponent;
class UT_InventoryWidget;
struct FInputActionValue;
struct FGameplayTag;

UCLASS()
class GASTESTDEMO1_API AT_PlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:
	
	UPROPERTY(BlueprintReadOnly, Category = "Crash|Input|Movement", meta = (AllowPrivateAccess = "true"))
	FVector2D MovementVector;

	UFUNCTION(BlueprintCallable, Category="Inventory")
	void ToggleInventory();

	UFUNCTION(BlueprintCallable, Category="Inventory")
	void OpenInventory(UT_InventoryComponent* StorageInventory = nullptr);

	UFUNCTION(BlueprintCallable, Category="Inventory")
	void CloseInventory();
	
protected:
	virtual void SetupInputComponent() override;

private:

	UPROPERTY(EditDefaultsOnly, Category = "Crash|Input")
	TArray<TObjectPtr<UInputMappingContext>> InputMappingContexts;

	UPROPERTY(EditDefaultsOnly, Category = "Crash|Input|Movement")
	TObjectPtr<UInputAction> JumpAction;

	UPROPERTY(EditDefaultsOnly, Category = "Crash|Input|Movement")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditDefaultsOnly, Category = "Crash|Input|Movement")
	TObjectPtr<UInputAction> LookAction;
	
	UPROPERTY(EditDefaultsOnly, Category = "Crash|Input|Abilities")
	TObjectPtr<UInputAction> PrimaryAction;
		
	UPROPERTY(EditDefaultsOnly, Category = "Crash|Input|Abilities")
	TObjectPtr<UInputAction> SecondaryAction;
		
	UPROPERTY(EditDefaultsOnly, Category = "Crash|Input|Abilities")
	TObjectPtr<UInputAction> TertiaryAction;
	
	UPROPERTY(EditDefaultsOnly, Category = "Crash|Input|Abilities")
	TObjectPtr<UInputAction> RollAction;

	UPROPERTY(EditDefaultsOnly, Category = "Crash|Input|Abilities")
	TObjectPtr<UInputAction> AimAction;
	
	UPROPERTY(EditDefaultsOnly, Category = "Crash|Input|Abilities")
	UInputAction* LockOnAction;

	UPROPERTY(EditDefaultsOnly, Category = "Crash|Input|Abilities")
	UInputAction* SwitchLockOnAction;
	
	UPROPERTY(EditDefaultsOnly, Category = "Crash|Input|Abilities")
	UInputAction* CatchAction;

	UPROPERTY(EditDefaultsOnly, Category = "Crash|Input|Abilities")
	UInputAction* CrouchAction;
	
	UPROPERTY(EditDefaultsOnly, Category = "Crash|Input|Abilities")
	UInputAction* ReleaseAction;
	
	UPROPERTY(EditDefaultsOnly, Category = "Crash|Input|Abilities")
	TObjectPtr<UInputAction> PickUpAction;

	UPROPERTY(EditDefaultsOnly, Category="Crash|Input|Inventory")
	TObjectPtr<UInputAction> InventoryAction;

	UPROPERTY(EditDefaultsOnly, Category="Crash|UI|Inventory")
	TSubclassOf<UT_InventoryWidget> InventoryWidgetClass;

	void Jump();
	void StopJumping();
	void Move(const FInputActionValue& Value);
	void StopMove();
	void Look(const FInputActionValue& Value);
	void Primary();
	void Secondary();
	void Tertiary();
	void StandingDodge();
	void Roll();
	void ToggleCrouch();
	void StartAim();
	void StopAim();
	void PickUp();
	void ActivateQuickSlot(FKey Key);
	void ActivateAbility(const FGameplayTag& AbilityTag) const;
	void ReleaseAbility(const FGameplayTag& AbilityTag) const;
	bool IsAlive() const;
	void StartLockOn();
	void SwitchLockOnTarget(const FInputActionValue& Value);
	void StartCatch();
	void StopCatch();
	void ReleaseGrab();
	void SendPlayerGameplayEvent(const FGameplayTag& EventTag, float EventMagnitude = 0.0f) const;

	UPROPERTY(Transient)
	TObjectPtr<UT_InventoryWidget> InventoryWidget;

	bool bWasMouseCursorVisible = false;

};
