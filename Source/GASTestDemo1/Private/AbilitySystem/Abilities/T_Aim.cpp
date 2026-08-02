#include "AbilitySystem/Abilities/T_Aim.h"

#include "Abilities/Tasks/AbilityTask_WaitInputRelease.h"
#include "AbilitySystemComponent.h"
#include "GameplayTags/TTags.h"
#include "Player/Components/T_AimingComponent.h"

UT_Aim::UT_Aim()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	SetAssetTags(FGameplayTagContainer(TTags::TAbilities::Aim.GetTag()));
	ActivationOwnedTags.AddTag(TTags::State::Aiming);
	ActivationBlockedTags.AddTag(TTags::State::Action::Attacking);
	ActivationBlockedTags.AddTag(TTags::State::Action::Rolling);
	ActivationBlockedTags.AddTag(TTags::State::Action::Traversing);
	ActivationBlockedTags.AddTag(TTags::State::Action::Grabbing);
	ActivationBlockedTags.AddTag(TTags::State::Action::HitReact);
}

void UT_Aim::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!IsValid(AvatarActor)) { EndAbility(Handle, ActorInfo, ActivationInfo, true, true); return; }
	AimingComponent = AvatarActor->FindComponentByClass<UT_AimingComponent>();
	if (!IsValid(AimingComponent)) { UE_LOG(LogTemp, Warning, TEXT("T_Aim could not find T_AimingComponent.")); EndAbility(Handle, ActorInfo, ActivationInfo, true, true); return; }

	AimingComponent->StartAiming();
	WaitInputReleaseTask = UAbilityTask_WaitInputRelease::WaitInputRelease(this, false);
	if (!IsValid(WaitInputReleaseTask)) { EndAbility(Handle, ActorInfo, ActivationInfo, true, true); return; }
	WaitInputReleaseTask->OnRelease.AddDynamic(this, &ThisClass::OnInputReleased);
	WaitInputReleaseTask->ReadyForActivation();
}

void UT_Aim::OnInputReleased(float TimeHeld)
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UT_Aim::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (IsValid(AimingComponent)) AimingComponent->StopAiming();
	if (IsValid(WaitInputReleaseTask)) WaitInputReleaseTask->EndTask();
	if (ActorInfo && ActorInfo->AbilitySystemComponent.IsValid())
	{
		if (FGameplayAbilitySpec* AbilitySpec = ActorInfo->AbilitySystemComponent->FindAbilitySpecFromHandle(Handle)) AbilitySpec->InputPressed = false;
	}
	WaitInputReleaseTask = nullptr;
	AimingComponent = nullptr;
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
