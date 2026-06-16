// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/T_PlayerState.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/T_AbilitySystemComponent.h"
#include "AbilitySystem/T_AttributeSet.h"
#include "Characters/T_BaseCharacter.h"

AT_PlayerState::AT_PlayerState()
{
	SetNetUpdateFrequency(100.0f);
	
	AbilitySystemComponent = CreateDefaultSubobject<UT_AbilitySystemComponent>("AbilitySystemComponent");
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);
	
	AttributeSet = CreateDefaultSubobject<UT_AttributeSet>("AttributeSet");
}

UAbilitySystemComponent* AT_PlayerState::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}
