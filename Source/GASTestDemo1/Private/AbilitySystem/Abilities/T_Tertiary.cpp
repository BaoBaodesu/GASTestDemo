#include "AbilitySystem/Abilities/T_Tertiary.h"

#include "Algo/Sort.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AIController.h"
#include "Animation/AnimInstance.h"
#include "BrainComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "ContextualAnimSceneActorComponent.h"
#include "ContextualAnimSceneAsset.h"
#include "ContextualAnimTypes.h"
#include "ContextualAnimUtilities.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameplayTags/TTags.h"
#include "MotionWarpingComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Utils/T_BlueprintLibrary.h"

UT_Tertiary::UT_Tertiary()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	SetAssetTags(FGameplayTagContainer(TTags::TAbilities::Tertiary.GetTag()));
	ActivationOwnedTags.AddTag(TTags::State::Action::Busy);
	ActivationOwnedTags.AddTag(TTags::State::Action::Attacking);
	BlockAbilitiesWithTag.AddTag(TTags::TAbilities::Primary);
	BlockAbilitiesWithTag.AddTag(TTags::TAbilities::Secondary);
	BlockAbilitiesWithTag.AddTag(TTags::TAbilities::Tertiary);

	static ConstructorHelpers::FObjectFinder<UContextualAnimSceneAsset> SceneAssetFinder(
		TEXT("/Game/GASTestDemo/Characters/PlayerCharacters/Animations/ContextMontage/CAS_Test.CAS_Test"));
	if (SceneAssetFinder.Succeeded())
	{
		SceneAsset = SceneAssetFinder.Object;
	}

	static ConstructorHelpers::FClassFinder<UAnimInstance> VictimFallbackAnimFinder(
		TEXT("/Game/GASTestDemo/Characters/PlayerCharacters/Animations/Test/ABP_TestShoot.ABP_TestShoot_C"));
	if (VictimFallbackAnimFinder.Succeeded())
	{
		VictimFallbackAnimClass = VictimFallbackAnimFinder.Class;
	}
}

bool UT_Tertiary::PrepareVictimForContextualScene(AActor* VictimActor)
{
	CachedVictimActor = VictimActor;
	bVictimAnimSwapped = false;
	bVictimBrainPaused = false;
	bVictimLeaveBound = false;
	CachedVictimPreviousAnimClass = nullptr;
	CachedVictimMesh = nullptr;
	CachedVictimSceneComp = nullptr;
	CachedVictimBrain = nullptr;

	ACharacter* VictimCharacter = Cast<ACharacter>(VictimActor);
	if (!IsValid(VictimCharacter)) return false;

	USkeletalMeshComponent* VictimMesh = VictimCharacter->GetMesh();
	if (!IsValid(VictimMesh)) return false;

	CachedVictimMesh = VictimMesh;
	CachedVictimSceneComp = VictimActor->FindComponentByClass<UContextualAnimSceneActorComponent>();

	// 取消 Victim 当前能力并暂停 BT，避免射击/瞄准打断 AM_Vic_Test1
	if (UAbilitySystemComponent* VictimASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(VictimCharacter))
	{
		VictimASC->CancelAllAbilities();
		if (!VictimASC->HasMatchingGameplayTag(TTags::State::Action::Busy))
		{
			VictimASC->AddLooseGameplayTag(TTags::State::Action::Busy);
		}
	}

	if (AAIController* AIController = VictimCharacter->GetController<AAIController>())
	{
		AIController->StopMovement();
		if (UBrainComponent* Brain = AIController->GetBrainComponent())
		{
			Brain->PauseLogic(TEXT("ContextualAnimVictim"));
			CachedVictimBrain = Brain;
			bVictimBrainPaused = true;
		}
	}

	UAnimInstance* AnimInstance = VictimMesh->GetAnimInstance();
	const float DefaultSlotWeight = IsValid(AnimInstance)
		? AnimInstance->GetSlotNodeGlobalWeight(TEXT("DefaultSlot"))
		: 0.f;

	if (DefaultSlotWeight > KINDA_SMALL_NUMBER)
	{
		return true;
	}

	if (!IsValid(VictimFallbackAnimClass))
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: VictimFallbackAnimClass missing; AM_Vic_Test1 may be invisible on %s."),
			*GetName(), *VictimActor->GetName());
		return false;
	}

	CachedVictimPreviousAnimClass = VictimMesh->GetAnimClass();
	VictimMesh->SetAnimInstanceClass(VictimFallbackAnimClass);
	bVictimAnimSwapped = true;

	return true;
}

void UT_Tertiary::BindVictimLeaveDelegate()
{
	if (bVictimLeaveBound || !CachedVictimSceneComp.IsValid()) return;

	CachedVictimSceneComp->OnLeftSceneDelegate.AddDynamic(this, &ThisClass::OnVictimLeftContextualAnimScene);
	bVictimLeaveBound = true;
}

void UT_Tertiary::ClearVictimMotionWarpTargets(AActor* VictimActor) const
{
	if (!IsValid(VictimActor)) return;

	UMotionWarpingComponent* MotionWarpComp = VictimActor->FindComponentByClass<UMotionWarpingComponent>();
	if (!IsValid(MotionWarpComp)) return;

	MotionWarpComp->RemoveAllWarpTargets();
}

void UT_Tertiary::PrepareAttackerStanceForContextualScene(AActor* AttackerActor)
{
	bAttackerWasCrouched = false;
	ACharacter* AttackerCharacter = Cast<ACharacter>(AttackerActor);
	if (!IsValid(AttackerCharacter)) return;

	if (!AttackerCharacter->bIsCrouched) return;

	bAttackerWasCrouched = true;
	// ACharacter::UnCrouch 只清 bWantsToCrouch，需同步调用 CMC::UnCrouch 立即恢复胶囊高度
	AttackerCharacter->UnCrouch();
	if (UCharacterMovementComponent* MoveComp = AttackerCharacter->GetCharacterMovement())
	{
		MoveComp->UnCrouch(false);
	}
}

void UT_Tertiary::RestoreAttackerStance()
{
	if (!bAttackerWasCrouched) return;

	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	ACharacter* AttackerCharacter = Cast<ACharacter>(AvatarActor);
	if (IsValid(AttackerCharacter) && !AttackerCharacter->bIsCrouched)
	{
		AttackerCharacter->Crouch();
		if (UCharacterMovementComponent* MoveComp = AttackerCharacter->GetCharacterMovement())
		{
			MoveComp->Crouch(false);
		}
	}

	bAttackerWasCrouched = false;
}

void UT_Tertiary::RestoreAttackerMovementOrientation()
{
	if (!bAttackerMovementOrientationCached) return;

	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	ACharacter* AttackerCharacter = Cast<ACharacter>(AvatarActor);
	if (IsValid(AttackerCharacter))
	{
		if (UCharacterMovementComponent* MoveComp = AttackerCharacter->GetCharacterMovement())
		{
			MoveComp->bOrientRotationToMovement = bCachedOrientRotationToMovement;
		}
	}

	bAttackerMovementOrientationCached = false;
}

void UT_Tertiary::OverrideAttackerWarpBehindVictim(AActor* AttackerActor, AActor* VictimActor, bool bSnapTransform)
{
	if (!IsValid(AttackerActor) || !IsValid(VictimActor)) return;

	UMotionWarpingComponent* MotionWarpComp = AttackerActor->FindComponentByClass<UMotionWarpingComponent>();
	if (!IsValid(MotionWarpComp)) return;

	const FVector VictimForward = VictimActor->GetActorForwardVector().GetSafeNormal2D();
	const FVector VictimLoc = VictimActor->GetActorLocation();
	const FRotator VictimRot = VictimActor->GetActorRotation();

	ACharacter* AttackerCharacter = Cast<ACharacter>(AttackerActor);

	const FVector BehindDir = -VictimForward;
	FVector BehindLoc = VictimLoc + BehindDir * AttackerBehindDistance;
	// 与站立 Victim 胶囊中心对齐，避免蹲伏胶囊中心偏低导致处决高度错误
	BehindLoc.Z = VictimLoc.Z;
	const FRotator BehindRot = VictimForward.IsNearlyZero()
		? VictimRot
		: FRotator(0.f, VictimForward.ToOrientationRotator().Yaw, 0.f);

	if (bSnapTransform)
	{
		if (IsValid(AttackerCharacter))
		{
			if (UCharacterMovementComponent* MoveComp = AttackerCharacter->GetCharacterMovement())
			{
				if (!bAttackerMovementOrientationCached)
				{
					bCachedOrientRotationToMovement = MoveComp->bOrientRotationToMovement;
					bAttackerMovementOrientationCached = true;
				}
				MoveComp->bOrientRotationToMovement = false;
				MoveComp->StopMovementImmediately();
			}
		}

		// 近距离时 Warp 平移量过小，旋转插值会飞；开场前硬对齐到身后位姿
		AttackerActor->SetActorLocationAndRotation(BehindLoc, BehindRot, false, nullptr, ETeleportType::TeleportPhysics);
	}

	// 硬对齐后清掉攻击者 Warp，避免近零平移时把 yaw 插到约 180
	MotionWarpComp->RemoveAllWarpTargets();
}

void UT_Tertiary::RestoreVictimAnimInstance(const TCHAR* Reason)
{
	(void)Reason;

	if (!bVictimAnimSwapped && !bVictimBrainPaused && !bVictimLeaveBound)
	{
		return;
	}

	if (bVictimLeaveBound && CachedVictimSceneComp.IsValid())
	{
		CachedVictimSceneComp->OnLeftSceneDelegate.RemoveDynamic(this, &ThisClass::OnVictimLeftContextualAnimScene);
		bVictimLeaveBound = false;
	}

	if (bVictimAnimSwapped && CachedVictimMesh.IsValid() && IsValid(CachedVictimPreviousAnimClass))
	{
		CachedVictimMesh->SetAnimInstanceClass(CachedVictimPreviousAnimClass);
		bVictimAnimSwapped = false;
	}

	if (CachedVictimActor.IsValid())
	{
		if (ACharacter* VictimCharacter = Cast<ACharacter>(CachedVictimActor.Get()))
		{
			if (UAbilitySystemComponent* VictimASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(VictimCharacter))
			{
				if (VictimASC->HasMatchingGameplayTag(TTags::State::Action::Busy))
				{
					VictimASC->RemoveLooseGameplayTag(TTags::State::Action::Busy);
				}
			}
		}
	}

	if (bVictimBrainPaused && CachedVictimBrain.IsValid())
	{
		CachedVictimBrain->ResumeLogic(TEXT("ContextualAnimVictim"));
		bVictimBrainPaused = false;
	}

	CachedVictimPreviousAnimClass = nullptr;
	CachedVictimMesh = nullptr;
	CachedVictimSceneComp = nullptr;
	CachedVictimBrain = nullptr;
	CachedVictimActor = nullptr;
}

void UT_Tertiary::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!IsValid(AvatarActor))
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: AvatarActor is invalid."), *GetName());
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!IsValid(SceneAsset))
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: SceneAsset is not set."), *GetName());
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UContextualAnimSceneActorComponent* SceneActorComp =
		AvatarActor->FindComponentByClass<UContextualAnimSceneActorComponent>();
	if (!IsValid(SceneActorComp))
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: %s is missing ContextualAnimSceneActorComponent."),
			*GetName(), *AvatarActor->GetName());
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	TArray<AActor*> Candidates = UT_BlueprintLibrary::HitBoxOverlapTest(
		AvatarActor,
		SearchRadius,
		SearchForwardOffset,
		0.f,
		bDrawDebugs);

	const FVector AvatarLocation = AvatarActor->GetActorLocation();
	Algo::Sort(Candidates, [AvatarLocation](const AActor* LHS, const AActor* RHS)
	{
		const float DistLHS = IsValid(LHS)
			? FVector::DistSquared(AvatarLocation, LHS->GetActorLocation())
			: TNumericLimits<float>::Max();
		const float DistRHS = IsValid(RHS)
			? FVector::DistSquared(AvatarLocation, RHS->GetActorLocation())
			: TNumericLimits<float>::Max();
		return DistLHS < DistRHS;
	});

	FContextualAnimSceneBindings Bindings;
	AActor* SelectedVictim = nullptr;
	bool bBindingsCreated = false;
	for (AActor* Candidate : Candidates)
	{
		if (!IsValid(Candidate) || Candidate == AvatarActor) continue;

		TMap<FName, FContextualAnimSceneBindingContext> Params;
		Params.Add(AttackerRole, FContextualAnimSceneBindingContext(AvatarActor));
		Params.Add(VictimRole, FContextualAnimSceneBindingContext(Candidate));

		if (UContextualAnimUtilities::BP_CreateContextualAnimSceneBindings(SceneAsset, Params, Bindings))
		{
			SelectedVictim = Candidate;
			bBindingsCreated = true;
			break;
		}
	}

	if (!bBindingsCreated || !IsValid(SelectedVictim))
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: failed to create contextual anim bindings. Candidates=%d SceneAsset=%s."),
			*GetName(), Candidates.Num(), *GetNameSafe(SceneAsset));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	PrepareVictimForContextualScene(SelectedVictim);
	PrepareAttackerStanceForContextualScene(AvatarActor);

	// 开场前硬对齐身后位姿，避免近距离 Motion Warp 旋转插飞
	OverrideAttackerWarpBehindVictim(AvatarActor, SelectedVictim, true);

	if (!SceneActorComp->StartContextualAnimScene(Bindings))
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: StartContextualAnimScene failed. Avatar=%s SceneAsset=%s."),
			*GetName(), *AvatarActor->GetName(), *GetNameSafe(SceneAsset));
		RestoreVictimAnimInstance(TEXT("start_failed"));
		RestoreAttackerStance();
		RestoreAttackerMovementOrientation();
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// Start 完成后再监听 Victim Leave，避免 JoinScene 内旧 Bindings 的 Leave 立刻还原 AnimBP
	BindVictimLeaveDelegate();
	ClearVictimMotionWarpTargets(SelectedVictim);
	OverrideAttackerWarpBehindVictim(AvatarActor, SelectedVictim, true);

	SceneActorComp->OnLeftSceneDelegate.AddDynamic(this, &ThisClass::OnLeftContextualAnimScene);
}

void UT_Tertiary::OnLeftContextualAnimScene(UContextualAnimSceneActorComponent* SceneActorComponent)
{
	if (!IsActive()) return;
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UT_Tertiary::OnVictimLeftContextualAnimScene(UContextualAnimSceneActorComponent* SceneActorComponent)
{
	RestoreVictimAnimInstance(TEXT("victim_left_scene"));
}

void UT_Tertiary::UnbindSceneLeftDelegate()
{
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!IsValid(AvatarActor)) return;

	UContextualAnimSceneActorComponent* SceneActorComp =
		AvatarActor->FindComponentByClass<UContextualAnimSceneActorComponent>();
	if (!IsValid(SceneActorComp)) return;

	SceneActorComp->OnLeftSceneDelegate.RemoveDynamic(this, &ThisClass::OnLeftContextualAnimScene);
}

void UT_Tertiary::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	UnbindSceneLeftDelegate();

	const bool bVictimStillInScene = CachedVictimSceneComp.IsValid() && CachedVictimSceneComp->IsInActiveScene();
	if (bWasCancelled || !bVictimStillInScene)
	{
		RestoreVictimAnimInstance(bWasCancelled ? TEXT("ability_cancelled") : TEXT("ability_ended"));
	}

	RestoreAttackerStance();
	RestoreAttackerMovementOrientation();

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
