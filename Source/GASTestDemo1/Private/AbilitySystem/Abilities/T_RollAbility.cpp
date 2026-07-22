// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Abilities/T_RollAbility.h"

#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Characters/T_BaseCharacter.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "GameplayTags/TTags.h"
#include "MotionWarpingComponent.h"
#include "Player/T_PlayerController.h"


UT_RollAbility::UT_RollAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	SetAssetTags(FGameplayTagContainer(TTags::TAbilities::Roll.GetTag()));
	ActivationOwnedTags.AddTag(TTags::State::Action::Busy);
	ActivationOwnedTags.AddTag(TTags::State::Action::Rolling);
}

void UT_RollAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	APawn* AvatarPawn = Cast<APawn>(AvatarActor);
	if (!IsValid(AvatarPawn))
	{
		return;
	}

	RollDirection = UT_BlueprintLibrary::GetRollDirectionFromInput(AvatarPawn, GetMovementInput());
	RollSectionName = UT_BlueprintLibrary::GetRollDirectionName(RollDirection);
	
	const FVector RollLocalDirection = GetRollLocalDirection(RollDirection);
	const FVector RollWorldDirection = AvatarActor->GetActorRotation().RotateVector(RollLocalDirection).GetSafeNormal();
	const FVector WarpTargetLocation = AvatarActor->GetActorLocation() + RollWorldDirection * RollExtraDistance;

	UMotionWarpingComponent* MotionWarpingComponent = AvatarActor->FindComponentByClass<UMotionWarpingComponent>();
	if (IsValid(MotionWarpingComponent))
	{
		MotionWarpingComponent->AddOrUpdateWarpTargetFromLocation(
			WarpTargetName,
			WarpTargetLocation
		);
	}
	
	UAbilityTask_PlayMontageAndWait* PlayMontageTask =
		UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this,
			NAME_None,
			RollMontage,
			1.0f,
			RollSectionName,
			true,
			1.0f,
			0.0f,
			false
		);

	if (!IsValid(PlayMontageTask))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	
	PlayMontageTask->OnCompleted.AddDynamic(this, &ThisClass::OnRollFinished);
	PlayMontageTask->OnBlendOut.AddDynamic(this, &ThisClass::OnRollFinished);
	PlayMontageTask->OnInterrupted.AddDynamic(this, &ThisClass::OnRollCancelled);
	PlayMontageTask->OnCancelled.AddDynamic(this, &ThisClass::OnRollCancelled);

	PlayMontageTask->ReadyForActivation();
	
}

FVector2D UT_RollAbility::GetMovementInput() const
{
	APlayerController* PlayerController = Cast<APlayerController>(GetActorInfo().PlayerController.Get());
	if (!IsValid(PlayerController))
	{
		return FVector2D::ZeroVector;
	}

	AT_PlayerController* TPlayerController = Cast<AT_PlayerController>(PlayerController);
	if (!IsValid(TPlayerController))
	{
		return FVector2D::ZeroVector;
	}

	return FVector2D(
		TPlayerController->MovementVector.X,
		TPlayerController->MovementVector.Y
	);
}

FVector UT_RollAbility::GetRollLocalDirection(const ERollDirection& InRollDirection) const
{
	switch (InRollDirection)
	{
	case ERollDirection::Forward:
		return FVector(1.0f, 0.0f, 0.0f);

	case ERollDirection::ForwardRight:
		return FVector(1.0f, 1.0f, 0.0f);

	case ERollDirection::Right:
		return FVector(0.0f, 1.0f, 0.0f);

	case ERollDirection::BackRight:
		return FVector(-1.0f, 1.0f, 0.0f);

	case ERollDirection::Back:
		return FVector(-1.0f, 0.0f, 0.0f);

	case ERollDirection::BackLeft:
		return FVector(-1.0f, -1.0f, 0.0f);

	case ERollDirection::Left:
		return FVector(0.0f, -1.0f, 0.0f);

	case ERollDirection::ForwardLeft:
		return FVector(1.0f, -1.0f, 0.0f);

	default:
		return FVector(1.0f, 0.0f, 0.0f);
	}
}

void UT_RollAbility::OnRollFinished()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UT_RollAbility::OnRollCancelled()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UT_RollAbility::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
