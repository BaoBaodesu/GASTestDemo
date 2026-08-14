// Fill out your copyright notice in the Description page of Project Settings.


#include "GASTestDemo1/Public/Characters/T_PlayerCharacter.h"

#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbility.h"
#include "Animation/AnimMontage.h"
#include "MotionWarpingComponent.h"
#include "AbilitySystem/T_AttributeSet.h"
#include "AbilitySystem/Abilities/T_Reload.h"
#include "AbilitySystem/Abilities/T_Throw.h"
#include "Camera/CameraComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameObjects/T_PlayerProjectile.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameplayTags/TTags.h"
#include "Player/Components/T_LockOnComponent.h"
#include "Player/T_PlayerController.h"
#include "Player/T_PlayerState.h"
#include "Player/Components/T_AimingComponent.h"
#include "Player/Components/T_GrabComponent.h"
#include "Player/Components/T_PickUpComponent.h"
#include "Player/Components/T_TraversalComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Inventory/T_InventoryComponent.h"
#include "Inventory/T_ItemDefinition.h"
#include "Net/UnrealNetwork.h"
#include "Perception/AISense_Hearing.h"
#include "Quest/T_QuestGameState.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

// Sets default values
AT_PlayerCharacter::AT_PlayerCharacter()
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.f);

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 540.f, 0.f);
	GetCharacterMovement()->JumpZVelocity = 500.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 450.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.f;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>("CameraBoom");
	CameraBoom->SetupAttachment(GetRootComponent());
	CameraBoom->SetRelativeLocation(FVector(0.f, 0.f, 100.f));
	CameraBoom->TargetArmLength = 400.f;
	CameraBoom->SocketOffset = FVector(0.f, 40.f, -25.f);
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->bInheritPitch = true;
	CameraBoom->bInheritYaw = true;
	CameraBoom->bInheritRoll = false;
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->CameraLagSpeed = 20.f;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>("FollowCamera");
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;
	FollowCamera->FieldOfView = 90.f;
	
	Tags.Add(CrashTags::Player);
	
	LockOnComponent = CreateDefaultSubobject<UT_LockOnComponent>(TEXT("LockOnComponent"));
	TraversalComponent = CreateDefaultSubobject<UT_TraversalComponent>(TEXT("TraversalComponent"));
	GrabComponent = CreateDefaultSubobject<UT_GrabComponent>(TEXT("GrabComponent"));
	AimingComponent = CreateDefaultSubobject<UT_AimingComponent>(TEXT("AimingComponent"));
	MotionWarpingComponent = CreateDefaultSubobject<UMotionWarpingComponent>(TEXT("MotionWarpingComponent"));
	PickUpComponent = CreateDefaultSubobject<UT_PickUpComponent>(TEXT("PickUpComponent"));
	InventoryComponent = CreateDefaultSubobject<UT_InventoryComponent>(TEXT("InventoryComponent"));

	static ConstructorHelpers::FObjectFinder<UAnimMontage> EquipPistolMontageAsset(
		TEXT("/Game/GASTestDemo/Characters/PlayerCharacters/Animations/Test/Shoot/AM_Equip_Pistol_Standing.AM_Equip_Pistol_Standing"));
	EquipPistolMontage = EquipPistolMontageAsset.Object;
}

void AT_PlayerCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (HasAuthority()) UpdateFootstepNoise(DeltaSeconds);
}

void AT_PlayerCharacter::SetRunInputHeld(bool bHeld)
{
	if (bRunInputHeld == bHeld) return;
	bRunInputHeld = bHeld;
	RefreshNormalMovementSpeed();
	if (!HasAuthority()) ServerSetRunInputHeld(bHeld);
}

void AT_PlayerCharacter::ServerSetRunInputHeld_Implementation(bool bHeld)
{
	bRunInputHeld = bHeld;
	RefreshNormalMovementSpeed();
}

void AT_PlayerCharacter::OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode)
{
	Super::OnMovementModeChanged(PrevMovementMode, PreviousCustomMode);

	if (IsLocallyControlled())
	{
		if (AT_PlayerController* PlayerController = Cast<AT_PlayerController>(GetController()))
		{
			PlayerController->HandleCatchMovementModeChanged(GetCharacterMovement()->MovementMode);
		}
	}

	if (UWorld* World = GetWorld()) World->GetTimerManager().SetTimerForNextTick(this, &ThisClass::RefreshNormalMovementSpeed);
}

void AT_PlayerCharacter::OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	Super::OnEndCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);
	RefreshNormalMovementSpeed();
}

bool AT_PlayerCharacter::HasSpecialMovementState() const
{
	if (bIsCrouched) return true;

	const UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	return IsValid(ASC) && (
		ASC->HasMatchingGameplayTag(TTags::State::Action::Grabbing) ||
		ASC->HasMatchingGameplayTag(TTags::State::Action::Rolling) ||
		ASC->HasMatchingGameplayTag(TTags::State::Action::Traversing));
}

void AT_PlayerCharacter::RefreshNormalMovementSpeed()
{
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	if (!IsValid(Movement) || !IsValid(AimingComponent) || HasSpecialMovementState()) return;
	if (Movement->MovementMode != MOVE_Walking && Movement->MovementMode != MOVE_NavWalking) return;

	const float TargetSpeed = bRunInputHeld ? (bHasPistolGun ? 600.f : 650.f) : 450.f;
	AimingComponent->SetUnaimedMaxWalkSpeed(TargetSpeed);
}

void AT_PlayerCharacter::OnRep_HasPistolGun()
{
	RefreshNormalMovementSpeed();
}

bool AT_PlayerCharacter::CanReportFootstepNoise(float HorizontalSpeed) const
{
	const UCharacterMovementComponent* Movement = GetCharacterMovement();
	if (!IsValid(Movement) || !Movement->IsMovingOnGround() || bIsCrouched || HorizontalSpeed < 80.f) return false;

	const UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (IsValid(ASC) && ASC->HasMatchingGameplayTag(TTags::State::Action::Rolling)) return false;
	if (IsValid(ASC))
	{
		for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
		{
			if (Spec.IsActive() && IsValid(Spec.Ability) && Spec.Ability->GetAssetTags().HasTag(TTags::TAbilities::Traversal))
			{
				return false;
			}
		}
	}
	return true;
}

void AT_PlayerCharacter::UpdateFootstepNoise(float DeltaSeconds)
{
	const float HorizontalSpeed = GetVelocity().Size2D();
	if (!CanReportFootstepNoise(HorizontalSpeed))
	{
		FootstepNoiseElapsed = 0.f;
		return;
	}

	const bool bRunning = HorizontalSpeed >= 600.f;
	const float Interval = bRunning ? 0.3f : 0.55f;
	FootstepNoiseElapsed += DeltaSeconds;
	if (FootstepNoiseElapsed < Interval) return;
	FootstepNoiseElapsed = 0.f;

	UAISense_Hearing::ReportNoiseEvent(
		this,
		GetActorLocation(),
		bRunning ? 1.f : 0.45f,
		this,
		bRunning ? 1500.f : 800.f,
		bRunning ? FName(TEXT("GuardNoise.Footstep.Run")) : FName(TEXT("GuardNoise.Footstep.Walk")));
}


void AT_PlayerCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ThisClass, bHasPistolGun);
}

UAbilitySystemComponent* AT_PlayerCharacter::GetAbilitySystemComponent() const
{
	AT_PlayerState* TPlayerState = Cast<AT_PlayerState>(GetPlayerState());
	if (!IsValid(TPlayerState)) return nullptr;
	
	return TPlayerState->GetAbilitySystemComponent();
}

UAttributeSet* AT_PlayerCharacter::GetAttributeSet() const
{
	AT_PlayerState* TPlayerState = Cast<AT_PlayerState>(GetPlayerState());
	if (!IsValid(TPlayerState)) return nullptr;
	
	return TPlayerState->GetAttributeSet();
}

USkeletalMeshComponent* AT_PlayerCharacter::GetEquippedWeaponMesh() const
{
	if (IsValid(EquippedWeaponMesh)) return EquippedWeaponMesh;

	TArray<USkeletalMeshComponent*> SkeletalMeshComponents;
	GetComponents(SkeletalMeshComponents);
	for (USkeletalMeshComponent* SkeletalMeshComponent : SkeletalMeshComponents)
	{
		if (IsValid(SkeletalMeshComponent) && SkeletalMeshComponent != GetMesh() && SkeletalMeshComponent->DoesSocketExist(TEXT("Muzzle"))) return SkeletalMeshComponent;
	}

	TArray<AActor*> AttachedActors;
	GetAttachedActors(AttachedActors);
	for (AActor* AttachedActor : AttachedActors)
	{
		if (!IsValid(AttachedActor)) continue;
		SkeletalMeshComponents.Reset();
		AttachedActor->GetComponents(SkeletalMeshComponents);
		for (USkeletalMeshComponent* SkeletalMeshComponent : SkeletalMeshComponents)
		{
			if (IsValid(SkeletalMeshComponent) && SkeletalMeshComponent->DoesSocketExist(TEXT("Muzzle"))) return SkeletalMeshComponent;
		}
	}
	return nullptr;
}

void AT_PlayerCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	
	if (!IsValid(GetAbilitySystemComponent()) || !HasAuthority()) return;
	
	GetAbilitySystemComponent()->InitAbilityActorInfo(GetPlayerState(), this);
	
	GiveStartupAbilities();
	// 换弹/投掷能力兜底授予：若蓝图未配置对应能力，直接授予原生类，保证输入可用
	if (UAbilitySystemComponent* T_ASC = GetAbilitySystemComponent())
	{
		bool bReloadGranted = false;
		bool bThrowGranted = false;
		for (const FGameplayAbilitySpec& AbilitySpec : T_ASC->GetActivatableAbilities())
		{
			if (!AbilitySpec.Ability) continue;
			if (AbilitySpec.Ability->GetAssetTags().HasTagExact(TTags::TAbilities::Reload)) bReloadGranted = true;
			if (AbilitySpec.Ability->GetAssetTags().HasTagExact(TTags::TAbilities::Throw)) bThrowGranted = true;
		}
		if (!bReloadGranted)
		{
			T_ASC->GiveAbility(FGameplayAbilitySpec(UT_Reload::StaticClass()));
		}
		if (!bThrowGranted)
		{
			T_ASC->GiveAbility(FGameplayAbilitySpec(UT_Throw::StaticClass()));
		}
	}
	InitializeAttributes();
	
	OnASCInitialized.Broadcast(GetAbilitySystemComponent(), GetAttributeSet());
	
	UT_AttributeSet* T_AttributeSet = Cast<UT_AttributeSet>(GetAttributeSet());
	if (!IsValid(T_AttributeSet)) return;
	
	GetAbilitySystemComponent()->GetGameplayAttributeValueChangeDelegate(T_AttributeSet->GetHealthAttribute()).AddUObject(this, &ThisClass::OnHealthChanged);
}

void AT_PlayerCharacter::UnPossessed()
{
	if (AT_PlayerController* PlayerController = Cast<AT_PlayerController>(GetController())) PlayerController->CancelRunAndCatch();
	else SetRunInputHeld(false);
	Super::UnPossessed();
}

void AT_PlayerCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	
	if (!IsValid(GetAbilitySystemComponent())) return;
	
	GetAbilitySystemComponent()->InitAbilityActorInfo(GetPlayerState(), this);
	OnASCInitialized.Broadcast(GetAbilitySystemComponent(), GetAttributeSet());
	
	UT_AttributeSet* T_AttributeSet = Cast<UT_AttributeSet>(GetAttributeSet());
	if (!IsValid(T_AttributeSet)) return;
	
	GetAbilitySystemComponent()->GetGameplayAttributeValueChangeDelegate(T_AttributeSet->GetHealthAttribute()).AddUObject(this, &ThisClass::OnHealthChanged);
}

void AT_PlayerCharacter::HandleDeath()
{
	if (!IsAlive()) return;
	if (AT_PlayerController* PlayerController = Cast<AT_PlayerController>(GetController())) PlayerController->CancelRunAndCatch();
	else SetRunInputHeld(false);
	Super::HandleDeath();
}

void AT_PlayerCharacter::SetCameraCollisionEnabled(bool bEnabled)
{
	if (!IsValid(CameraBoom)) return;
	
	if (!bEnabled)
	{
		if (bTraversalCameraModeActive) return;
		
		bPreviousCameraCollisionEnabled = CameraBoom->bDoCollisionTest;
		CameraBoom->bDoCollisionTest = false;
		bTraversalCameraModeActive = true;
		
		return;
	}
	
	if (!bTraversalCameraModeActive) return;
	
	CameraBoom->bDoCollisionTest = bPreviousCameraCollisionEnabled;
	bTraversalCameraModeActive = false;
}

void AT_PlayerCharacter::SetTraversalCollisionEnabled(bool bEnabled)
{
	if (!IsValid(GetCapsuleComponent())) return;
	if (!bEnabled)
	{
		if (bTraversalCollisionDisabled) return;
		PreviousTraversalCollisionEnabled = GetCapsuleComponent()->GetCollisionEnabled();
		GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		bTraversalCollisionDisabled = true;
		return;
	}
	if (!bTraversalCollisionDisabled) return;
	GetCapsuleComponent()->SetCollisionEnabled(PreviousTraversalCollisionEnabled);
	bTraversalCollisionDisabled = false;
}

void AT_PlayerCharacter::SetInvincibilityCollisionEnabled(bool bInvincible)
{
	if (bInvincibilityCollisionActive == bInvincible) return;
	bInvincibilityCollisionActive = bInvincible;

	UWorld* World = GetWorld();
	if (!IsValid(World)) return;

	// 只让飞行中的投射物忽略自身。修改胶囊对 WorldDynamic 的响应会连地面平台和可动墙体一起忽略，
	// 导致无敌窗口内踩空坠落或卡进墙里。
	for (TActorIterator<AT_PlayerProjectile> ProjectileIt(World); ProjectileIt; ++ProjectileIt)
	{
		ProjectileIt->SetMoveIgnoredActor(this, bInvincible);
	}
}

bool AT_PlayerCharacter::CanUseInventoryItem_Implementation(UT_ItemDefinition* ItemDefinition)
{
	return IsValid(ItemDefinition)
		&& ItemDefinition->bCanUse
		&& ItemDefinition->ItemType != ETItemType::Weapon;
}

bool AT_PlayerCharacter::UseInventoryItem_Implementation(UT_ItemDefinition* ItemDefinition)
{
	if (!IsValid(ItemDefinition) || !ItemDefinition->UseEffect)
	{
		UE_LOG(LogTemp, Warning, TEXT("UseInventoryItem: 物品 [%s] 未配置 UseEffect。"), *GetNameSafe(ItemDefinition));
		return false;
	}

	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!IsValid(ASC))
	{
		UE_LOG(LogTemp, Warning, TEXT("UseInventoryItem: ASC 无效，无法应用 UseEffect。"));
		return false;
	}

	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(ItemDefinition->UseEffect, 1.0f, Context);
	if (!Spec.IsValid()) return false;

	ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
	if (HasAuthority())
	{
		if (AT_QuestGameState* QuestGameState = GetWorld() ? GetWorld()->GetGameState<AT_QuestGameState>() : nullptr)
		{
			if (QuestGameState->IsForbiddenHealthItem(ItemDefinition)) QuestGameState->NotifyForbiddenHealthItemUsed();
		}
	}
	return true;
}

bool AT_PlayerCharacter::CanEquipInventoryItem_Implementation(UT_ItemDefinition* ItemDefinition)
{
	return IsValid(ItemDefinition) &&
		(ItemDefinition->ItemType == ETItemType::Weapon || ItemDefinition->ItemType == ETItemType::Throwable) &&
		IsValid(ItemDefinition->EquippedActorClass) &&
		!ItemDefinition->EquipSocketName.IsNone() &&
		IsValid(GetMesh()) &&
		GetMesh()->DoesSocketExist(ItemDefinition->EquipSocketName);
}

bool AT_PlayerCharacter::EquipInventoryItem_Implementation(UT_ItemDefinition* ItemDefinition)
{
	if (!CanEquipInventoryItem_Implementation(ItemDefinition) || !IsValid(GetWorld()))
	{
		UE_LOG(LogTemp, Warning, TEXT("EquipInventoryItem 失败：Item=%s, EquippedActorClass=%s, Socket=%s, SocketExists=%s。"),
			*GetNameSafe(ItemDefinition),
			IsValid(ItemDefinition) ? *GetNameSafe(ItemDefinition->EquippedActorClass) : TEXT("None"),
			IsValid(ItemDefinition) ? *ItemDefinition->EquipSocketName.ToString() : TEXT("None"),
			IsValid(ItemDefinition) && IsValid(GetMesh()) && GetMesh()->DoesSocketExist(ItemDefinition->EquipSocketName) ? TEXT("true") : TEXT("false"));
		return false;
	}
	bHasPistolGun = false;
	RefreshNormalMovementSpeed();

	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (IsValid(ASC)) ASC->RemoveLooseGameplayTag(TTags::State::ThrowableEquipped);

	if (IsValid(EquippedInventoryActor))
	{
		EquippedInventoryActor->Destroy();
		EquippedInventoryActor = nullptr;
		EquippedWeaponMesh = nullptr;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = this;
	SpawnParameters.Instigator = this;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	EquippedInventoryActor = GetWorld()->SpawnActor<AActor>(
		ItemDefinition->EquippedActorClass,
		GetActorTransform(),
		SpawnParameters
	);

	if (!IsValid(EquippedInventoryActor)) return false;

	TInlineComponentArray<UPrimitiveComponent*> PrimitiveComponents;
	EquippedInventoryActor->GetComponents(PrimitiveComponents);

	for (UPrimitiveComponent* PrimitiveComponent : PrimitiveComponents)
	{
		if (!IsValid(PrimitiveComponent)) continue;
		PrimitiveComponent->SetSimulatePhysics(false);
		PrimitiveComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	if (UProjectileMovementComponent* ProjMovement = EquippedInventoryActor->FindComponentByClass<UProjectileMovementComponent>())
	{
		ProjMovement->StopMovementImmediately();
		ProjMovement->Deactivate();
	}
	EquippedInventoryActor->SetLifeSpan(0.f);

	if (!EquippedInventoryActor->AttachToComponent(
		GetMesh(),
		FAttachmentTransformRules::SnapToTargetNotIncludingScale,
		ItemDefinition->EquipSocketName))
	{
		UE_LOG(LogTemp, Warning, TEXT("EquipInventoryItem 挂载失败：Item=%s, Socket=%s。"),
			*GetNameSafe(ItemDefinition), *ItemDefinition->EquipSocketName.ToString());
		EquippedInventoryActor->Destroy();
		EquippedInventoryActor = nullptr;
		return false;
	}

	// 手持投掷物的根组件是碰撞球，Snap 后需要清掉生成时残留的相对变换，保证贴合 Socket
	if (USceneComponent* EquippedRoot = EquippedInventoryActor->GetRootComponent())
	{
		EquippedRoot->SetRelativeLocationAndRotation(FVector::ZeroVector, FRotator::ZeroRotator);

		// Snap 只对齐根组件（碰撞球），投掷物 BP 的可见网格体相对根还有偏移，需要按网格体几何中心把模型拉回 Socket
		if (ItemDefinition->ItemType == ETItemType::Throwable)
		{
			if (const UStaticMeshComponent* HeldMesh = EquippedInventoryActor->FindComponentByClass<UStaticMeshComponent>())
			{
				const FVector GeometryOffset = HeldMesh->Bounds.Origin - GetMesh()->GetSocketLocation(ItemDefinition->EquipSocketName);
				if (!GeometryOffset.IsNearlyZero()) EquippedRoot->AddWorldOffset(-GeometryOffset);
			}
		}
	}

	if (ItemDefinition->ItemType == ETItemType::Weapon)
	{
		EquippedWeaponMesh = EquippedInventoryActor->FindComponentByClass<USkeletalMeshComponent>();
		if (!IsValid(EquippedWeaponMesh))
		{
			UE_LOG(LogTemp, Warning, TEXT("EquipInventoryItem 失败：武器 [%s] 没有 SkeletalMeshComponent。"), *GetNameSafe(EquippedInventoryActor));
			EquippedInventoryActor->Destroy();
			EquippedInventoryActor = nullptr;
			RefreshNormalMovementSpeed();
			return false;
		}
		bHasPistolGun = true;
		RefreshNormalMovementSpeed();
		if (IsValid(EquipPistolMontage)) PlayAnimMontage(EquipPistolMontage);
	}
	else if (ItemDefinition->ItemType == ETItemType::Throwable)
	{
		if (IsValid(ASC)) ASC->AddLooseGameplayTag(TTags::State::ThrowableEquipped);
		if (IsValid(EquipPistolMontage)) PlayAnimMontage(EquipPistolMontage);
	}

	return true;
}

bool AT_PlayerCharacter::UnequipInventoryItem_Implementation(UT_ItemDefinition* ItemDefinition)
{
	if (IsValid(EquippedInventoryActor)) EquippedInventoryActor->Destroy();

	EquippedInventoryActor = nullptr;
	EquippedWeaponMesh = nullptr;
	bHasPistolGun = false;
	RefreshNormalMovementSpeed();

	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (IsValid(ASC)) ASC->RemoveLooseGameplayTag(TTags::State::ThrowableEquipped);

	return true;
}

void AT_PlayerCharacter::ClientNotifyHitConfirmed_Implementation()
{
	UAbilitySystemComponent* AbilitySystemComponent = GetAbilitySystemComponent();
	if (!IsValid(AbilitySystemComponent)) return;

	FGameplayEventData Payload;
	Payload.EventTag = TTags::Events::Player::Shoot::Hit;
	Payload.Instigator = this;
	Payload.Target = this;
	AbilitySystemComponent->HandleGameplayEvent(TTags::Events::Player::Shoot::Hit, &Payload);
}
