// Fill out your copyright notice in the Description page of Project Settings.


#include "Notifies/AnimNotifyState_InvincibilityWindow.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Characters/T_PlayerCharacter.h"
#include "GameplayTags/TTags.h"

void UAnimNotifyState_InvincibilityWindow::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (!IsValid(MeshComp)) return;

	AActor* Owner = MeshComp->GetOwner();
	if (!IsValid(Owner)) return;

	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Owner);
	if (!IsValid(ASC)) return;

	ASC->AddLooseGameplayTag(TTags::State::Action::Invincible);

	if (AT_PlayerCharacter* PlayerCharacter = Cast<AT_PlayerCharacter>(Owner))
	{
		PlayerCharacter->SetInvincibilityCollisionEnabled(true);
	}
}

void UAnimNotifyState_InvincibilityWindow::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (!IsValid(MeshComp)) return;

	AActor* Owner = MeshComp->GetOwner();
	if (!IsValid(Owner)) return;

	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Owner);
	if (IsValid(ASC))
	{
		ASC->RemoveLooseGameplayTag(TTags::State::Action::Invincible);
	}

	if (AT_PlayerCharacter* PlayerCharacter = Cast<AT_PlayerCharacter>(Owner))
	{
		PlayerCharacter->SetInvincibilityCollisionEnabled(false);
	}
}
