// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Abilities/T_Primary.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Engine/OverlapResult.h"
#include "GameplayTags/TTags.h"

void UT_Primary::SendHitReactEventToActors(const TArray<AActor*>& ActorsHit)
{
	for (AActor* HitActor : ActorsHit)
	{
		FGameplayEventData Payload;
		Payload.Instigator = GetAvatarActorFromActorInfo();
		
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(HitActor, TTags::Events::Enemy::HitReact, Payload);
	}
}
