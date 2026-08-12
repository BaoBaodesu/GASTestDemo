#include "AbilitySystem/Abilities/T_Throw.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Abilities/Tasks/AbilityTask_WaitInputRelease.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimInstance.h"
#include "Characters/T_PlayerCharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/EngineTypes.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameObjects/T_ThrowTrajectoryPreview.h"
#include "GameObjects/T_Throwable.h"
#include "GameplayTags/TTags.h"
#include "Inventory/T_InventoryComponent.h"
#include "Inventory/T_ItemDefinition.h"
#include "Kismet/GameplayStatics.h"
#include "Player/Components/T_AimingComponent.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	constexpr float ThrowPreviewUpdateInterval = 0.05f;
	constexpr float ThrowPreviewMaxSimTime = 3.f;
	constexpr float ThrowPreviewSimFrequency = 15.f;
}

UT_Throw::UT_Throw()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	SetAssetTags(FGameplayTagContainer(TTags::TAbilities::Throw.GetTag()));
	ActivationOwnedTags.AddTag(TTags::State::Action::Throwing);
	ActivationBlockedTags.AddTag(TTags::State::Action::Rolling);
	ActivationBlockedTags.AddTag(TTags::State::Action::Traversing);
	ActivationBlockedTags.AddTag(TTags::State::Action::Grabbing);
	ActivationBlockedTags.AddTag(TTags::State::Action::HitReact);
	ActivationBlockedTags.AddTag(TTags::State::Action::Reloading);

	ActivationRequiredTags.AddTag(TTags::State::Aiming);
	ActivationRequiredTags.AddTag(TTags::State::ThrowableEquipped);

	static ConstructorHelpers::FObjectFinder<UAnimMontage> ThrowMontageAsset(
		TEXT("/Game/GASTestDemo/Characters/PlayerCharacters/Animations/Test/AM_Throw.AM_Throw"));
	ThrowMontage = ThrowMontageAsset.Object;

	static ConstructorHelpers::FClassFinder<AT_Throwable> ThrowableClassAsset(
		TEXT("/Game/GASTestDemo/GameObjects/Throw/BP_GlassBottle.BP_GlassBottle_C"));
	ThrowableClass = ThrowableClassAsset.Class;
}

void UT_Throw::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	StopThrowPreview();

	AT_PlayerCharacter* PlayerCharacter = Cast<AT_PlayerCharacter>(GetAvatarActorFromActorInfo());
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!IsValid(PlayerCharacter) || !IsValid(ASC) || !IsValid(ThrowMontage))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	AimingComponent = PlayerCharacter->FindComponentByClass<UT_AimingComponent>();

	// 监听蓄力就绪事件
	ChargeReadyEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this, TTags::Events::Player::Throw::ChargeReady, nullptr, true, true);
	if (IsValid(ChargeReadyEventTask))
	{
		ChargeReadyEventTask->EventReceived.AddDynamic(this, &ThisClass::OnChargeReady);
		ChargeReadyEventTask->ReadyForActivation();
	}

	// 监听出手事件
	ReleaseEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this, TTags::Events::Player::Throw::Release, nullptr, true, true);
	if (!IsValid(ReleaseEventTask))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	ReleaseEventTask->EventReceived.AddDynamic(this, &ThisClass::OnThrowReleaseEvent);
	ReleaseEventTask->ReadyForActivation();

	// 播放蒙太奇
	MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, NAME_None, ThrowMontage, 1.f, NAME_None, false);
	if (!IsValid(MontageTask))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	MontageTask->OnCompleted.AddDynamic(this, &ThisClass::OnMontageCompleted);
	MontageTask->OnBlendOut.AddDynamic(this, &ThisClass::OnMontageBlendOut);
	MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::OnMontageCancelled);
	MontageTask->OnCancelled.AddDynamic(this, &ThisClass::OnMontageCancelled);
	MontageTask->ReadyForActivation();

	// 等待松手
	WaitInputReleaseTask = UAbilityTask_WaitInputRelease::WaitInputRelease(this, false);
	if (IsValid(WaitInputReleaseTask))
	{
		WaitInputReleaseTask->OnRelease.AddDynamic(this, &ThisClass::OnInputReleased);
		WaitInputReleaseTask->ReadyForActivation();
	}

	// 最长蓄力保险
	MaxChargeDelayTask = UAbilityTask_WaitDelay::WaitDelay(this, MaxChargeDuration);
	if (IsValid(MaxChargeDelayTask))
	{
		MaxChargeDelayTask->OnFinish.AddDynamic(this, &ThisClass::OnMaxChargeReached);
		MaxChargeDelayTask->ReadyForActivation();
	}
}

void UT_Throw::OnChargeReady(FGameplayEventData Payload)
{
	if (bReleaseRequested) return;

	bCharging = true;
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (IsValid(ASC))
	{
		ASC->CurrentMontageSetPlayRate(0.f);
	}
	StartThrowPreview();
}

void UT_Throw::OnInputReleased(float TimeHeld)
{
	if (bReleaseRequested) return;
	bReleaseRequested = true;
	StopThrowPreview();

	if (bCharging)
	{
		bCharging = false;
		UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
		if (IsValid(ASC))
		{
			ASC->CurrentMontageSetPlayRate(1.f);
		}
	}
}

void UT_Throw::OnMaxChargeReached()
{
	if (bReleaseRequested) return;
	bReleaseRequested = true;
	StopThrowPreview();

	if (bCharging)
	{
		bCharging = false;
		UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
		if (IsValid(ASC))
		{
			ASC->CurrentMontageSetPlayRate(1.f);
		}
	}
}

void UT_Throw::OnThrowReleaseEvent(FGameplayEventData Payload)
{
	ExecuteThrow();
}

UT_InventoryComponent* UT_Throw::GetOwnerInventory() const
{
	AT_PlayerCharacter* PlayerCharacter = Cast<AT_PlayerCharacter>(GetAvatarActorFromActorInfo());
	return IsValid(PlayerCharacter) ? PlayerCharacter->GetInventoryComponent() : nullptr;
}

TSubclassOf<AT_Throwable> UT_Throw::ResolveThrowableClass() const
{
	UT_InventoryComponent* Inventory = GetOwnerInventory();
	UT_ItemDefinition* EquippedItem = IsValid(Inventory) ? Inventory->GetEquippedItem() : nullptr;
	if (IsValid(EquippedItem))
	{
		TSubclassOf<AActor> ActorClass = EquippedItem->EquippedActorClass;
		if (IsValid(ActorClass) && ActorClass->IsChildOf(AT_Throwable::StaticClass())) return *ActorClass;
	}
	return ThrowableClass;
}

bool UT_Throw::CalculateThrowParameters(
	FVector& OutSpawnLocation,
	FVector& OutThrowDirection,
	bool bLogMissingSocket) const
{
	AT_PlayerCharacter* PlayerCharacter = Cast<AT_PlayerCharacter>(GetAvatarActorFromActorInfo());
	if (!IsValid(PlayerCharacter)) return false;

	// 优先从手上那件投掷物的位置出手，保证预览和真实出手点完全一致
	OutSpawnLocation = PlayerCharacter->GetActorLocation()
		+ PlayerCharacter->GetActorForwardVector() * 50.f
		+ FVector(0.f, 0.f, 60.f);
	USkeletalMeshComponent* Mesh = PlayerCharacter->GetMesh();
	const AActor* HeldActor = PlayerCharacter->GetEquippedInventoryActor();
	if (IsValid(HeldActor))
	{
		OutSpawnLocation = HeldActor->GetActorLocation();
	}
	else if (IsValid(Mesh) && Mesh->DoesSocketExist(HandSocketName))
	{
		OutSpawnLocation = Mesh->GetSocketLocation(HandSocketName);
	}
	else if (bLogMissingSocket)
	{
		UE_LOG(LogTemp, Warning, TEXT("T_Throw 找不到手部 Socket '%s'，使用兜底出手点。"), *HandSocketName.ToString());
	}

	OutThrowDirection = PlayerCharacter->GetActorForwardVector();
	if (IsValid(AimingComponent))
	{
		FVector AimPoint;
		FHitResult CameraHit;
		if (AimingComponent->GetCameraAimPoint(AimPoint, CameraHit))
		{
			OutThrowDirection = (AimPoint - OutSpawnLocation).GetSafeNormal();
		}
	}
	else if (AController* Controller = PlayerCharacter->GetController())
	{
		OutThrowDirection = Controller->GetControlRotation().Vector();
	}

	FRotator ThrowRotation = OutThrowDirection.Rotation();
	ThrowRotation.Pitch = FMath::Clamp(ThrowRotation.Pitch + ThrowPitchOffset, -89.f, 89.f);
	OutThrowDirection = ThrowRotation.Vector();
	if (OutThrowDirection.IsNearlyZero()) OutThrowDirection = PlayerCharacter->GetActorForwardVector();
	return !OutThrowDirection.IsNearlyZero();
}

void UT_Throw::StartThrowPreview()
{
	AT_PlayerCharacter* PlayerCharacter = Cast<AT_PlayerCharacter>(GetAvatarActorFromActorInfo());
	UWorld* World = GetWorld();
	if (!IsValid(PlayerCharacter) || !PlayerCharacter->IsLocallyControlled() || !IsValid(World)
		|| World->GetNetMode() == NM_DedicatedServer || IsValid(ThrowPreview))
	{
		return;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = PlayerCharacter;
	SpawnParameters.Instigator = PlayerCharacter;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParameters.ObjectFlags |= RF_Transient;
	ThrowPreview = World->SpawnActor<AT_ThrowTrajectoryPreview>(
		AT_ThrowTrajectoryPreview::StaticClass(),
		FTransform::Identity,
		SpawnParameters);
	if (!IsValid(ThrowPreview)) return;

	UpdateThrowPreview();
	World->GetTimerManager().SetTimer(
		ThrowPreviewTimerHandle,
		this,
		&ThisClass::UpdateThrowPreview,
		ThrowPreviewUpdateInterval,
		true);
}

void UT_Throw::UpdateThrowPreview()
{
	UWorld* World = GetWorld();
	AT_PlayerCharacter* PlayerCharacter = Cast<AT_PlayerCharacter>(GetAvatarActorFromActorInfo());
	const TSubclassOf<AT_Throwable> ResolvedClass = ResolveThrowableClass();
	const AT_Throwable* ThrowableCDO = IsValid(ResolvedClass) ? ResolvedClass->GetDefaultObject<AT_Throwable>() : nullptr;
	if (!bCharging || !IsValid(World) || !IsValid(PlayerCharacter) || !IsValid(ThrowPreview) || !IsValid(ThrowableCDO))
	{
		StopThrowPreview();
		return;
	}

	FVector SpawnLocation;
	FVector ThrowDirection;
	if (!CalculateThrowParameters(SpawnLocation, ThrowDirection))
	{
		StopThrowPreview();
		return;
	}

	FPredictProjectilePathParams PredictParams;
	const float LaunchSpeed = ThrowableCDO->GetLaunchSpeed();
	if (LaunchSpeed <= KINDA_SMALL_NUMBER)
	{
		StopThrowPreview();
		return;
	}
	PredictParams.StartLocation = SpawnLocation;
	PredictParams.LaunchVelocity = ThrowDirection * LaunchSpeed;
	PredictParams.bTraceWithCollision = true;
	PredictParams.ProjectileRadius = ThrowableCDO->GetPredictionCollisionRadius();
	PredictParams.MaxSimTime = ThrowPreviewMaxSimTime;
	if (ThrowableCDO->GetMaxThrowDistance() > 0.f)
	{
		PredictParams.MaxSimTime = FMath::Min(PredictParams.MaxSimTime, ThrowableCDO->GetMaxThrowDistance() / LaunchSpeed);
	}
	PredictParams.bTraceWithChannel = false;
	PredictParams.ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_WorldStatic));
	PredictParams.ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_WorldDynamic));
	PredictParams.ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));
	PredictParams.ActorsToIgnore.Add(PlayerCharacter);
	if (AActor* HeldActor = PlayerCharacter->GetEquippedInventoryActor()) PredictParams.ActorsToIgnore.Add(HeldActor);
	PredictParams.SimFrequency = ThrowPreviewSimFrequency;
	PredictParams.OverrideGravityZ = World->GetGravityZ() * ThrowableCDO->GetPredictionGravityScale();
	PredictParams.DrawDebugType = EDrawDebugTrace::None;
	PredictParams.bTraceComplex = false;

	FPredictProjectilePathResult PathResult;
	UGameplayStatics::PredictProjectilePath(this, PredictParams, PathResult);
	if (ThrowableCDO->GetMaxThrowDistance() > 0.f)
	{
		const float MaxDistanceSquared = FMath::Square(ThrowableCDO->GetMaxThrowDistance());
		for (int32 PointIndex = 1; PointIndex < PathResult.PathData.Num(); ++PointIndex)
		{
			if (FVector::DistSquared(SpawnLocation, PathResult.PathData[PointIndex].Location) < MaxDistanceSquared) continue;

			PathResult.PathData.SetNum(PointIndex);
			PathResult.HitResult = FHitResult();
			break;
		}
	}
	ThrowPreview->UpdatePath(PathResult);
}

void UT_Throw::StopThrowPreview()
{
	if (UWorld* World = GetWorld()) World->GetTimerManager().ClearTimer(ThrowPreviewTimerHandle);
	if (IsValid(ThrowPreview)) ThrowPreview->Destroy();
	ThrowPreview = nullptr;
}

void UT_Throw::SetHeldThrowableHidden(bool bHidden)
{
	AT_PlayerCharacter* PlayerCharacter = Cast<AT_PlayerCharacter>(GetAvatarActorFromActorInfo());
	AActor* HeldActor = IsValid(PlayerCharacter) ? PlayerCharacter->GetEquippedInventoryActor() : nullptr;
	if (!IsValid(HeldActor)) return;

	HeldActor->SetActorHiddenInGame(bHidden);
	bHeldThrowableHidden = bHidden;
}

void UT_Throw::ConsumeThrownItem()
{
	UT_InventoryComponent* Inventory = GetOwnerInventory();
	if (!IsValid(Inventory)) return;

	UT_ItemDefinition* ThrownItem = Inventory->GetEquippedItem();
	const int32 SlotIndex = Inventory->GetEquippedSlotIndex();
	if (SlotIndex == INDEX_NONE || !Inventory->RemoveItem(SlotIndex, 1)) return;

	// 扣光当前堆会自动卸下；背包里还有同种物品时补一个到手上，避免"还有数量但已经不能投"
	if (Inventory->GetEquippedSlotIndex() != INDEX_NONE || !IsValid(ThrownItem)) return;

	const int32 NextSlotIndex = Inventory->FindItemById(ThrownItem->ItemId);
	if (NextSlotIndex != INDEX_NONE) Inventory->EquipItem(NextSlotIndex);
}

void UT_Throw::ExecuteThrow()
{
	if (bThrowExecuted) return;

	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!IsValid(AvatarActor) || !AvatarActor->HasAuthority()) return;

	const TSubclassOf<AT_Throwable> ResolvedClass = ResolveThrowableClass();
	if (!IsValid(ResolvedClass)) return;

	UWorld* World = AvatarActor->GetWorld();
	AT_PlayerCharacter* PlayerCharacter = Cast<AT_PlayerCharacter>(AvatarActor);
	if (!IsValid(World) || !IsValid(PlayerCharacter)) return;

	bThrowExecuted = true;

	FVector SpawnLocation;
	FVector ThrowDirection;
	if (!CalculateThrowParameters(SpawnLocation, ThrowDirection, true)) return;

	const FTransform SpawnTransform(ThrowDirection.Rotation(), SpawnLocation);
	AT_Throwable* Throwable = World->SpawnActorDeferred<AT_Throwable>(
		ResolvedClass,
		SpawnTransform,
		PlayerCharacter,
		PlayerCharacter,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!IsValid(Throwable))
	{
		UE_LOG(LogTemp, Warning, TEXT("T_Throw failed to spawn throwable of class %s."), *GetNameSafe(ResolvedClass));
		return;
	}

	UGameplayStatics::FinishSpawningActor(Throwable, SpawnTransform);
	Throwable->LaunchThrowable(ThrowDirection);

	// 出手瞬间隐藏手持模型，避免飞出的投掷物和手上那件同时出现；跟随动作结束后再恢复
	SetHeldThrowableHidden(true);
	ConsumeThrownItem();
}

void UT_Throw::OnMontageCompleted()
{
	FinishThrowMontage();
}

void UT_Throw::OnMontageBlendOut()
{
	// 正常播完也会先进入 blend-out，这里不能当作被打断，否则出手通知还没到能力就已经结束
	FinishThrowMontage();
}

void UT_Throw::FinishThrowMontage()
{
	const AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!bThrowExecuted && IsValid(AvatarActor) && AvatarActor->HasAuthority())
	{
		UE_LOG(LogTemp, Warning, TEXT("T_Throw 未收到 %s 通知，已在蒙太奇结束时兜底投出。"),
			*TTags::Events::Player::Throw::Release.GetTag().ToString());
		ExecuteThrow();
	}
	FinishAbility(false);
}

void UT_Throw::OnMontageCancelled()
{
	FinishAbility(true);
}

void UT_Throw::FinishAbility(bool bWasCancelled)
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, bWasCancelled);
}

void UT_Throw::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	StopThrowPreview();
	if (bCharging)
	{
		bCharging = false;
		UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
		// 只在投掷蒙太奇仍是当前蒙太奇时恢复速率，避免改到打断它的那段动画
		if (IsValid(ASC) && ASC->GetCurrentMontage() == ThrowMontage)
		{
			ASC->CurrentMontageSetPlayRate(1.f);
		}
	}

	if (bHeldThrowableHidden) SetHeldThrowableHidden(false);
	bHeldThrowableHidden = false;

	if (IsValid(ChargeReadyEventTask)) ChargeReadyEventTask->EndTask();
	if (IsValid(ReleaseEventTask)) ReleaseEventTask->EndTask();
	if (IsValid(WaitInputReleaseTask)) WaitInputReleaseTask->EndTask();
	if (IsValid(MaxChargeDelayTask)) MaxChargeDelayTask->EndTask();

	if (ActorInfo && ActorInfo->AbilitySystemComponent.IsValid())
	{
		if (FGameplayAbilitySpec* AbilitySpec = ActorInfo->AbilitySystemComponent->FindAbilitySpecFromHandle(Handle))
		{
			AbilitySpec->InputPressed = false;
		}
	}

	ChargeReadyEventTask = nullptr;
	ReleaseEventTask = nullptr;
	WaitInputReleaseTask = nullptr;
	MaxChargeDelayTask = nullptr;
	MontageTask = nullptr;
	AimingComponent = nullptr;
	bCharging = false;
	bReleaseRequested = false;
	bThrowExecuted = false;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
