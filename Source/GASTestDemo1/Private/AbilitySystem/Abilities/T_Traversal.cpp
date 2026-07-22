#include "AbilitySystem/Abilities/T_Traversal.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "AbilitySystemComponent.h"
#include "Characters/T_BaseCharacter.h"
#include "Characters/T_PlayerCharacter.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameplayTags/TTags.h"
#include "MotionWarpingComponent.h"

UT_Traversal::UT_Traversal()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	AbilityTags.AddTag(TTags::TAbilities::Traversal);
}

bool UT_Traversal::CanActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags,
	const FGameplayTagContainer* TargetTags,
	FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(
		Handle,
		ActorInfo,
		SourceTags,
		TargetTags,
		OptionalRelevantTags))
	{
		return false;
	}

	const AT_BaseCharacter* BaseCharacter =
		ActorInfo ? Cast<AT_BaseCharacter>(ActorInfo->AvatarActor.Get()) : nullptr;
	return !IsValid(BaseCharacter) || !BaseCharacter->bIsFalling;
}

void UT_Traversal::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!IsValid(Character))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	if (const AT_BaseCharacter* BaseCharacter = Cast<AT_BaseCharacter>(Character);
		IsValid(BaseCharacter) && BaseCharacter->bIsFalling)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	PlayerCharacter = Cast<AT_PlayerCharacter>(Character);

	TraversalComponent = Character->FindComponentByClass<UT_TraversalComponent>();
	CharacterMovementComponent = Character->GetCharacterMovement();

	if (!IsValid(TraversalComponent) ||
		!IsValid(CharacterMovementComponent) ||
		!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	FTraversalCheckResult Result;
	if (!TraversalComponent->DetectTraversal(Result))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (Result.ActionType == ETraversalActionType::None)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	CurrentActionType = Result.ActionType;
	CurrentTraversalResult = Result;
	TraversalStartLocation = PlayerCharacter->GetActorLocation();
	UAnimMontage* TraversalMontage = GetTraversalMontage();
	if (!IsValid(TraversalMontage))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	CharacterMovementComponent->StopMovementImmediately();
	CharacterMovementComponent->SetMovementMode(MOVE_Flying);
	if (IsValid(PlayerCharacter)) PlayerCharacter->SetTraversalCollisionEnabled(false);

	ActorInfo->AbilitySystemComponent->AddLooseGameplayTag(TTags::State::Action::Busy);
	ActorInfo->AbilitySystemComponent->AddLooseGameplayTag(TTags::State::Action::Traversing);

	if (IsValid(PlayerCharacter))
	{
		PlayerCharacter->SetCameraCollisionEnabled(false);
	}

	UpdateMotionWarpTargets(Result);

	// UAbilityTask_PlayMontageAndWait* PlayMontageTask =
	// 	UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
	// 		this,
	// 		NAME_None,
	// 		TraversalMontage
	// 	);

	UAbilityTask_PlayMontageAndWait* PlayMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this,
		NAME_None,
		TraversalMontage,
		1.f,
		NAME_None,
		false
	);

	if (!IsValid(PlayMontageTask))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	PlayMontageTask->OnCompleted.AddDynamic(this, &ThisClass::OnTraversalFinished);
	PlayMontageTask->OnInterrupted.AddDynamic(this, &ThisClass::OnTraversalCancelled);
	PlayMontageTask->OnCancelled.AddDynamic(this, &ThisClass::OnTraversalCancelled);
	PlayMontageTask->ReadyForActivation();
}

UAnimMontage* UT_Traversal::GetTraversalMontage() const
{
	switch (CurrentActionType)
	{
	case ETraversalActionType::Vault:
		return VaultMontage;
	case ETraversalActionType::Climb:
		return ClimbMontage;
	case ETraversalActionType::Mantle:
		return MantleMontage;
	default:
		return nullptr;
	}
}

void UT_Traversal::UpdateMotionWarpTargets(const FTraversalCheckResult& Result) const
{
	UMotionWarpingComponent* MotionWarpingComponent =
		GetAvatarActorFromActorInfo()->FindComponentByClass<UMotionWarpingComponent>();
	if (!IsValid(MotionWarpingComponent)) return;
	
	MotionWarpingComponent->RemoveWarpTarget(FrontLedgeWarpTargetName);
	MotionWarpingComponent->RemoveWarpTarget(BackLedgeWarpTargetName);

	MotionWarpingComponent->AddOrUpdateWarpTargetFromTransform(FrontLedgeWarpTargetName, Result.FrontLedgeWarpTarget);
	MotionWarpingComponent->AddOrUpdateWarpTargetFromTransform(BackLedgeWarpTargetName, Result.BackLedgeWarpTarget);
}

void UT_Traversal::ClearMotionWarpTargets() const
{
	UMotionWarpingComponent* MotionWarpingComponent =
		GetAvatarActorFromActorInfo()->FindComponentByClass<UMotionWarpingComponent>();
	if (!IsValid(MotionWarpingComponent)) return;

	MotionWarpingComponent->RemoveWarpTarget(FrontLedgeWarpTargetName);
	MotionWarpingComponent->RemoveWarpTarget(BackLedgeWarpTargetName);
}

void UT_Traversal::RestoreCharacterState( const bool bWasCancelled, const bool bRestoredToLanding)
{
	if (!IsValid(CharacterMovementComponent)) return;

	if (bWasCancelled)
	{
		CharacterMovementComponent->StopMovementImmediately();
	}

	CharacterMovementComponent->SetMovementMode( !bWasCancelled && bRestoredToLanding ? MOVE_Walking : MOVE_Falling);
}

bool UT_Traversal::TryRestoreTraversalCollision(const bool bWasCancelled)
{
	if (!IsValid(PlayerCharacter)) return false;
	if (!IsValid(TraversalComponent))
	{
		PlayerCharacter->SetTraversalCollisionEnabled(true);
		return false;
	}
	if (!bWasCancelled && CurrentActionType == ETraversalActionType::Vault)
	{
		PlayerCharacter->SetTraversalCollisionEnabled(true);
		return false;
	}
	if (!bWasCancelled && CurrentActionType == ETraversalActionType::Climb)
	{
		const FVector CurrentLocation = PlayerCharacter->GetActorLocation();
		if (TraversalComponent->IsCapsuleLocationClear(CurrentLocation, 0.f))
		{
			PlayerCharacter->SetTraversalCollisionEnabled(true);
			return false;
		}

		FVector CurrentStandingLocation;
		if (TraversalComponent->FindSafeStandingLocationBelow(
				CurrentLocation,
				LandingCollisionClearance,
				CurrentStandingLocation) &&
			FMath::Abs(CurrentStandingLocation.Z - CurrentLocation.Z) <=
				MaximumLandingCorrectionDistance)
		{
			PlayerCharacter->SetTraversalCollisionEnabled(true);
			return false;
		}

		const FVector SafeLandingLocation =
			CurrentTraversalResult.LandingLocation +
			FVector::UpVector * LandingCollisionClearance;
		if (TraversalComponent->IsCapsuleLocationClear(SafeLandingLocation, 0.f))
		{
			PlayerCharacter->SetActorLocation(
				SafeLandingLocation,
				false,
				nullptr,
				ETeleportType::TeleportPhysics);
			PlayerCharacter->SetTraversalCollisionEnabled(true);
			UE_LOG(LogTemp, Warning,
				TEXT("Climb ended in collision; recovered to the cached landing location."));
			return true;
		}

		UE_LOG(LogTemp, Warning,
			TEXT("Climb landing location became blocked before collision restoration."));
	}

	/*
	 * 正常完成时优先保留动画结束后的水平位置。玩家可能已经在 Montage
	 * 完成回调前开始移动，不能无条件传回检测阶段缓存的 LandingLocation。
	 */
	else if (!bWasCancelled)
	{
		const FVector CurrentLocation = PlayerCharacter->GetActorLocation();
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("Traversal End: Action=%d Current=%s Landing=%s Delta2D=%.2f DeltaZ=%.2f"),
			static_cast<int32>(CurrentActionType),
			*CurrentLocation.ToCompactString(),
			*CurrentTraversalResult.LandingLocation.ToCompactString(),
			FVector::Dist2D(CurrentLocation, CurrentTraversalResult.LandingLocation),
			FMath::Abs(CurrentLocation.Z - CurrentTraversalResult.LandingLocation.Z));

		// 动画结束位置已经能容纳完整胶囊时，保留该位置和动画速度。
		if (TraversalComponent->IsCapsuleLocationClear(CurrentLocation, 0.f))
		{
			PlayerCharacter->SetTraversalCollisionEnabled(true);
			return true;
		}

		FVector CurrentStandingLocation;
		if (TraversalComponent->FindSafeStandingLocationBelow(
			CurrentLocation,
			LandingCollisionClearance,
			CurrentStandingLocation))
		{
			const float VerticalCorrection = FMath::Abs(
				CurrentStandingLocation.Z - CurrentLocation.Z);

			if (VerticalCorrection <= MaximumLandingCorrectionDistance)
			{
				if (!FMath::IsNearlyZero(VerticalCorrection, 0.5f))
				{
					PlayerCharacter->SetActorLocation(
						CurrentStandingLocation,
						false,
						nullptr,
						ETeleportType::TeleportPhysics);
				}

				PlayerCharacter->SetTraversalCollisionEnabled(true);
				return true;
			}
		}

		if (TraversalComponent->IsCapsuleLocationClear(CurrentLocation, 0.f))
		{
			PlayerCharacter->SetTraversalCollisionEnabled(true);
			return false;
		}

		// 只有当前位置确实发生穿透时，才使用检测阶段缓存的安全落点兜底。
		const FVector SafeLandingLocation =
			CurrentTraversalResult.LandingLocation +
			FVector::UpVector * LandingCollisionClearance;

		// 最终恢复碰撞必须使用完整胶囊，不能使用检测阶段的缩小胶囊。
		if (TraversalComponent->IsCapsuleLocationClear(SafeLandingLocation, 0.f))
		{
			PlayerCharacter->SetActorLocation(
				SafeLandingLocation,
				false,
				nullptr,
				ETeleportType::TeleportPhysics);
			PlayerCharacter->SetTraversalCollisionEnabled(true);
			UE_LOG(LogTemp, Warning,
				TEXT("Traversal ended in collision; recovered to the cached landing location."));
			return true;
		}

		UE_LOG(LogTemp, Warning,
			TEXT("Traversal landing location became blocked before collision restoration."));
	}

	// 中断时优先留在当前安全位置；这里同样使用完整胶囊验证。
	if (TraversalComponent->IsCapsuleLocationClear(PlayerCharacter->GetActorLocation(), 0.f))
	{
		PlayerCharacter->SetTraversalCollisionEnabled(true);
		return false;
	}
	if (bWasCancelled && CurrentActionType == ETraversalActionType::Climb)
	{
		const FVector SafeLandingLocation =
			CurrentTraversalResult.LandingLocation +
			FVector::UpVector * LandingCollisionClearance;
		if (TraversalComponent->IsCapsuleLocationClear(SafeLandingLocation, 0.f))
		{
			PlayerCharacter->SetActorLocation(
				SafeLandingLocation,
				false,
				nullptr,
				ETeleportType::TeleportPhysics);
			PlayerCharacter->SetTraversalCollisionEnabled(true);
			return true;
		}
	}

	// 起点在 Traversal 开始前启用了碰撞，是中断或落点失效时最可靠的回退点。
	PlayerCharacter->SetActorLocation(TraversalStartLocation, false, nullptr, ETeleportType::TeleportPhysics);
	PlayerCharacter->SetTraversalCollisionEnabled(true);
	return false;
}

void UT_Traversal::OnTraversalFinished()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UT_Traversal::OnTraversalCancelled()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UT_Traversal::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	const bool bRestoredToLanding = TryRestoreTraversalCollision(bWasCancelled);
	if (IsValid(PlayerCharacter)) PlayerCharacter->SetCameraCollisionEnabled(true);
	RestoreCharacterState(bWasCancelled, bRestoredToLanding);
	ClearMotionWarpTargets();

	if (ActorInfo && ActorInfo->AbilitySystemComponent.IsValid())
	{
		ActorInfo->AbilitySystemComponent->RemoveLooseGameplayTag(TTags::State::Action::Busy);
		ActorInfo->AbilitySystemComponent->RemoveLooseGameplayTag(TTags::State::Action::Traversing);

	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
