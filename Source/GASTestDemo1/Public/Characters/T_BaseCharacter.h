// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Character.h"
#include "T_BaseCharacter.generated.h"

namespace CrashTags
{
	extern GASTESTDEMO1_API const FName Player;
}


struct FOnAttributeChangeData;
class UAttributeSet;
class UGameplayAbility;
class UAbilitySystemComponent;
class UT_AbilitySystemComponent;
class UGameplayEffect;
class UT_LockOnComponent;

// 声明一个 ASC 初始化完成事件
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FASCInitialized, UAbilitySystemComponent*, ASC, UAttributeSet*, AS);

UCLASS(Abstract)
class GASTESTDEMO1_API AT_BaseCharacter : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AT_BaseCharacter();
	
	virtual void OnMovementModeChanged( EMovementMode PrevMovementMode, uint8 PreviousCustomMode) override;
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	virtual UAttributeSet* GetAttributeSet() const { return nullptr; }
	bool IsAlive() const { return bAlive; }
	void SetAlive(bool bAliveStatus) { bAlive = bAliveStatus; }
	
	UPROPERTY(BlueprintReadWrite, Category = "Crash|Movement")
	bool bIsFalling = false;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UT_LockOnComponent* LockOnComponent;
	
	UPROPERTY(BlueprintAssignable)
	FASCInitialized OnASCInitialized;
	
	UFUNCTION(BlueprintCallable, Category = "Crash|Death")
	virtual void HandleRespawn();
	
	UFUNCTION(BlueprintCallable, Category = "Crash|Attributes")
	void ResetAttributes();

	UFUNCTION(BlueprintImplementableEvent)
	void RotateToTarget(AActor* RotateTarget);

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Crash|Damage")
	float DamageNumberVerticalOffset{200.f};

protected:
	// 初始能力
	void GiveStartupAbilities();
	// 初始化属性
	void InitializeAttributes() const;
	
	void OnHealthChanged(const FOnAttributeChangeData& AttributeChangeData);
	virtual void HandleDeath();
private:
	
	UPROPERTY(EditDefaultsOnly, Category = "Crash|Abilities")
	TArray<TSubclassOf<UGameplayAbility>> StartupAbilities;
	
	UPROPERTY(EditDefaultsOnly, Category = "Crash|Effects")
	TSubclassOf<UGameplayEffect> InitializeAttributesEffect;
	
	UPROPERTY(EditDefaultsOnly, Category = "Crash|Effects")
	TSubclassOf<UGameplayEffect> ResetAttributesEffect;
	
	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Replicated)
    bool bAlive = true;
};
