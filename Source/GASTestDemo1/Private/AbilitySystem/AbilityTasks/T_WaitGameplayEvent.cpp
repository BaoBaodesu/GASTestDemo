// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/AbilityTasks/T_WaitGameplayEvent.h"

UT_WaitGameplayEvent* UT_WaitGameplayEvent::WaitGameplayEventToActorProxy(AActor* TargetActor, FGameplayTag EventTag,
	bool OnlyTriggerOnce, bool OnlyMatchExact)
{
	UT_WaitGameplayEvent* MyObj = NewObject<UT_WaitGameplayEvent>();
	MyObj->SetAbilityActor(TargetActor);
	MyObj->Tag = EventTag;
	MyObj->OnlyTriggerOnce = OnlyTriggerOnce;
	MyObj->OnlyMatchExact = OnlyMatchExact;
	return MyObj;
	
}

void UT_WaitGameplayEvent::StartActivation()
{
	Activate();
}
