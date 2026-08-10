// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/Abilities/T_GuardAim.h"

#include "AI/T_ShooterAIController.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequence.h"
#include "Characters/T_GuardCharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameplayTags/TTags.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	const FGameplayTag& GetGuardAimDeadTag()
	{
		static const FGameplayTag DeadTag = FGameplayTag::RequestGameplayTag(TEXT("TTags.Status.Dead"));
		return DeadTag;
	}
}

UT_GuardAim::UT_GuardAim()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
	SetAssetTags(FGameplayTagContainer(TTags::TAbilities::Enemy::Aim.GetTag()));
	ActivationOwnedTags.AddTag(TTags::State::Aiming);
	ActivationBlockedTags.AddTag(TTags::State::Action::HitReact);
	const FGameplayTag& DeadTag = GetGuardAimDeadTag();
	if (DeadTag.IsValid()) ActivationBlockedTags.AddTag(DeadTag);

	static ConstructorHelpers::FObjectFinder<UAnimSequence> AimIdleFinder(
		TEXT("/Game/GASTestDemo/Characters/PlayerCharacters/Animations/Shoot/MM_Pistol_Idle_ADS.MM_Pistol_Idle_ADS"));
	if (AimIdleFinder.Succeeded())
	{
		AimIdleSequence = AimIdleFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UAnimMontage> FireMontageFinder(
		TEXT("/Game/Characters/Mannequins/Anims/Pistol/MM_Pistol_Fire_Montage.MM_Pistol_Fire_Montage"));
	if (FireMontageFinder.Succeeded())
	{
		SlotReferenceMontage = FireMontageFinder.Object;
	}
}

void UT_GuardAim::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	AT_GuardCharacter* Guard = Cast<AT_GuardCharacter>(GetAvatarActorFromActorInfo());
	AT_ShooterAIController* ShooterController = IsValid(Guard) ? Cast<AT_ShooterAIController>(Guard->GetController()) : nullptr;
	if (!IsValid(ShooterController))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	CachedController = ShooterController;

	AActor* Target = ShooterController->GetCurrentTarget();
	if (!IsValid(Target))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	ShooterController->SetFocus(Target, EAIFocusPriority::Gameplay);
	Guard->SyncAnimIsAiming();
	StartAimPoseMontage(Guard);
}

void UT_GuardAim::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (CachedController.IsValid()) CachedController->ClearFocus(EAIFocusPriority::Gameplay);
	CachedController = nullptr;

	StopAimPoseMontage(GetAvatarActorFromActorInfo());

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);

	if (AT_GuardCharacter* Guard = Cast<AT_GuardCharacter>(GetAvatarActorFromActorInfo()))
	{
		Guard->SyncAnimIsAiming();
	}
}

void UT_GuardAim::RestartAimPose()
{
	if (!IsActive()) return;
	StartAimPoseMontage(GetAvatarActorFromActorInfo());
}

void UT_GuardAim::StartAimPoseMontage(AActor* AvatarActor)
{
	StopAimPoseMontage(AvatarActor);

	AT_GuardCharacter* Guard = Cast<AT_GuardCharacter>(AvatarActor);
	if (!IsValid(Guard) || !IsValid(AimIdleSequence)) return;

	USkeletalMeshComponent* CharacterMesh = Guard->GetMesh();
	UAnimInstance* AnimInstance = IsValid(CharacterMesh) ? CharacterMesh->GetAnimInstance() : nullptr;
	if (!IsValid(AnimInstance)) return;

	FName ResolvedSlot = AimSlotName;
	if (ResolvedSlot.IsNone() && IsValid(SlotReferenceMontage) && SlotReferenceMontage->SlotAnimTracks.Num() > 0)
	{
		ResolvedSlot = SlotReferenceMontage->SlotAnimTracks[0].SlotName;
	}
	if (ResolvedSlot.IsNone())
	{
		ResolvedSlot = TEXT("DefaultSlot");
	}

	ActiveAimPoseMontage = AnimInstance->PlaySlotAnimationAsDynamicMontage(
		AimIdleSequence,
		ResolvedSlot,
		AimBlendInTime,
		AimBlendOutTime,
		1.f,
		1000);
}

void UT_GuardAim::StopAimPoseMontage(AActor* AvatarActor)
{
	if (!ActiveAimPoseMontage.IsValid()) return;

	AT_GuardCharacter* Guard = Cast<AT_GuardCharacter>(AvatarActor);
	UAnimInstance* AnimInstance = (IsValid(Guard) && IsValid(Guard->GetMesh())) ? Guard->GetMesh()->GetAnimInstance() : nullptr;
	if (IsValid(AnimInstance))
	{
		AnimInstance->Montage_Stop(AimBlendOutTime, ActiveAimPoseMontage.Get());
	}
	ActiveAimPoseMontage = nullptr;
}
