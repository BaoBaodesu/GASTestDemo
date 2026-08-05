// Fill out your copyright notice in the Description page of Project Settings.

#include "Player/Components/T_PickUpComponent.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "Inventory/T_InventoryComponent.h"
#include "Inventory/T_ItemDefinition.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "UObject/ConstructorHelpers.h"

UT_PickUpComponent::UT_PickUpComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.1f;

	SetIsReplicatedByDefault(true);

	static ConstructorHelpers::FObjectFinder<USoundBase> PickUpSoundAsset(TEXT("/Game/VisualSandbox/Audio/Cue/UI/PickupItem_Cue.PickupItem_Cue"));
	PickUpSound = PickUpSoundAsset.Object;
}

void UT_PickUpComponent::BeginPlay()
{
	Super::BeginPlay();

	UAnimInstance* AnimInstance = GetAnimInstance();
	if (IsValid(AnimInstance)) AnimInstance->OnPlayMontageNotifyBegin.AddDynamic(this, &ThisClass::HandleMontageNotifyBegin);
	UpdateFocusedItem();
}

void UT_PickUpComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UAnimInstance* AnimInstance = GetAnimInstance();
	if (IsValid(AnimInstance)) AnimInstance->OnPlayMontageNotifyBegin.RemoveDynamic(this, &ThisClass::HandleMontageNotifyBegin);
	SetFocusedItem(nullptr);

	Super::EndPlay(EndPlayReason);
}

void UT_PickUpComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	UpdateFocusedItem();
}

bool UT_PickUpComponent::TryPickUp()
{
	if (bPickUpInProgress) return false;

	AT_PickUpItems* Item = FindPickUpItem();
	if (!IsValid(Item)) return false;

	PendingItem = Item;
	bPickUpInProgress = true;

	UAnimInstance* AnimInstance = GetAnimInstance();

	if (IsValid(PickUpMontage) && IsValid(AnimInstance))
	{
		const float MontageDuration = AnimInstance->Montage_Play(PickUpMontage);

		if (MontageDuration > 0.f)
		{
			FOnMontageEnded MontageEndedDelegate;
			MontageEndedDelegate.BindUObject(this, &ThisClass::HandlePickUpMontageEnded);
			AnimInstance->Montage_SetEndDelegate(MontageEndedDelegate, PickUpMontage);

			return true;
		}
	}

	const bool bPickedUp = CommitPendingItem();
	if (bPickedUp && IsValid(PickUpSound)) UGameplayStatics::PlaySound2D(this, PickUpSound);

	ResetPickUpState();

	return bPickedUp;
}

AT_PickUpItems* UT_PickUpComponent::FindPickUpItem() const
{
	if (!IsValid(GetWorld()) || !IsValid(GetOwner())) return nullptr;
	if (IsValid(FocusedItem) && ValidatePickUpItem(FocusedItem)) return FocusedItem;

	FVector ViewLocation;
	FRotator ViewRotation;

	if (!GetViewPoint(ViewLocation, ViewRotation)) return nullptr;

	const FVector TraceEnd = ViewLocation + ViewRotation.Vector() * TraceDistance;

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(TPickUpTrace), false, GetOwner());

	FHitResult HitResult;

	const bool bHit = GetWorld()->SweepSingleByChannel(
		HitResult,
		ViewLocation,
		TraceEnd,
		FQuat::Identity,
		TraceChannel,
		FCollisionShape::MakeSphere(TraceRadius),
		QueryParams
	);

	if (!bHit) return nullptr;

	return Cast<AT_PickUpItems>(HitResult.GetActor());
}

void UT_PickUpComponent::UpdateFocusedItem()
{
	APawn* Pawn = Cast<APawn>(GetOwner());
	APlayerController* PlayerController = IsValid(Pawn) ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
	if (!IsValid(Pawn) || !Pawn->IsLocallyControlled() || !IsValid(PlayerController) || !IsValid(GetWorld()))
	{
		SetFocusedItem(nullptr);
		return;
	}
	if (bPickUpInProgress && IsValid(PendingItem))
	{
		SetFocusedItem(PendingItem);
		return;
	}

	int32 ViewportSizeX = 0;
	int32 ViewportSizeY = 0;
	PlayerController->GetViewportSize(ViewportSizeX, ViewportSizeY);
	if (ViewportSizeX <= 0 || ViewportSizeY <= 0)
	{
		SetFocusedItem(nullptr);
		return;
	}

	FVector ViewLocation;
	FRotator ViewRotation;
	if (!GetViewPoint(ViewLocation, ViewRotation))
	{
		SetFocusedItem(nullptr);
		return;
	}

	AT_PickUpItems* BestItem = nullptr;
	float BestScore = TNumericLimits<float>::Max();
	const FVector2D ScreenCenter(ViewportSizeX * 0.5f, ViewportSizeY * 0.5f);
	for (TActorIterator<AT_PickUpItems> ItemIterator(GetWorld()); ItemIterator; ++ItemIterator)
	{
		AT_PickUpItems* Item = *ItemIterator;
		if (!IsValid(Item) || !Item->CanBePickedUp(GetOwner())) continue;

		const FVector ToItem = Item->GetActorLocation() - ViewLocation;
		const float Distance = ToItem.Size();
		if (Distance > FocusDistance || FVector::DotProduct(ViewRotation.Vector(), ToItem.GetSafeNormal()) <= 0.f) continue;
		if (!ValidatePickUpItem(Item)) continue;

		FVector2D ScreenPosition;
		if (!PlayerController->ProjectWorldLocationToScreen(Item->GetActorLocation(), ScreenPosition, true)) continue;
		if (ScreenPosition.X < 0.f || ScreenPosition.X > ViewportSizeX || ScreenPosition.Y < 0.f || ScreenPosition.Y > ViewportSizeY) continue;

		const FVector2D NormalizedScreenOffset(
			(ScreenPosition.X - ScreenCenter.X) / ScreenCenter.X,
			(ScreenPosition.Y - ScreenCenter.Y) / ScreenCenter.Y);
		const float Score = NormalizedScreenOffset.SizeSquared() * 0.75f + FMath::Square(Distance / FocusDistance) * 0.25f;
		if (Score >= BestScore) continue;

		BestScore = Score;
		BestItem = Item;
	}

	SetFocusedItem(BestItem);
}

void UT_PickUpComponent::SetFocusedItem(AT_PickUpItems* Item)
{
	if (FocusedItem == Item) return;
	if (IsValid(FocusedItem)) FocusedItem->SetFocused(false);
	FocusedItem = Item;
	if (IsValid(FocusedItem)) FocusedItem->SetFocused(true);
}

bool UT_PickUpComponent::GetViewPoint(FVector& ViewLocation, FRotator& ViewRotation) const
{
	APawn* Pawn = Cast<APawn>(GetOwner());

	if (IsValid(Pawn) && IsValid(Pawn->GetController()))
	{
		Pawn->GetController()->GetPlayerViewPoint(ViewLocation, ViewRotation);
		return true;
	}

	if (!IsValid(GetOwner())) return false;

	GetOwner()->GetActorEyesViewPoint(ViewLocation, ViewRotation);

	return true;
}

bool UT_PickUpComponent::CommitPendingItem()
{
	if (!IsValid(PendingItem) || !IsValid(GetOwner())) return false;

	if (GetOwner()->HasAuthority()) return PickUpItem(PendingItem);

	ServerPickUpItem(PendingItem);

	return true;
}

bool UT_PickUpComponent::PickUpItem(AT_PickUpItems* Item)
{
	if (!ValidatePickUpItem(Item)) return false;

	UT_InventoryComponent* InventoryComponent = GetOwner()->FindComponentByClass<UT_InventoryComponent>();
	UT_ItemDefinition* ItemDefinition = Item->GetItemDefinition();
	if (IsValid(InventoryComponent) && IsValid(ItemDefinition))
	{
		int32 RemainingQuantity = Item->GetQuantity();
		if (!InventoryComponent->AddItem(ItemDefinition, Item->GetQuantity(), RemainingQuantity)) return false;

		const int32 PickedUpQuantity = Item->GetQuantity() - RemainingQuantity;
		FTPickUpItemData PickedItemData = Item->GetItemData();
		PickedItemData.Quantity = PickedUpQuantity;
		if (!Item->ConsumeQuantity(GetOwner(), PickedUpQuantity)) return false;

		OnItemPickedUp.Broadcast(PickedItemData);
		return true;
	}

	const FTPickUpItemData PickedItemData = Item->GetItemData();
	if (!Item->PickUp(GetOwner())) return false;

	OnItemPickedUp.Broadcast(PickedItemData);

	return true;
}

bool UT_PickUpComponent::ValidatePickUpItem(const AT_PickUpItems* Item) const
{
	if (!IsValid(Item) || !IsValid(GetOwner()) || !Item->CanBePickedUp(GetOwner())) return false;

	const float MaximumDistance = FMath::Max(TraceDistance + 100.f, FocusDistance);

	if (FVector::DistSquared(GetOwner()->GetActorLocation(), Item->GetActorLocation()) > FMath::Square(MaximumDistance)) return false;

	FVector ViewLocation;
	FRotator ViewRotation;

	if (!GetViewPoint(ViewLocation, ViewRotation) || !IsValid(GetWorld())) return false;

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(TPickUpValidationTrace), false, GetOwner());

	FHitResult HitResult;

	const bool bHit = GetWorld()->LineTraceSingleByChannel(
		HitResult,
		ViewLocation,
		Item->GetActorLocation(),
		TraceChannel,
		QueryParams
	);

	return bHit && HitResult.GetActor() == Item;
}

void UT_PickUpComponent::HandleMontageNotifyBegin(
	FName NotifyName,
	const FBranchingPointNotifyPayload& BranchingPointPayload)
{
	if (!bPickUpInProgress || NotifyName != PickUpNotifyName || !IsValid(PendingItem)) return;

	const bool bPickedUp = CommitPendingItem();
	if (bPickedUp && IsValid(PickUpSound)) UGameplayStatics::PlaySound2D(this, PickUpSound);

	PendingItem = nullptr;
}

void UT_PickUpComponent::HandlePickUpMontageEnded(UAnimMontage* Montage, bool /*bInterrupted*/)
{
	if (Montage != PickUpMontage) return;
	ResetPickUpState();
}

void UT_PickUpComponent::ResetPickUpState()
{
	PendingItem = nullptr;
	bPickUpInProgress = false;
}

UAnimInstance* UT_PickUpComponent::GetAnimInstance() const
{
	const ACharacter* Character = Cast<ACharacter>(GetOwner());
	if (!IsValid(Character) || !IsValid(Character->GetMesh())) return nullptr;

	return Character->GetMesh()->GetAnimInstance();
}

void UT_PickUpComponent::ServerPickUpItem_Implementation(AT_PickUpItems* Item)
{
	PickUpItem(Item);
}
