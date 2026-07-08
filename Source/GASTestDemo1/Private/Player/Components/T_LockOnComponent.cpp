// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/Components/T_LockOnComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Camera/CameraComponent.h"
#include "Characters/T_BaseCharacter.h"
#include "Characters/T_EnemyCharacter.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameplayTags/TTags.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "UI/T_ActorWidgetComponent.h"

UT_LockOnComponent::UT_LockOnComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

// Called when the game starts
void UT_LockOnComponent::BeginPlay()
{
	Super::BeginPlay();

	OwnerCharacter = Cast<ACharacter>(GetOwner());

	if (OwnerCharacter)
	{
		PlayerController = Cast<APlayerController>(OwnerCharacter->GetController());
		MovementComponent = OwnerCharacter->GetCharacterMovement();
	}
}


void UT_LockOnComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                       FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!IsValid(CurrentTargetActor)) return;

	if (!IsValidTarget(CurrentTargetActor))
	{
		StopLockOn();
		return;
	}
	UpdateLockOnRotation(DeltaTime);
}

void UT_LockOnComponent::ToggleLockOn()
{
	if (IsValid(CurrentTargetActor))
	{
		StopLockOn();
		return;
	}

	StartLockOn();
}


void UT_LockOnComponent::StartLockOn()
{
	CurrentTargetActor = FindBestTarget();

	if (!IsValid(CurrentTargetActor)) return;

	SetLockOnMovementMode(true);

	if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OwnerCharacter))
	{
		ASC->AddLooseGameplayTag(TTags::State::LockOn);
	}

	if (AT_EnemyCharacter* EnemyCharacter = Cast<AT_EnemyCharacter>(CurrentTargetActor))
	{
		if (EnemyCharacter->LockOnWidget)
		{
			EnemyCharacter->LockOnWidget->ShowWidget();
		}
	}
}

void UT_LockOnComponent::StopLockOn()
{
	if (IsValid(CurrentTargetActor))
	{
		if (AT_EnemyCharacter* EnemyCharacter = Cast<AT_EnemyCharacter>(CurrentTargetActor))
		{
			if (EnemyCharacter->LockOnWidget)
			{
				EnemyCharacter->LockOnWidget->HideWidget();
			}
		}
	}

	CurrentTargetActor = nullptr;

	SetLockOnMovementMode(false);

	if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OwnerCharacter))
	{
		ASC->RemoveLooseGameplayTag(TTags::State::LockOn);
	}
}

AActor* UT_LockOnComponent::FindBestTarget() const
{
	if (!OwnerCharacter) return nullptr;

	TArray<AActor*> OverlappedActors;
	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(GetOwner());

	UKismetSystemLibrary::SphereOverlapActors(
		GetWorld(),
		OwnerCharacter->GetActorLocation(),
		LockOnRadius,
		{ UEngineTypes::ConvertToObjectType(ECC_Pawn) },
		TargetClassFilter,
		ActorsToIgnore,
		OverlappedActors
	);

	AActor* BestTarget = nullptr;
	float BestScore = TNumericLimits<float>::Max();

	for (AActor* Target : OverlappedActors)
	{
		if (!IsValidTarget(Target)) continue;

		const float Distance = FVector::Dist(OwnerCharacter->GetActorLocation(), Target->GetActorLocation());
		const FVector ToTarget = (Target->GetActorLocation() - OwnerCharacter->GetActorLocation()).GetSafeNormal();
		const float ForwardDot = FVector::DotProduct(OwnerCharacter->GetActorForwardVector(), ToTarget);

		if (ForwardDot < MinForwardDot) continue;

		const float Score = Distance - ForwardDot * 300.0f;

		if (Score < BestScore)
		{
			BestScore = Score;
			BestTarget = Target;
		}
	}

	return BestTarget;
}

bool UT_LockOnComponent::IsValidTarget(AActor* Target) const
{
	if (!OwnerCharacter || !IsValid(Target)) return false;

	const float Distance = FVector::Dist(OwnerCharacter->GetActorLocation(), Target->GetActorLocation());

	if (Distance > BreakDistance) return false;
	if (!HasLineOfSight(Target)) return false;

	return true;
}

bool UT_LockOnComponent::HasLineOfSight(AActor* Target) const
{
	if (!OwnerCharacter || !Target) return false;

	FHitResult HitResult;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(OwnerCharacter);
	Params.AddIgnoredActor(Target);

	const FVector Start = OwnerCharacter->GetActorLocation() + FVector(0.0f, 0.0f, 60.0f);
	const FVector End = Target->GetActorLocation() + FVector(0.0f, 0.0f, 60.0f);

	const bool bHit = GetWorld()->LineTraceSingleByChannel(
		HitResult,
		Start,
		End,
		ECC_Visibility,
		Params
	);

	return !bHit;
}

void UT_LockOnComponent::UpdateLockOnRotation(float DeltaTime)
{
	if (!OwnerCharacter || !PlayerController || !CurrentTargetActor) return;

	const FVector Start = OwnerCharacter->GetActorLocation();
	const FVector End = CurrentTargetActor->GetActorLocation();

	const FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(Start, End);
	const FRotator CurrentRotation = PlayerController->GetControlRotation();

	const FRotator TargetRotation = FRotator(CurrentRotation.Pitch, LookAtRotation.Yaw, 0.0f);
	const FRotator NewRotation = FMath::RInterpTo(CurrentRotation, TargetRotation, DeltaTime, CameraInterpSpeed);

	PlayerController->SetControlRotation(NewRotation);
}

void UT_LockOnComponent::SetLockOnMovementMode(bool bEnable)
{
	if (!OwnerCharacter || !MovementComponent) return;

	OwnerCharacter->bUseControllerRotationYaw = bEnable;

	MovementComponent->bOrientRotationToMovement = !bEnable;
	MovementComponent->bUseControllerDesiredRotation = bEnable;
}


void UT_LockOnComponent::SwitchTarget(float Direction)
{
	if (!OwnerCharacter || !IsValid(CurrentTargetActor)) return;

	TArray<AActor*> OverlappedActors;
	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(GetOwner());
	ActorsToIgnore.Add(CurrentTargetActor);

	UKismetSystemLibrary::SphereOverlapActors(
		GetWorld(),
		OwnerCharacter->GetActorLocation(),
		LockOnRadius,
		{ UEngineTypes::ConvertToObjectType(ECC_Pawn) },
		TargetClassFilter,
		ActorsToIgnore,
		OverlappedActors
	);

	AActor* BestTarget = nullptr;
	float BestScore = TNumericLimits<float>::Max();

	const FVector CameraRight = PlayerController ? PlayerController->PlayerCameraManager->GetActorRightVector() : OwnerCharacter->GetActorRightVector();

	for (AActor* Target : OverlappedActors)
	{
		if (!IsValidTarget(Target)) continue;

		const FVector ToTarget = (Target->GetActorLocation() - OwnerCharacter->GetActorLocation()).GetSafeNormal();
		const float SideDot = FVector::DotProduct(CameraRight, ToTarget);

		if (Direction > 0.0f && SideDot <= 0.1f) continue;
		if (Direction < 0.0f && SideDot >= -0.1f) continue;

		const float Score = FMath::Abs(SideDot);

		if (Score < BestScore)
		{
			BestScore = Score;
			BestTarget = Target;
		}
	}

	if (IsValid(BestTarget))
	{
		if (AT_EnemyCharacter* OldEnemy = Cast<AT_EnemyCharacter>(CurrentTargetActor))
		{
			if (OldEnemy->LockOnWidget)
			{
				OldEnemy->LockOnWidget->HideWidget();
			}
		}

		CurrentTargetActor = BestTarget;

		if (AT_EnemyCharacter* NewEnemy = Cast<AT_EnemyCharacter>(CurrentTargetActor))
		{
			if (NewEnemy->LockOnWidget)
			{
				NewEnemy->LockOnWidget->ShowWidget();
			}
		}
	}
}
