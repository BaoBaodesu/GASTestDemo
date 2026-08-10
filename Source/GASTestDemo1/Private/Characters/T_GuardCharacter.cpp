// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/T_GuardCharacter.h"

#include "AI/T_ShooterAIController.h"
#include "AbilitySystem/Abilities/Enemy/T_HitReact.h"
#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbility.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Components/CapsuleComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameObjects/T_ProjectileShooterComponent.h"
#include "GameplayTags/TTags.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"
#include "UObject/EnumProperty.h"
#include "UObject/UnrealType.h"

namespace
{
	const FGameplayTag& GetGuardCharacterDeadTag()
	{
		static const FGameplayTag DeadTag = FGameplayTag::RequestGameplayTag(TEXT("TTags.Status.Dead"));
		return DeadTag;
	}
}

AT_GuardCharacter::AT_GuardCharacter()
{
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	AIControllerClass = AT_ShooterAIController::StaticClass();

	static ConstructorHelpers::FClassFinder<AActor> DefaultPistolClass(TEXT("/Game/GASTestDemo/GameObjects/BP_Pistol.BP_Pistol_C"));
	DefaultWeaponClass = DefaultPistolClass.Class;

	static ConstructorHelpers::FObjectFinder<UAnimMontage> DeathMontageAsset(
		TEXT("/Game/GASTestDemo/Characters/PlayerCharacters/Animations/Test/Death/AM_Death1.AM_Death1"));
	DeathMontage = DeathMontageAsset.Object;

	static ConstructorHelpers::FObjectFinder<UAnimMontage> HitReactMontageAsset(
		TEXT("/Game/GASTestDemo/Characters/EnemyCharacter/Animations/AM_HitReact_Ranged.AM_HitReact_Ranged"));
	HitReactMontage = HitReactMontageAsset.Object;
}

bool AT_GuardCharacter::IsWeaponReady() const
{
	return IsValid(WeaponActor) && IsValid(ProjectileShooterComponent);
}

void AT_GuardCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		SpawnDefaultWeapon();
		RegisterHitReactTagWatch();

		// HitReact if already active at grant time will not fire New event; clear immediately
		if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
		{
			if (ASC->HasMatchingGameplayTag(TTags::State::Action::HitReact))
			{
				ClearStaleHitReact();
			}
		}
	}

	RegisterAimingTagWatch();
	SyncAnimIsAiming();
}

void AT_GuardCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnregisterHitReactTagWatch();
	UnregisterAimingTagWatch();
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(HitReactStaleTimerHandle);
		World->GetTimerManager().ClearTimer(DeathDestroyTimerHandle);
	}
	Super::EndPlay(EndPlayReason);
}

void AT_GuardCharacter::SyncAnimIsAiming()
{
	USkeletalMeshComponent* CharacterMesh = GetMesh();
	UAnimInstance* AnimInstance = IsValid(CharacterMesh) ? CharacterMesh->GetAnimInstance() : nullptr;
	if (!IsValid(AnimInstance)) return;

	UClass* AnimClass = AnimInstance->GetClass();
	const UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	const bool bShouldAim = IsValid(ASC) && ASC->HasMatchingGameplayTag(TTags::State::Aiming);

	if (FBoolProperty* IsAimingProp = FindFProperty<FBoolProperty>(AnimClass, TEXT("IsAiming")))
	{
		IsAimingProp->SetPropertyValue_InContainer(AnimInstance, bShouldAim);
	}

	// ABP_Shooter uses EHowToHold via BlendListByEnum; cast to BP_ShooterCharacter may fail
	constexpr uint8 PistolGripValue = 1;
	const uint8 DesiredGrip = IsWeaponReady() ? PistolGripValue : 0;
	auto SetAnimByteOrEnum = [AnimInstance, AnimClass](const FName PropName, const uint8 Value)
	{
		if (FByteProperty* ByteProp = FindFProperty<FByteProperty>(AnimClass, PropName))
		{
			ByteProp->SetPropertyValue_InContainer(AnimInstance, Value);
			return;
		}
		if (FEnumProperty* EnumProp = FindFProperty<FEnumProperty>(AnimClass, PropName))
		{
			void* ValuePtr = EnumProp->ContainerPtrToValuePtr<void>(AnimInstance);
			if (FNumericProperty* Underlying = EnumProp->GetUnderlyingProperty())
			{
				Underlying->SetIntPropertyValue(ValuePtr, static_cast<int64>(Value));
			}
		}
	};

	SetAnimByteOrEnum(TEXT("EquipmentGripType"), DesiredGrip);
	SetAnimByteOrEnum(TEXT("GripType"), DesiredGrip);
}

void AT_GuardCharacter::RegisterAimingTagWatch()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!IsValid(ASC) || AimingTagWatchHandle.IsValid()) return;

	AimingTagWatchHandle = ASC->RegisterGameplayTagEvent(
		TTags::State::Aiming,
		EGameplayTagEventType::NewOrRemoved).AddUObject(this, &ThisClass::OnAimingTagChanged);
}

void AT_GuardCharacter::UnregisterAimingTagWatch()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (IsValid(ASC) && AimingTagWatchHandle.IsValid())
	{
		ASC->UnregisterGameplayTagEvent(AimingTagWatchHandle, TTags::State::Aiming, EGameplayTagEventType::NewOrRemoved);
	}
	AimingTagWatchHandle.Reset();
}

void AT_GuardCharacter::OnAimingTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	SyncAnimIsAiming();
}

void AT_GuardCharacter::SpawnDefaultWeapon()
{
	if (!IsValid(DefaultWeaponClass))
	{
		UE_LOG(LogTemp, Error, TEXT("%s: DefaultWeaponClass not set; Guard AI stopped."), *GetName());
		StopGuardAI();
		return;
	}

	if (!IsValid(GetMesh()) || !GetMesh()->DoesSocketExist(WeaponAttachSocketName))
	{
		UE_LOG(LogTemp, Error, TEXT("%s: mesh missing attach socket '%s'; Guard AI stopped."), *GetName(), *WeaponAttachSocketName.ToString());
		StopGuardAI();
		return;
	}

	UWorld* World = GetWorld();
	if (!IsValid(World)) return;

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = this;
	SpawnParameters.Instigator = this;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	WeaponActor = World->SpawnActor<AActor>(DefaultWeaponClass, GetActorTransform(), SpawnParameters);
	if (!IsValid(WeaponActor))
	{
		UE_LOG(LogTemp, Error, TEXT("%s: failed to spawn default weapon %s; Guard AI stopped."), *GetName(), *DefaultWeaponClass->GetName());
		StopGuardAI();
		return;
	}

	TInlineComponentArray<UPrimitiveComponent*> PrimitiveComponents;
	WeaponActor->GetComponents(PrimitiveComponents);
	for (UPrimitiveComponent* PrimitiveComponent : PrimitiveComponents)
	{
		if (!IsValid(PrimitiveComponent)) continue;
		PrimitiveComponent->SetSimulatePhysics(false);
		PrimitiveComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	if (!WeaponActor->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, WeaponAttachSocketName))
	{
		UE_LOG(LogTemp, Error, TEXT("%s: failed to attach weapon %s to socket '%s'; Guard AI stopped."), *GetName(), *WeaponActor->GetName(), *WeaponAttachSocketName.ToString());
		WeaponActor->Destroy();
		WeaponActor = nullptr;
		StopGuardAI();
		return;
	}

	WeaponMesh = WeaponActor->FindComponentByClass<USkeletalMeshComponent>();
	ProjectileShooterComponent = WeaponActor->FindComponentByClass<UT_ProjectileShooterComponent>();
	if (!IsValid(WeaponMesh) || !IsValid(ProjectileShooterComponent))
	{
		UE_LOG(LogTemp, Error, TEXT("%s: weapon %s missing mesh or projectile shooter; Guard AI stopped."), *GetName(), *WeaponActor->GetName());
		StopGuardAI();
		return;
	}
}

void AT_GuardCharacter::StopGuardAI()
{
	AT_ShooterAIController* ShooterController = Cast<AT_ShooterAIController>(GetController());
	if (IsValid(ShooterController))
	{
		ShooterController->OnGuardDied();
		return;
	}

	AAIController* AIController = GetController<AAIController>();
	if (IsValid(AIController)) AIController->StopMovement();
}

void AT_GuardCharacter::RegisterHitReactTagWatch()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!IsValid(ASC) || HitReactTagWatchHandle.IsValid()) return;

	HitReactTagWatchHandle = ASC->RegisterGameplayTagEvent(
		TTags::State::Action::HitReact,
		EGameplayTagEventType::NewOrRemoved).AddUObject(this, &ThisClass::OnHitReactTagChanged);
}

void AT_GuardCharacter::UnregisterHitReactTagWatch()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (IsValid(ASC) && HitReactTagWatchHandle.IsValid())
	{
		ASC->UnregisterGameplayTagEvent(HitReactTagWatchHandle, TTags::State::Action::HitReact, EGameplayTagEventType::NewOrRemoved);
	}
	HitReactTagWatchHandle.Reset();
}

void AT_GuardCharacter::OnHitReactTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	UWorld* World = GetWorld();
	if (!IsValid(World)) return;

	if (NewCount <= 0)
	{
		World->GetTimerManager().ClearTimer(HitReactStaleTimerHandle);
		return;
	}

	World->GetTimerManager().SetTimer(
		HitReactStaleTimerHandle,
		this,
		&ThisClass::ClearStaleHitReact,
		HitReactStaleTimeout,
		false);
}

void AT_GuardCharacter::ClearStaleHitReact()
{
	ClearHitReactPresentation();
}

void AT_GuardCharacter::ClearHitReactPresentation()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!IsValid(ASC)) return;

	for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
	{
		if (!Spec.IsActive() || !IsValid(Spec.Ability)) continue;
		if (Spec.Ability->IsA(UT_HitReact::StaticClass()) || Spec.Ability->GetClass()->GetName().Contains(TEXT("HitReact")))
		{
			ASC->CancelAbilityHandle(Spec.Handle);
		}
	}

	if (ASC->HasMatchingGameplayTag(TTags::State::Action::HitReact))
	{
		ASC->RemoveLooseGameplayTag(TTags::State::Action::HitReact);
	}
}

void AT_GuardCharacter::ClearStaleBlockingTags()
{
	ClearStaleHitReact();

	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!IsValid(ASC)) return;

	const FGameplayTag& DeadTag = GetGuardCharacterDeadTag();
	if (DeadTag.IsValid() && ASC->HasMatchingGameplayTag(DeadTag))
	{
		ASC->RemoveLooseGameplayTag(DeadTag);
	}
}

void AT_GuardCharacter::PlayHitReactPresentation(AActor* InstigatorActor)
{
	if (!IsAlive() || bDeathStarted) return;

	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (IsValid(ASC) && !ASC->HasMatchingGameplayTag(TTags::State::Action::HitReact))
	{
		ASC->AddLooseGameplayTag(TTags::State::Action::HitReact);
	}

	USkeletalMeshComponent* CharacterMesh = GetMesh();
	UAnimInstance* AnimInstance = IsValid(CharacterMesh) ? CharacterMesh->GetAnimInstance() : nullptr;
	if (!IsValid(AnimInstance) || !IsValid(HitReactMontage))
	{
		// Tag already added; stale timer clears it so shoot still gets interrupted briefly
		return;
	}

	HitReactMontageEndedDelegate.BindUObject(this, &ThisClass::OnHitReactMontageEnded);
	const float MontageLength = AnimInstance->Montage_Play(HitReactMontage);
	if (MontageLength <= 0.f)
	{
		ClearHitReactPresentation();
		return;
	}

	AnimInstance->Montage_SetEndDelegate(HitReactMontageEndedDelegate, HitReactMontage);
}

void AT_GuardCharacter::OnHitReactMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	ClearHitReactPresentation();
}

void AT_GuardCharacter::PrepareForDeathPresentation()
{
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->StopMovementImmediately();
		Movement->DisableMovement();
	}

	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	if (USkeletalMeshComponent* CharacterMesh = GetMesh())
	{
		CharacterMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

void AT_GuardCharacter::ScheduleDestroyAfterDeath(float DelaySeconds)
{
	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		DestroyGuardAfterDeath();
		return;
	}

	World->GetTimerManager().SetTimer(
		DeathDestroyTimerHandle,
		this,
		&ThisClass::DestroyGuardAfterDeath,
		FMath::Max(DelaySeconds, 0.f),
		false);
}

void AT_GuardCharacter::PlayFallbackDeathMontage()
{
	USkeletalMeshComponent* CharacterMesh = GetMesh();
	UAnimInstance* AnimInstance = IsValid(CharacterMesh) ? CharacterMesh->GetAnimInstance() : nullptr;
	if (!IsValid(AnimInstance) || !IsValid(DeathMontage))
	{
		ScheduleDestroyAfterDeath(DeathDestroyDelayAfterAnim);
		return;
	}

	float SlotWeight = -1.f;
	for (const FSlotAnimationTrack& Track : DeathMontage->SlotAnimTracks)
	{
		SlotWeight = AnimInstance->GetSlotNodeGlobalWeight(Track.SlotName);
	}

	// ABP_Shooter DefaultSlot weight is 0: Montage_Play is invisible. Play embedded sequence in SingleNode.
	const bool bShooterAnimBP = AnimInstance->GetClass()->GetName().Contains(TEXT("ABP_Shooter"));
	if (SlotWeight <= KINDA_SMALL_NUMBER || bShooterAnimBP)
	{
		UAnimSequenceBase* PlaySequence = nullptr;
		for (const FSlotAnimationTrack& Track : DeathMontage->SlotAnimTracks)
		{
			for (const FAnimSegment& Segment : Track.AnimTrack.AnimSegments)
			{
				if (UAnimSequenceBase* Ref = Segment.GetAnimReference())
				{
					PlaySequence = Ref;
					break;
				}
			}
			if (IsValid(PlaySequence)) break;
		}

		if (IsValid(PlaySequence))
		{
			const float Duration = PlaySequence->GetPlayLength();
			CharacterMesh->SetAnimInstanceClass(nullptr);
			CharacterMesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);
			CharacterMesh->PlayAnimation(PlaySequence, false);
			if (UAnimSingleNodeInstance* SingleNode = CharacterMesh->GetSingleNodeInstance())
			{
				SingleNode->SetAnimationAsset(PlaySequence, false);
				SingleNode->PlayAnim(false, 1.f, 0.f);
			}

			ScheduleDestroyAfterDeath(Duration + DeathDestroyDelayAfterAnim);
			return;
		}
	}

	FallbackDeathMontageEndedDelegate.BindUObject(this, &ThisClass::OnFallbackDeathMontageEnded);
	const float MontageLength = AnimInstance->Montage_Play(DeathMontage);
	if (MontageLength <= 0.f)
	{
		ScheduleDestroyAfterDeath(DeathDestroyDelayAfterAnim);
		return;
	}

	AnimInstance->Montage_SetEndDelegate(FallbackDeathMontageEndedDelegate, DeathMontage);
	// Backup if montage end delegate is missed
	ScheduleDestroyAfterDeath(MontageLength + DeathDestroyDelayAfterAnim);
}

void AT_GuardCharacter::OnFallbackDeathMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	ScheduleDestroyAfterDeath(DeathDestroyDelayAfterAnim);
}

void AT_GuardCharacter::DestroyGuardAfterDeath()
{
	if (bDeathDestroyed) return;
	bDeathDestroyed = true;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DeathDestroyTimerHandle);
	}

	if (IsValid(WeaponActor))
	{
		WeaponActor->Destroy();
		WeaponActor = nullptr;
		WeaponMesh = nullptr;
		ProjectileShooterComponent = nullptr;
	}

	Destroy();
}

void AT_GuardCharacter::HandleDeath()
{
	if (bDeathStarted) return;
	bDeathStarted = true;

	StopGuardAI();
	PrepareForDeathPresentation();
	ClearHitReactPresentation();

	// Do not activate GA_T_Death_*: its EndAbility interrupts C++ death presentation
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		ASC->CancelAllAbilities();
	}

	Super::HandleDeath();

	if (ShouldSkipDeathPresentation()) return;

	PlayFallbackDeathMontage();
}

void AT_GuardCharacter::ScheduleDeathDestroy(float DelaySeconds)
{
	ScheduleDestroyAfterDeath(DelaySeconds);
}

void AT_GuardCharacter::HandleRespawn()
{
	// Guard no longer respawns: Death BP revive branch destroys instead
	DestroyGuardAfterDeath();
}

void AT_GuardCharacter::ResetAttributes()
{
	// After death, block GA_T_Death ResetAttributes revive
	if (!IsAlive()) return;
	Super::ResetAttributes();
}
