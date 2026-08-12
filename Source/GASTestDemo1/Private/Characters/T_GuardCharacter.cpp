// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/T_GuardCharacter.h"

#include "AI/T_ShooterAIController.h"
#include "AbilitySystem/Abilities/Enemy/T_HitReact.h"
#include "AbilitySystemComponent.h"
#include "Characters/T_PlayerCharacter.h"
#include "Abilities/GameplayAbility.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Components/CapsuleComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/ShapeComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Player/Components/T_ProjectileShooterComponent.h"
#include "GameplayTags/TTags.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"
#include "UI/T_AIAwarenessWidget.h"
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
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	PrimaryActorTick.TickGroup = TG_PrePhysics;

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

	static ConstructorHelpers::FObjectFinder<USoundBase> AlertSoundAsset(
		TEXT("/Game/GASTestDemo/Audio/SC_Alert.SC_Alert"));
	AlertSound = AlertSoundAsset.Object;
}

void AT_GuardCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ThisClass, GuardAwareness);
	DOREPLIFETIME(ThisClass, GuardAIState);
	DOREPLIFETIME(ThisClass, bGuardHasVisualContact);
}

bool AT_GuardCharacter::IsWeaponReady() const
{
	return IsValid(WeaponActor) && IsValid(ProjectileShooterComponent);
}

void AT_GuardCharacter::InitializePatrolRoute()
{
	CachedPatrolWorldPoints.Reset();
	const FTransform ActorTM = GetActorTransform();
	CachedPatrolWorldPoints.Reserve(PatrolPoints.Num());
	for (const FVector& LocalPoint : PatrolPoints)
	{
		CachedPatrolWorldPoints.Add(ActorTM.TransformPosition(LocalPoint));
	}
	PatrolPointIndex = 0;
	PatrolDirection = 1;
	bPatrolRouteInitialized = true;
}

void AT_GuardCharacter::EnsurePatrolRouteInitialized()
{
	if (!bPatrolRouteInitialized) InitializePatrolRoute();
}

bool AT_GuardCharacter::IsStationaryPatrol() const
{
	if (PatrolMode == EGuardPatrolMode::Stationary) return true;
	return CachedPatrolWorldPoints.Num() < 2;
}

FVector AT_GuardCharacter::GetCurrentPatrolDestination() const
{
	if (IsStationaryPatrol()) return GetActorLocation();
	if (!CachedPatrolWorldPoints.IsValidIndex(PatrolPointIndex)) return GetActorLocation();
	return CachedPatrolWorldPoints[PatrolPointIndex];
}

void AT_GuardCharacter::ComputeNextPatrolIndex(
	EGuardPatrolMode Mode,
	int32 PointCount,
	int32 CurrentIndex,
	int32 CurrentDirection,
	int32& OutIndex,
	int32& OutDirection)
{
	OutIndex = CurrentIndex;
	OutDirection = CurrentDirection == 0 ? 1 : CurrentDirection;
	if (PointCount < 2)
	{
		OutIndex = 0;
		OutDirection = 1;
		return;
	}

	if (Mode == EGuardPatrolMode::Loop)
	{
		OutIndex = (CurrentIndex + 1) % PointCount;
		OutDirection = 1;
		return;
	}

	// PingPong
	int32 NextIndex = CurrentIndex + OutDirection;
	if (NextIndex >= PointCount)
	{
		OutDirection = -1;
		NextIndex = PointCount - 2;
	}
	else if (NextIndex < 0)
	{
		OutDirection = 1;
		NextIndex = 1;
	}
	OutIndex = FMath::Clamp(NextIndex, 0, PointCount - 1);
}

int32 AT_GuardCharacter::FindNearestPatrolIndex(const TArray<FVector>& WorldPoints, const FVector& Location)
{
	if (WorldPoints.Num() == 0) return 0;
	int32 BestIndex = 0;
	float BestDistSq = FVector::DistSquared(WorldPoints[0], Location);
	for (int32 Index = 1; Index < WorldPoints.Num(); ++Index)
	{
		const float DistSq = FVector::DistSquared(WorldPoints[Index], Location);
		if (DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			BestIndex = Index;
		}
	}
	return BestIndex;
}

float AT_GuardCharacter::GetRolledPatrolPointWaitDuration() const
{
	const float Rolled = PatrolPointWaitSeconds
		+ FMath::FRandRange(-PatrolPointWaitRandomDeviation, PatrolPointWaitRandomDeviation);
	return FMath::Max(0.f, Rolled);
}

void AT_GuardCharacter::AdvancePatrolPointIfNeeded(const FVector& PawnLocation)
{
	EnsurePatrolRouteInitialized();
	if (IsStationaryPatrol()) return;
	if (!CachedPatrolWorldPoints.IsValidIndex(PatrolPointIndex))
	{
		PatrolPointIndex = 0;
		return;
	}

	const float AcceptanceSq = FMath::Square(PatrolPointAcceptanceRadius);
	// MoveTo 默认 AcceptanceRadius=100；到点判定必须 ≥ 移动到达半径，否则会卡在“已到达但不换点”
	const float MoveArrivalRadiusSq = FMath::Square(100.f);
	const float EffectiveAcceptanceSq = FMath::Max(AcceptanceSq, MoveArrivalRadiusSq);
	const float DistSq = FVector::DistSquared(PawnLocation, CachedPatrolWorldPoints[PatrolPointIndex]);
	if (DistSq > EffectiveAcceptanceSq)
	{
		return;
	}

	AdvancePatrolPointAfterWait();
}

void AT_GuardCharacter::AdvancePatrolPointAfterWait()
{
	EnsurePatrolRouteInitialized();
	if (IsStationaryPatrol()) return;
	if (!CachedPatrolWorldPoints.IsValidIndex(PatrolPointIndex))
	{
		PatrolPointIndex = 0;
		return;
	}

	ComputeNextPatrolIndex(
		PatrolMode,
		CachedPatrolWorldPoints.Num(),
		PatrolPointIndex,
		PatrolDirection,
		PatrolPointIndex,
		PatrolDirection);
}

void AT_GuardCharacter::SnapPatrolIndexToNearest(const FVector& PawnLocation)
{
	EnsurePatrolRouteInitialized();
	if (IsStationaryPatrol()) return;
	PatrolPointIndex = FindNearestPatrolIndex(CachedPatrolWorldPoints, PawnLocation);
	PatrolDirection = 1;
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
	InitializePatrolRoute();
	if (GetNetMode() != NM_DedicatedServer && IsValid(AlertSound)) UGameplayStatics::PrimeSound(AlertSound);
	InitializeAwarenessWidget();
	RefreshAwarenessWidget();

	if (USkeletalMeshComponent* CharacterMesh = GetMesh())
	{
		CharacterMesh->PrimaryComponentTick.AddPrerequisite(this, PrimaryActorTick);
	}
}

void AT_GuardCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	// ABP_Shooter Event Graph may overwrite IsAiming/Grip each update; re-apply after anim
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

void AT_GuardCharacter::InitializeAwarenessWidget()
{
	TInlineComponentArray<UWidgetComponent*> WidgetComponents;
	GetComponents(WidgetComponents);
	for (UWidgetComponent* WidgetComponent : WidgetComponents)
	{
		if (!IsValid(WidgetComponent)) continue;
		WidgetComponent->InitWidget();
		if (UT_AIAwarenessWidget* CandidateWidget = Cast<UT_AIAwarenessWidget>(WidgetComponent->GetUserWidgetObject()))
		{
			WidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
			WidgetComponent->SetDrawAtDesiredSize(false);
			WidgetComponent->SetDrawSize(FVector2D(120.f, 44.f));
			WidgetComponent->SetPivot(FVector2D(0.5f, 1.f));
			AwarenessWidgetComponent = WidgetComponent;
			AwarenessWidget = CandidateWidget;
			break;
		}
	}
}

void AT_GuardCharacter::RefreshAwarenessWidget()
{
	if (!IsValid(AwarenessWidget)) InitializeAwarenessWidget();
	if (!IsValid(AwarenessWidgetComponent) || !IsValid(AwarenessWidget)) return;

	const bool bVisible = GuardAwareness > 0.f;
	AwarenessWidgetComponent->SetHiddenInGame(!bVisible);
	AwarenessWidget->UpdateAwareness(GuardAwareness, GuardAIState, bGuardHasVisualContact);
}

void AT_GuardCharacter::SetGuardAIPresentation(float InAwareness, ETGuardAIState InAIState, bool bHasVisualContact)
{
	if (!HasAuthority()) return;
	const float ClampedAwareness = FMath::Clamp(InAwareness, 0.f, 100.f);
	if (FMath::IsNearlyEqual(GuardAwareness, ClampedAwareness)
		&& GuardAIState == InAIState
		&& bGuardHasVisualContact == bHasVisualContact) return;
	const ETGuardAIState PreviousState = GuardAIState;
	GuardAwareness = ClampedAwareness;
	GuardAIState = InAIState;
	bGuardHasVisualContact = bHasVisualContact;
	HandleAIStateChanged(PreviousState);
	SyncAnimIsAiming();
	RefreshAwarenessWidget();
}

void AT_GuardCharacter::OnRep_GuardAwareness()
{
	SyncAnimIsAiming();
	RefreshAwarenessWidget();
}

void AT_GuardCharacter::OnRep_GuardAIState(ETGuardAIState PreviousState)
{
	HandleAIStateChanged(PreviousState);
	SyncAnimIsAiming();
	RefreshAwarenessWidget();
}

void AT_GuardCharacter::OnRep_GuardHasVisualContact()
{
	RefreshAwarenessWidget();
}

void AT_GuardCharacter::HandleAIStateChanged(ETGuardAIState PreviousState)
{
	if (AT_ShooterAIController::IsCombatEntry(PreviousState, GuardAIState)
		&& GetNetMode() != NM_DedicatedServer && IsValid(AlertSound))
	{
		UGameplayStatics::PlaySound2D(this, AlertSound);
	}
}

void AT_GuardCharacter::SetCombatStrafeEnabled(bool bEnabled)
{
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	if (!IsValid(Movement) || bCombatStrafeEnabled == bEnabled) return;

	if (bEnabled)
	{
		bSavedOrientRotationToMovement = Movement->bOrientRotationToMovement;
		bSavedUseControllerDesiredRotation = Movement->bUseControllerDesiredRotation;
		bSavedUseControllerRotationYaw = bUseControllerRotationYaw;
		SavedRotationRate = Movement->RotationRate;
		Movement->bOrientRotationToMovement = false;
		Movement->bUseControllerDesiredRotation = true;
		Movement->RotationRate.Yaw = 240.f;
		bUseControllerRotationYaw = false;
	}
	else
	{
		Movement->bOrientRotationToMovement = bSavedOrientRotationToMovement;
		Movement->bUseControllerDesiredRotation = bSavedUseControllerDesiredRotation;
		Movement->RotationRate = SavedRotationRate;
		bUseControllerRotationYaw = bSavedUseControllerRotationYaw;
	}
	bCombatStrafeEnabled = bEnabled;
}

void AT_GuardCharacter::SetReturnMovementEnabled(bool bEnabled)
{
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	if (!IsValid(Movement) || bReturnMovementEnabled == bEnabled) return;

	if (bEnabled)
	{
		bReturnSavedOrientRotationToMovement = Movement->bOrientRotationToMovement;
		bReturnSavedUseControllerDesiredRotation = Movement->bUseControllerDesiredRotation;
		bReturnSavedUseControllerRotationYaw = bUseControllerRotationYaw;
		ReturnSavedRotationRate = Movement->RotationRate;
		Movement->bOrientRotationToMovement = true;
		Movement->bUseControllerDesiredRotation = false;
		Movement->RotationRate.Yaw = 240.f;
		bUseControllerRotationYaw = false;
	}
	else
	{
		Movement->bOrientRotationToMovement = bReturnSavedOrientRotationToMovement;
		Movement->bUseControllerDesiredRotation = bReturnSavedUseControllerDesiredRotation;
		Movement->RotationRate = ReturnSavedRotationRate;
		bUseControllerRotationYaw = bReturnSavedUseControllerRotationYaw;
	}
	bReturnMovementEnabled = bEnabled;
}

void AT_GuardCharacter::NotifyHit(UPrimitiveComponent* MyComp, AActor* Other, UPrimitiveComponent* OtherComp,
	bool bSelfMoved, FVector HitLocation, FVector HitNormal, FVector NormalImpulse, const FHitResult& Hit)
{
	Super::NotifyHit(MyComp, Other, OtherComp, bSelfMoved, HitLocation, HitNormal, NormalImpulse, Hit);
	if (!HasAuthority() || !IsValid(Other) || !Other->IsA<AT_PlayerCharacter>()) return;
	if (AT_ShooterAIController* ShooterController = Cast<AT_ShooterAIController>(GetController()))
	{
		ShooterController->ConfirmTargetFromContact(Other);
	}
}

void AT_GuardCharacter::SyncAnimIsAiming()
{
	USkeletalMeshComponent* CharacterMesh = GetMesh();
	UAnimInstance* AnimInstance = IsValid(CharacterMesh) ? CharacterMesh->GetAnimInstance() : nullptr;
	if (!IsValid(AnimInstance)) return;

	UClass* AnimClass = AnimInstance->GetClass();
	const UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	const bool bAlertAiming = AT_ShooterAIController::IsAlertAimingState(GuardAIState);
	const bool bHasAimingTag = IsValid(ASC) && ASC->HasMatchingGameplayTag(TTags::State::Aiming);
	const bool bShouldAim = bAlertAiming || bHasAimingTag;

	if (FBoolProperty* AlwaysAimingProp = FindFProperty<FBoolProperty>(AnimClass, TEXT("bAlwaysAiming")))
	{
		AlwaysAimingProp->SetPropertyValue_InContainer(AnimInstance, bAlertAiming);
	}

	if (FBoolProperty* IsAimingProp = FindFProperty<FBoolProperty>(AnimClass, TEXT("IsAiming")))
	{
		IsAimingProp->SetPropertyValue_InContainer(AnimInstance, bShouldAim);
	}
	else if (FBoolProperty* AimingProp = FindFProperty<FBoolProperty>(AnimClass, TEXT("bAiming")))
	{
		AimingProp->SetPropertyValue_InContainer(AnimInstance, bShouldAim);
	}

	// ABP_Shooter: 仅警觉时切手枪握持+瞄准；巡逻用 Unarmed，避免一直举枪姿势
	constexpr uint8 PistolGripValue = 1;
	const uint8 DesiredGrip = (IsWeaponReady() && bAlertAiming) ? PistolGripValue : 0;
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

	// 非警觉时收起 ADS Slot 姿势（GuardAim 用 DynamicMontage 强制举枪）
	if (!bShouldAim)
	{
		const float SlotWeightDefault = AnimInstance->GetSlotNodeGlobalWeight(TEXT("DefaultSlot"));
		const float SlotWeightUpper = AnimInstance->GetSlotNodeGlobalWeight(TEXT("UpperBody"));
		if (SlotWeightDefault > KINDA_SMALL_NUMBER)
		{
			AnimInstance->StopSlotAnimation(0.2f, TEXT("DefaultSlot"));
		}
		if (SlotWeightUpper > KINDA_SMALL_NUMBER)
		{
			AnimInstance->StopSlotAnimation(0.2f, TEXT("UpperBody"));
		}
	}

	// 非警觉隐藏枪械，避免手持枪模型看起来像一直举枪
	const bool bShowWeapon = bAlertAiming || bHasAimingTag;
	if (IsValid(WeaponActor))
	{
		WeaponActor->SetActorHiddenInGame(!bShowWeapon);
	}
	if (IsValid(WeaponMesh))
	{
		// 不要向子组件传播：否则会把已关掉的 Sphere 线框重新显示出来
		WeaponMesh->SetHiddenInGame(!bShowWeapon, false);
	}
	// 必须在 Mesh 显隐之后再关 Shape，避免传播/Actor 显隐冲掉设置
	if (IsValid(WeaponActor))
	{
		DisableWeaponCollisionDisplay();
	}
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

	DisableWeaponCollisionDisplay();

	if (!WeaponActor->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, WeaponAttachSocketName))
	{
		UE_LOG(LogTemp, Error, TEXT("%s: failed to attach weapon %s to socket '%s'; Guard AI stopped."), *GetName(), *WeaponActor->GetName(), *WeaponAttachSocketName.ToString());
		WeaponActor->Destroy();
		WeaponActor = nullptr;
		StopGuardAI();
		return;
	}

	// 附着后组件可能重新注册，再关一次碰撞显示
	DisableWeaponCollisionDisplay();

	WeaponMesh = WeaponActor->FindComponentByClass<USkeletalMeshComponent>();
	ProjectileShooterComponent = WeaponActor->FindComponentByClass<UT_ProjectileShooterComponent>();
	if (!IsValid(WeaponMesh) || !IsValid(ProjectileShooterComponent))
	{
		UE_LOG(LogTemp, Error, TEXT("%s: weapon %s missing mesh or projectile shooter; Guard AI stopped."), *GetName(), *WeaponActor->GetName());
		StopGuardAI();
		return;
	}
}

void AT_GuardCharacter::DisableWeaponCollisionDisplay()
{
	if (!IsValid(WeaponActor))
	{
		return;
	}

	WeaponActor->SetActorEnableCollision(false);

	TInlineComponentArray<UPrimitiveComponent*> PrimitiveComponents;
	WeaponActor->GetComponents(PrimitiveComponents);

	TArray<UShapeComponent*> ShapesToDestroy;
	for (UPrimitiveComponent* PrimitiveComponent : PrimitiveComponents)
	{
		if (!IsValid(PrimitiveComponent))
		{
			continue;
		}

		const bool bIsMesh =
			PrimitiveComponent->IsA(USkeletalMeshComponent::StaticClass()) ||
			PrimitiveComponent->IsA(UStaticMeshComponent::StaticClass());

		PrimitiveComponent->SetSimulatePhysics(false);
		PrimitiveComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		PrimitiveComponent->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
		PrimitiveComponent->SetGenerateOverlapEvents(false);
		PrimitiveComponent->bAlwaysCreatePhysicsState = false;

		if (UShapeComponent* ShapeComponent = Cast<UShapeComponent>(PrimitiveComponent))
		{
			ShapesToDestroy.Add(ShapeComponent);
		}
		else if (!bIsMesh)
		{
			PrimitiveComponent->SetHiddenInGame(true);
			PrimitiveComponent->SetVisibility(false, false);
		}
	}

	// BP_Pistol 的 Sphere 会在 Show Collision / 线框下画出大红球；直接销毁
	for (UShapeComponent* ShapeComponent : ShapesToDestroy)
	{
		if (!IsValid(ShapeComponent))
		{
			continue;
		}

		if (WeaponActor->GetRootComponent() == ShapeComponent)
		{
			USkeletalMeshComponent* MeshComp = WeaponActor->FindComponentByClass<USkeletalMeshComponent>();
			if (!IsValid(MeshComp))
			{
				MeshComp = WeaponMesh.Get();
			}
			if (IsValid(MeshComp))
			{
				MeshComp->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
				WeaponActor->SetRootComponent(MeshComp);
			}
		}

		ShapeComponent->DestroyComponent();
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
