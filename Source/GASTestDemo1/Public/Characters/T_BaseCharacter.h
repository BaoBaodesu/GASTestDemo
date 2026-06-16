// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Character.h"
#include "T_BaseCharacter.generated.h"

class UAttributeSet;
class UGameplayAbility;
class UAbilitySystemComponent;
class UT_AbilitySystemComponent;
class UGameplayEffect;

// 声明一个 ASC 初始化完成事件
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FASCInitialized, UAbilitySystemComponent*, ASC, UAttributeSet*, AS);

UCLASS(Abstract)
class GASTESTDEMO1_API AT_BaseCharacter : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AT_BaseCharacter();
	
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	virtual UAttributeSet* GetAttributeSet() const { return nullptr; }
	
	UPROPERTY(BlueprintAssignable)
	FASCInitialized OnASCInitialized;
protected:
	// 初始能力
	void GiveStartupAbilities();
	// 初始化属性
	void InitializeAttributes() const;

private:
	
	UPROPERTY(EditDefaultsOnly, Category = "Crash|Abilities")
	TArray<TSubclassOf<UGameplayAbility>> StartupAbilities;
	
	UPROPERTY(EditDefaultsOnly, Category = "Crash|Effects")
	TSubclassOf<UGameplayEffect> InitializeAttributesEffect;
};