// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "AbilitySystem/T_AttributeSet.h"
#include "T_PlayerController.generated.h"

class UInputMappingContext;
class UInputAction;
class UUserWidget;
class AT_PlayerCharacter;
class UT_InventoryComponent;
class UT_InventoryWidget;
class UT_AttributeWidget;
class UT_AttributeSet;
class UT_GameMenuWidget;
class UT_LastChanceWidget;
class UT_QuestWidget;
class AT_QuestGameState;
struct FInputActionValue;
struct FGameplayTag;
struct FOnAttributeChangeData;

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

	void HandleCatchMovementModeChanged(EMovementMode MovementMode);
	void CancelRunAndCatch();

	void HandleGameMenuContinue();
	void RestartQuestLevel();
	void QuitQuestGame();

	UFUNCTION(Client, Reliable)
	void ClientShowLastChance();
	
protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void OnPossess(APawn* InPawn) override;
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
	TObjectPtr<UInputAction> ReloadAction;

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

	UPROPERTY(EditDefaultsOnly, Category="Crash|UI|HUD")
	TSubclassOf<UUserWidget> PlayerHUDWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category="Crash|Input|Quest")
	TObjectPtr<UInputAction> QuestAction;

	UPROPERTY(EditDefaultsOnly, Category="Crash|UI|Quest")
	TSubclassOf<UT_QuestWidget> QuestWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category="Crash|UI|Quest")
	TSubclassOf<UT_GameMenuWidget> GameMenuWidgetClass;

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
	void Reload();
	void StopPrimary();
	void PickUp();
	void ActivateQuickSlot(FKey Key);
	void ToggleQuestUI();
	void ToggleGameMenu();
	void OpenGameMenu(uint8 MenuMode);
	void CloseGameMenu();
	void BindQuestState();
	void HideLastChance();

	UFUNCTION()
	void HandleQuestStateChanged();
	void ActivateAbility(const FGameplayTag& AbilityTag) const;
	void ReleaseAbility(const FGameplayTag& AbilityTag) const;
	bool IsAlive() const;
	void StartLockOn();
	void SwitchLockOnTarget(const FInputActionValue& Value);
	void StartCatch();
	void StopCatch();
	void SendCatchEvent();
	void ReleaseGrab();
	void SendPlayerGameplayEvent(const FGameplayTag& EventTag, float EventMagnitude = 0.0f) const;

	UPROPERTY(Transient)
	TObjectPtr<UT_InventoryWidget> InventoryWidget;

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> PlayerHUDWidget;

	UPROPERTY(Transient)
	TObjectPtr<UT_QuestWidget> QuestWidget;

	UPROPERTY(Transient)
	TObjectPtr<UT_GameMenuWidget> GameMenuWidget;

	UPROPERTY(Transient)
	TObjectPtr<UT_LastChanceWidget> LastChanceWidget;

	UPROPERTY(Transient)
	TObjectPtr<AT_QuestGameState> BoundQuestGameState;

	FTimerHandle LastChanceTimerHandle;

	bool bWasMouseCursorVisible = false;

	// HUD 属性 Widget 绑定
	void BindPlayerStatusWidgets();
	UFUNCTION()
	void OnPlayerHUDASCInitialized(UAbilitySystemComponent* ASC, UAttributeSet* AS);
	void DoBindHUDWidgets(UAbilitySystemComponent* ASC, UT_AttributeSet* AttributeSet, AT_PlayerCharacter* PlayerCharacter);
	void OnHUDWidgetAttributeChanged(const FOnAttributeChangeData& ChangeData, TWeakObjectPtr<UT_AttributeWidget> Widget, FGameplayAttribute Attribute, FGameplayAttribute MaxAttribute);

	TSet<FGameplayAttribute> BoundHUDAttributeKeys;

};
