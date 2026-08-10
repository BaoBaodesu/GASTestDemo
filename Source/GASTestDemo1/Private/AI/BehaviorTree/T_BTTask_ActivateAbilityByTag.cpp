// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/BehaviorTree/T_BTTask_ActivateAbilityByTag.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AIController.h"
#include "Abilities/GameplayAbility.h"
#include "BehaviorTree/BehaviorTreeComponent.h"

UT_BTTask_ActivateAbilityByTag::UT_BTTask_ActivateAbilityByTag()
{
	NodeName = TEXT("Activate Ability By Tag");
	bCreateNodeInstance = true;
	INIT_TASK_NODE_NOTIFY_FLAGS();
}

EBTNodeResult::Type UT_BTTask_ActivateAbilityByTag::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	APawn* Pawn = IsValid(AIController) ? AIController->GetPawn() : nullptr;
	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Pawn);
	if (!IsValid(ASC) || !AbilityTag.IsValid())
	{
		return EBTNodeResult::Failed;
	}

	const bool bActivated = ASC->TryActivateAbilitiesByTag(FGameplayTagContainer(AbilityTag));
	if (!bActivated && !bWaitForCompletion)
	{
		// 不等待完成时，能力已激活（如持续瞄准）视为任务成功
		for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
		{
			if (!Spec.Ability || !Spec.Ability->GetAssetTags().HasTagExact(AbilityTag)) continue;
			if (Spec.IsActive()) return EBTNodeResult::Succeeded;
		}
		return EBTNodeResult::Failed;
	}

	if (!bActivated)
	{
		UE_LOG(LogTemp, Warning, TEXT("UT_BTTask_ActivateAbilityByTag: 能力标签 %s 激活失败。"), *AbilityTag.ToString());
		return EBTNodeResult::Failed;
	}

	if (!bWaitForCompletion) return EBTNodeResult::Succeeded;

	OwningComponent = &OwnerComp;
	AbilitySystemComponent = ASC;
	ElapsedTime = 0.f;
	bWaitingForAbility = true;
	ASC->OnAbilityEnded.AddUObject(this, &ThisClass::OnAbilityEnded);
	return EBTNodeResult::InProgress;
}

void UT_BTTask_ActivateAbilityByTag::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	if (!bWaitingForAbility) return;

	ElapsedTime += DeltaSeconds;
	if (ElapsedTime >= ActivationTimeout)
	{
		UE_LOG(LogTemp, Warning, TEXT("UT_BTTask_ActivateAbilityByTag: 能力标签 %s 超时未结束，任务失败。"), *AbilityTag.ToString());
		FinishTask(OwnerComp, EBTNodeResult::Failed);
	}
}

void UT_BTTask_ActivateAbilityByTag::OnAbilityEnded(const FAbilityEndedData& EndedData)
{
	if (!bWaitingForAbility || !IsValid(EndedData.AbilityThatEnded)) return;

	if (EndedData.AbilityThatEnded->GetAssetTags().HasTagExact(AbilityTag))
	{
		if (UBehaviorTreeComponent* Comp = OwningComponent.Get())
		{
			FinishTask(*Comp, EBTNodeResult::Succeeded);
		}
	}
}

void UT_BTTask_ActivateAbilityByTag::FinishTask(UBehaviorTreeComponent& OwnerComp, EBTNodeResult::Type Result)
{
	if (!bWaitingForAbility) return;

	FinishLatentTask(OwnerComp, Result);
}

void UT_BTTask_ActivateAbilityByTag::OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult)
{
	if (UAbilitySystemComponent* ASC = AbilitySystemComponent.Get())
	{
		ASC->OnAbilityEnded.RemoveAll(this);
	}

	OwningComponent = nullptr;
	AbilitySystemComponent = nullptr;
	bWaitingForAbility = false;
	ElapsedTime = 0.f;

	Super::OnTaskFinished(OwnerComp, NodeMemory, TaskResult);
}
