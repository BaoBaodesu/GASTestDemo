// Fill out your copyright notice in the Description page of Project Settings.

#include "Player/Components/T_PickUpComponent.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"

UT_PickUpComponent::UT_PickUpComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	SetIsReplicatedByDefault(true);
}

void UT_PickUpComponent::BeginPlay()
{
	Super::BeginPlay();

	UAnimInstance* AnimInstance = GetAnimInstance();
	if (!IsValid(AnimInstance)) return;

	AnimInstance->OnPlayMontageNotifyBegin.AddDynamic(this, &ThisClass::HandleMontageNotifyBegin);
}

void UT_PickUpComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UAnimInstance* AnimInstance = GetAnimInstance();
	if (IsValid(AnimInstance)) AnimInstance->OnPlayMontageNotifyBegin.RemoveDynamic(this, &ThisClass::HandleMontageNotifyBegin);

	Super::EndPlay(EndPlayReason);
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

	ResetPickUpState();

	return bPickedUp;
}

AT_PickUpItems* UT_PickUpComponent::FindPickUpItem() const
{
	if (!IsValid(GetWorld()) || !IsValid(GetOwner())) return nullptr;

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

	const FTPickUpItemData PickedItemData = Item->GetItemData();

	if (!Item->PickUp(GetOwner())) return false;

	OnItemPickedUp.Broadcast(PickedItemData);

	return true;
}

bool UT_PickUpComponent::ValidatePickUpItem(const AT_PickUpItems* Item) const
{
	if (!IsValid(Item) || !IsValid(GetOwner()) || !Item->CanBePickedUp(GetOwner())) return false;

	const float MaximumDistance = TraceDistance + 100.f;

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

	CommitPendingItem();

	PendingItem = nullptr;
}

void UT_PickUpComponent::HandlePickUpMontageEnded(UAnimMontage* Montage, bool bInterrupted)
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