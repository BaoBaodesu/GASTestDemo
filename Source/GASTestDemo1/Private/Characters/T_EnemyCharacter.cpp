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

	if (!HasAuthority()) return;
	
	GiveStartupAbilities();
	InitializeAttributes();
	
	OnASCInitialized.Broadcast(GetAbilitySystemComponent(), GetAttributeSet());
	
	UT_AttributeSet* T_AttributeSet = Cast<UT_AttributeSet>(GetAttributeSet());
	if (!IsValid(T_AttributeSet)) return;
	
	GetAbilitySystemComponent()->GetGameplayAttributeValueChangeDelegate(T_AttributeSet->GetHealthAttribute()).AddUObject(this, &ThisClass::OnHealthChanged);
}
