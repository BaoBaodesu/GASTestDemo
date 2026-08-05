// Fill out your copyright notice in the Description page of Project Settings.


#include "GASTestDemo1/Public/Characters/T_PlayerCharacter.h"

#include "AbilitySystemComponent.h"
#include "MotionWarpingComponent.h"
#include "AbilitySystem/T_AttributeSet.h"
#include "Camera/CameraComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Player/Components/T_LockOnComponent.h"
#include "Player/T_PlayerState.h"
#include "Player/Components/T_AimingComponent.h"
#include "Player/Components/T_GrabComponent.h"
#include "Player/Components/T_PickUpComponent.h"
#include "Player/Components/T_TraversalComponent.h"
#include "Inventory/T_InventoryComponent.h"
#include "Inventory/T_ItemDefinition.h"


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
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
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
	InitializeAttributes();
	
	OnASCInitialized.Broadcast(GetAbilitySystemComponent(), GetAttributeSet());
	
	UT_AttributeSet* T_AttributeSet = Cast<UT_AttributeSet>(GetAttributeSet());
	if (!IsValid(T_AttributeSet)) return;
	
	GetAbilitySystemComponent()->GetGameplayAttributeValueChangeDelegate(T_AttributeSet->GetHealthAttribute()).AddUObject(this, &ThisClass::OnHealthChanged);
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

bool AT_PlayerCharacter::CanUseInventoryItem_Implementation(UT_ItemDefinition* ItemDefinition)
{
	return IsValid(ItemDefinition) && ItemDefinition->bCanUse;
}

bool AT_PlayerCharacter::UseInventoryItem_Implementation(UT_ItemDefinition* ItemDefinition)
{
	// 具体药水或投掷物效果由角色蓝图覆写接口事件。
	return false;
}

bool AT_PlayerCharacter::CanEquipInventoryItem_Implementation(UT_ItemDefinition* ItemDefinition)
{
	return IsValid(ItemDefinition) &&
		ItemDefinition->ItemType == ETItemType::Weapon &&
		IsValid(ItemDefinition->EquippedActorClass) &&
		!ItemDefinition->EquipSocketName.IsNone() &&
		IsValid(GetMesh()) &&
		GetMesh()->DoesSocketExist(ItemDefinition->EquipSocketName);
}

bool AT_PlayerCharacter::EquipInventoryItem_Implementation(UT_ItemDefinition* ItemDefinition)
{
	if (!CanEquipInventoryItem_Implementation(ItemDefinition) || !IsValid(GetWorld())) return false;
	bHasPistolGun = false;

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

	if (!EquippedInventoryActor->AttachToComponent(
		GetMesh(),
		FAttachmentTransformRules::SnapToTargetNotIncludingScale,
		ItemDefinition->EquipSocketName))
	{
		EquippedInventoryActor->Destroy();
		EquippedInventoryActor = nullptr;
		return false;
	}

	EquippedWeaponMesh = EquippedInventoryActor->FindComponentByClass<USkeletalMeshComponent>();
	bHasPistolGun = true;
	return true;
}

bool AT_PlayerCharacter::UnequipInventoryItem_Implementation(UT_ItemDefinition* ItemDefinition)
{
	if (IsValid(EquippedInventoryActor)) EquippedInventoryActor->Destroy();

	EquippedInventoryActor = nullptr;
	EquippedWeaponMesh = nullptr;
	bHasPistolGun = false;
	return true;
}
