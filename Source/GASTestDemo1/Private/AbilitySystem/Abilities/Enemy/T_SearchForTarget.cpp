// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Abilities/Enemy/T_SearchForTarget.h"

#include "AbilitySystemComponent.h"
#include "AIController.h"
#include "AbilitySystem/AbilityTasks/T_WaitGameplayEvent.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "AbilitySystem/AbilityTasks/T_WaitGameplayEvent.h"
#include "Characters/T_EnemyCharacter.h"
#include "GameplayTags/TTags.h"
#include "Tasks/AITask_MoveTo.h"
#include "Utils/T_BlueprintLibrary.h"

UT_SearchForTarget::UT_SearchForTarget()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;	
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
}

void UT_SearchForTarget::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	
	OwningEnemy = Cast<AT_EnemyCharacter>(GetAvatarActorFromActorInfo());
	check(OwningEnemy.IsValid());
	OwningAIController = Cast<AAIController>(OwningEnemy->GetController());
	check(OwningAIController.IsValid());
	
	StartSearch();
	
	WaitGameplayEventTask = UT_WaitGameplayEvent::WaitGameplayEventToActorProxy(GetAvatarActorFromActorInfo(), TTags::Events::Enemy::EndAttack);
	WaitGameplayEventTask->EventReceived.AddDynamic(this, &ThisClass::EndAttackEventReceived);
	WaitGameplayEventTask->StartActivation();
}

void UT_SearchForTarget::StartSearch()
{
	if (bDrawDebugs) GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, FString::Printf(TEXT("UT_SearchForTarget::StartSearch")));
	if (!OwningEnemy.IsValid()) return;
	
	const float SearchDelay = FMath::RandRange(OwningEnemy->MinAttackDelay, OwningEnemy->MaxAttackDelay);
	SearchDelayTask = UAbilityTask_WaitDelay::WaitDelay(this, SearchDelay);
	SearchDelayTask->OnFinish.AddDynamic(this, &ThisClass::Search);
	SearchDelayTask->Activate();
}

void UT_SearchForTarget::EndAttackEventReceived(FGameplayEventData Payload)
{
	StartSearch();
}

void UT_SearchForTarget::Search()
{
	const FVector SearchOrigin = GetAvatarActorFromActorInfo()->GetActorLocation();
	FClosestActorWithTagResult ClosestActorResult = UT_BlueprintLibrary::FindClosestActorWithTag(this, SearchOrigin, CrashTags::Player);

	TargetBaseCharacter = Cast<AT_BaseCharacter>(ClosestActorResult.Actor);

	if (!TargetBaseCharacter.IsValid())
	{
		StartSearch();
		return;
	}
	if (TargetBaseCharacter->IsAlive())
	{
		MoveToTargetAndAttack();
	}
	else
	{
		StartSearch();
	}
}

void UT_SearchForTarget::MoveToTargetAndAttack()
{
	if (!OwningEnemy.IsValid() || !OwningAIController.IsValid() || !TargetBaseCharacter.IsValid()) return;
	if (!OwningEnemy->IsAlive())
	{
		StartSearch();
		return;
	}

	MoveToLocationOrActorTask = UAITask_MoveTo::AIMoveTo(
		OwningAIController.Get(),
		FVector(),
		TargetBaseCharacter.Get(),
		OwningEnemy->AcceptanceRadius);

	MoveToLocationOrActorTask->OnMoveTaskFinished.AddUObject(this, &ThisClass::AttackTarget);
	MoveToLocationOrActorTask->ConditionalPerformMove();
}

void UT_SearchForTarget::AttackTarget(TEnumAsByte<EPathFollowingResult::Type> Result, AAIController* AIController)
{
	if (Result != EPathFollowingResult::Success)
	{
		StartSearch();
		return;
	}
	OwningEnemy->RotateToTarget(TargetBaseCharacter.Get());
	
	AttackDelayTask = UAbilityTask_WaitDelay::WaitDelay(this, OwningEnemy->GetTimelineLength());
	AttackDelayTask->OnFinish.AddDynamic(this, &ThisClass::Attack);
	AttackDelayTask->Activate();
}

void UT_SearchForTarget::Attack()
{
	const FGameplayTag AttackTag = TTags::TAbilities::Enemy::Attack;
	GetAbilitySystemComponentFromActorInfo()->TryActivateAbilitiesByTag(AttackTag.GetSingleTagContainer());
}
