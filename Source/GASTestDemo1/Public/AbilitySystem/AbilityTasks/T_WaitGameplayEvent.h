// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Async/AbilityAsync_WaitGameplayEvent.h"
#include "T_WaitGameplayEvent.generated.h"

/**
 * 
 */
UCLASS()
class GASTESTDEMO1_API UT_WaitGameplayEvent :  public UAbilityAsync_WaitGameplayEvent
{
	GENERATED_BODY()
public:

	UFUNCTION(BlueprintCallable, Category = "Ability|Async", meta = (DefaultToSelf = "TargetActor", BlueprintInternalUseOnly = "TRUE"))
	static UT_WaitGameplayEvent* WaitGameplayEventToActorProxy(AActor* TargetActor, UPARAM(meta=(GameplayTagFilter="GameplayEventTagsCategory")) FGameplayTag EventTag, bool OnlyTriggerOnce = false, bool OnlyMatchExact = true);

	
	void StartActivation();
};
