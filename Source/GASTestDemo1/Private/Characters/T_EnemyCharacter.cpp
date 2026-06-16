// Fill out your copyright notice in the Description page of Project Settings.

#include "Characters/T_EnemyCharacter.h"

#include "AbilitySystem/T_AbilitySystemComponent.h"
#include "AbilitySystem/T_AttributeSet.h"


AT_EnemyCharacter::AT_EnemyCharacter()
{

	PrimaryActorTick.bCanEverTick = false;
	
	AbilitySystemComponent = CreateDefaultSubobject<UT_AbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);
	
	AttributeSet = CreateDefaultSubobject<UT_AttributeSet>("AttributeSet");
}

UAbilitySystemComponent* AT_EnemyCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

UAttributeSet* AT_EnemyCharacter::GetAttributeSet() const
{
	return AttributeSet;
}

void AT_EnemyCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	if (!IsValid(GetAbilitySystemComponent())) return;
	
	GetAbilitySystemComponent()->InitAbilityActorInfo(this, this);
	OnASCInitialized.Broadcast(GetAbilitySystemComponent(), GetAttributeSet());
	
	if (!HasAuthority()) return;
	
	GiveStartupAbilities();
	InitializeAttributes();
}
