// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "T_BaseCharacter.h"
#include "T_EnemyCharacter.generated.h"

class UAttributeSet;
UCLASS()
class GASTESTDEMO1_API AT_EnemyCharacter : public AT_BaseCharacter
{
	GENERATED_BODY()

public:

	AT_EnemyCharacter();
	
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	virtual UAttributeSet* GetAttributeSet() const override;

protected:

	virtual void BeginPlay() override;
	
private:

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<UAttributeSet> AttributeSet;
};
