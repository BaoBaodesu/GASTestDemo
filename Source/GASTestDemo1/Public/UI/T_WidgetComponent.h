// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/WidgetComponent.h"
#include "T_WidgetComponent.generated.h"

class UAbilitySystemComponent;
class UAttributeSet;
class UT_AttributeSet;
class UT_AbilitySystemComponent;
class AT_BaseCharacter;
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class GASTESTDEMO1_API UT_WidgetComponent : public UWidgetComponent
{
	GENERATED_BODY()

public:

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

private:
	TWeakObjectPtr<AT_BaseCharacter> CrashCharacter;
	TWeakObjectPtr<UT_AbilitySystemComponent> AbilitySystemComponent;
	TWeakObjectPtr<UT_AttributeSet> AttributeSet;

	void InitAbilitySystemData();
	bool IsASCInitialized() const;
	void InitializeAttributeDelegate();

	UFUNCTION()
	void OnASCInitialized(UAbilitySystemComponent* ASC, UAttributeSet* AS);
	
	UFUNCTION()
	void BindToAttributeChanges();
};
