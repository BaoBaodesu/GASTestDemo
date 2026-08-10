// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/BehaviorTree/T_BTDecorator_GuardHasAmmo.h"

#include "AIController.h"
#include "AbilitySystem/T_AttributeSet.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "Characters/T_GuardCharacter.h"

UT_BTDecorator_GuardHasAmmo::UT_BTDecorator_GuardHasAmmo()
{
	NodeName = TEXT("Guard Has Ammo");
}

bool UT_BTDecorator_GuardHasAmmo::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	const AAIController* AIController = OwnerComp.GetAIOwner();
	const AT_GuardCharacter* Guard = Cast<AT_GuardCharacter>(IsValid(AIController) ? AIController->GetPawn() : nullptr);
	if (!IsValid(Guard)) return false;

	const UT_AttributeSet* AttributeSet = Cast<UT_AttributeSet>(Guard->GetAttributeSet());
	if (!IsValid(AttributeSet)) return false;

	switch (AmmoCheck)
	{
	case EGuardAmmoCheck::HasLoadedAmmo:
		return AttributeSet->GetMagazineAmmo() > 0.f;
	case EGuardAmmoCheck::MagazineEmpty:
		return AttributeSet->GetMagazineAmmo() <= 0.f;
	case EGuardAmmoCheck::NeedsReload:
		return AttributeSet->GetMagazineAmmo() < AttributeSet->GetMaxMagazineAmmo();
	case EGuardAmmoCheck::HasReserveAmmo:
		return AttributeSet->GetReserveAmmo() > 0.f;
	default:
		return false;
	}
}
