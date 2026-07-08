// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "T_PlayerController.generated.h"

class UInputMappingContext;
class UInputAction;
struct FInputActionValue;
struct FGameplayTag;

UCLASS()
class GASTESTDEMO1_API AT_PlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:
	
	UPROPERTY(BlueprintReadOnly, Category = "Crash|Input|Movement", meta = (AllowPrivateAccess = "true"))
	FVector2D MovementVector;
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
	TObjectPtr<UInputAction> StandingDodgeAction;
	
	UPROPERTY(EditDefaultsOnly, Category = "Crash|Input|Abilities")
	UInputAction* LockOnAction;

	UPROPERTY(EditDefaultsOnly, Category = "Crash|Input|Abilities")
	UInputAction* SwitchLockOnAction;
	
	void Jump();
	void StopJumping();
	void Move(const FInputActionValue& Value);
	void StopMove();
	void Look(const FInputActionValue& Value);
	void Primary();
	void Secondary();
	void Tertiary();
	void StandingDodge();
	void ActivateAbility(const FGameplayTag& AbilityTag) const;
	bool IsAlive() const;
	void StartLockOn();
	void SwitchLockOnTarget(const FInputActionValue& Value);

};
