// Fill out your copyright notice in the Description page of Project Settings.

#include "Player/Components/Test2Component.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "MotionWarpingComponent.h"
#include "UObject/ConstructorHelpers.h"

UTest2Component::UTest2Component()
{
	PrimaryComponentTick.bCanEverTick = false;

	static ConstructorHelpers::FObjectFinder<UAnimMontage> LedgeClimbMontageObject(
		TEXT("/Game/GASTestDemo/Characters/PlayerCharacters/Animations/Test/Traversal/Climb/anim_LedgeClimb_ClimbUp_Montage.anim_LedgeClimb_ClimbUp_Montage"));

	if (LedgeClimbMontageObject.Succeeded()) LedgeClimbMontage = LedgeClimbMontageObject.Object;
}

void UTest2Component::BeginPlay()
{
	Super::BeginPlay();

	Character = Cast<ACharacter>(GetOwner());
	if (!IsValid(Character)) { UE_LOG(LogTemp, Error, TEXT("Test2Component只能添加到Character上")); return; }

	CharacterMovement = Character->GetCharacterMovement();
	CapsuleComponent = Character->GetCapsuleComponent();
	MeshComponent = Character->GetMesh();
	MotionWarpingComponent = Character->FindComponentByClass<UMotionWarpingComponent>();

	if (!IsValid(CharacterMovement) || !IsValid(CapsuleComponent) || !IsValid(MeshComponent)) UE_LOG(LogTemp, Error, TEXT("Test2Component获取角色组件失败"));
}

void UTest2Component::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(GrabTraceTimerHandle);
		World->GetTimerManager().ClearTimer(TransitionTimerHandle);
		World->GetTimerManager().ClearTimer(DetachTimerHandle);
		World->GetTimerManager().ClearTimer(BarJumpTimerHandle);
	}

	if (IsValid(MeshComponent))
	{
		if (UAnimInstance* AnimInstance = MeshComponent->GetAnimInstance())
		{
			FOnMontageEnded EmptyDelegate;
			AnimInstance->Montage_SetEndDelegate(EmptyDelegate, LedgeClimbMontage);
		}
	}

	Super::EndPlay(EndPlayReason);
}

bool UTest2Component::HasValidCharacter() const
{
	return IsValid(Character) &&
		IsValid(CharacterMovement) &&
		IsValid(CapsuleComponent) &&
		IsValid(MeshComponent);
}

void UTest2Component::StartGrabTimer()
{
	if (!HasValidCharacter() || !GetWorld()) return;

	GetWorld()->GetTimerManager().SetTimer(
		GrabTraceTimerHandle,
		this,
		&ThisClass::GrabTrace,
		GrabTraceInterval,
		true);
}

void UTest2Component::StopGrabTimer()
{
	if (!GetWorld()) return;

	GetWorld()->GetTimerManager().ClearTimer(GrabTraceTimerHandle);
}

void UTest2Component::GrabTrace()
{
	if (!HasValidCharacter() || CharacterMovement->Velocity.Z >= 0.0) return;

	TArray<AActor*> ActorsToIgnore;
	FHitResult FirstHit;

	const FVector FirstTraceBase =
		Character->GetActorLocation() +
		Character->GetActorForwardVector() * 45.0;

	const bool bFirstHit = UKismetSystemLibrary::BoxTraceSingle(
		this,
		FirstTraceBase + FVector(0.0, 0.0, 100.0),
		FirstTraceBase + FVector(0.0, 0.0, 50.0),
		FVector(25.0, 5.0, 5.0),
		Character->GetActorRotation(),
		ETraceTypeQuery::TraceTypeQuery1,
		false,
		ActorsToIgnore,
		EDrawDebugTrace::ForDuration,
		FirstHit,
		true,
		FLinearColor::Red,
		FLinearColor::Green,
		5.0f);

	if (!bFirstHit || !IsValid(FirstHit.GetActor())) return;
	if (FirstHit.bStartPenetrating && !FirstHit.GetActor()->ActorHasTag(FName(TEXT("Bar")))) return;

	if (FirstHit.GetActor()->ActorHasTag(FName(TEXT("Bar"))))
	{
		GrabType = FName(TEXT("Bar"));
		GrabOffset = 0.0;
	}
	else
	{
		GrabType = FName(TEXT("Wall"));
		GrabOffset = 120.0;
	}

	WallNormal2 = FVector(FirstHit.ImpactNormal.X, FirstHit.ImpactNormal.Y, 0.0).GetSafeNormal(0.0001);

	FHitResult TopHit;

	const FVector SecondTraceOffsetLocation =Character->GetActorLocation() + Character->GetActorForwardVector() * 5.0;

	const FVector SecondTraceStart(SecondTraceOffsetLocation.X, SecondTraceOffsetLocation.Y, FirstHit.ImpactPoint.Z);

	const FVector SecondTraceEnd =FirstHit.Location + Character->GetActorForwardVector() * 5.0;

	const bool bTopHit = UKismetSystemLibrary::BoxTraceSingle(
		this,
		SecondTraceStart,
		SecondTraceEnd,
		FVector(5.0),
		FRotator::ZeroRotator,
		ETraceTypeQuery::TraceTypeQuery1,
		false,
		ActorsToIgnore,
		EDrawDebugTrace::ForDuration,
		TopHit,
		true,
		FLinearColor::Red,
		FLinearColor::Green,
		5.0f);

	if (!bTopHit) return;

	TopTraceLocation = TopHit.Location;
	TopImpactPoint = TopHit.ImpactPoint;

	LowerCheckPoint = FVector(
		TopHit.ImpactPoint.X,
		TopHit.ImpactPoint.Y,
		TopHit.ImpactPoint.Z - GrabOffset);

	FHitResult LowerHit;

	const bool bLowerHit = UKismetSystemLibrary::BoxTraceSingle(
		this,
		LowerCheckPoint - Character->GetActorForwardVector() * 20.0,
		LowerCheckPoint + Character->GetActorForwardVector() * 60.0,
		FVector(5.0, 10.0, 10.0),
		FRotator::ZeroRotator,
		ETraceTypeQuery::TraceTypeQuery1,
		false,
		ActorsToIgnore,
		EDrawDebugTrace::ForDuration,
		LowerHit,
		true,
		FLinearColor::Red,
		FLinearColor::Green,
		5.0f);

	if (!bLowerHit) return;

	WallNormal2 = LowerHit.ImpactNormal.GetSafeNormal2D();
	BarMoveDirection = FVector::CrossProduct(LowerHit.Normal, FVector::UpVector).GetSafeNormal();

	BoundsCheck();
}

void UTest2Component::BoundsCheck()
{
	if (!HasValidCharacter()) return;

	TArray<AActor*> ActorsToIgnore;
	FHitResult RightHit;

	const FVector RightTraceStart =Character->GetActorLocation() + Character->GetActorRightVector() * 30.0 + FVector(0.0, 0.0, 75.0);

	const FVector RightTraceEnd =RightTraceStart + Character->GetActorForwardVector() * 50.0;

	const bool bRightHit = UKismetSystemLibrary::BoxTraceSingle(
		this,
		RightTraceStart,
		RightTraceEnd,
		FVector(5.0, 5.0, 25.0),
		Character->GetActorRotation(),
		ETraceTypeQuery::TraceTypeQuery1,
		false,
		ActorsToIgnore,
		EDrawDebugTrace::ForDuration,
		RightHit,
		true,
		FLinearColor::Red,
		FLinearColor::Green,
		5.0f);

	if (!bRightHit) return;

	FHitResult LeftHit;

	const FVector LeftTraceStart = Character->GetActorLocation() + Character->GetActorRightVector() * -30.0 + FVector(0.0, 0.0, 75.0);

	const FVector LeftTraceEnd = LeftTraceStart + Character->GetActorForwardVector() * 50.0;

	const bool bLeftHit = UKismetSystemLibrary::BoxTraceSingle(
		this,
		LeftTraceStart,
		LeftTraceEnd,
		FVector(5.0, 5.0, 25.0),
		Character->GetActorRotation(),
		ETraceTypeQuery::TraceTypeQuery1,
		false,
		ActorsToIgnore,
		EDrawDebugTrace::ForDuration,
		LeftHit,
		true,
		FLinearColor::Red,
		FLinearColor::Green,
		5.0f);

	if (bLeftHit) AlignGrab();
}

void UTest2Component::AlignGrab()
{
	if (!HasValidCharacter() || !GetWorld()) return;

	if (GrabType == FName(TEXT("Bar")))
	{
		GrabHeight = TopImpactPoint.Z - 146;
		DistanceToGrab = CapsuleComponent->GetScaledCapsuleRadius() - 42.0f;
	}
	else
	{
		GrabHeight = TopImpactPoint.Z - CapsuleComponent->GetUnscaledCapsuleHalfHeight() + 35.0;

		DistanceToGrab = 25.0;
	}

	bCanMove = false;

	GetWorld()->GetTimerManager().SetTimer(TransitionTimerHandle, this, &ThisClass::Transition, 0.01f, true);

	CharacterMovement->StopMovementImmediately();
	bGrabbed = true;
	StopGrabTimer();

	if (GrabType == FName(TEXT("Bar")))
	{
		CharacterMovement->GravityScale = 0.0f;
		CharacterMovement->MaxFlySpeed = 70.0f;
		CharacterMovement->bOrientRotationToMovement = false;
		CharacterMovement->bUseControllerDesiredRotation = false;
		CharacterMovement->SetPlaneConstraintNormal(FVector::UpVector);
		CharacterMovement->SetPlaneConstraintOrigin(
			FVector(0.0, 0.0, GrabHeight));
		CharacterMovement->SetPlaneConstraintEnabled(true);
		CharacterMovement->SetMovementMode(MOVE_Flying);
		bOnBar = true;
	}
	else
	{
		CharacterMovement->GravityScale = 0.0f;
		CharacterMovement->MaxFlySpeed = 70.0f;
		CharacterMovement->bOrientRotationToMovement = false;
		CharacterMovement->bUseControllerDesiredRotation = false;
		CharacterMovement->SetPlaneConstraintNormal(FVector::UpVector);
		CharacterMovement->SetPlaneConstraintOrigin(FVector(0.0, 0.0, GrabHeight));
		CharacterMovement->SetPlaneConstraintEnabled(true);
		CharacterMovement->SetMovementMode(MOVE_Flying);
		bOnBar = false;
	}
}

void UTest2Component::Transition()
{
	if (!HasValidCharacter() || !GetWorld()) return;
	
	const FVector DistanceOffset = Character->GetActorForwardVector() *DistanceToGrab;

	const FVector TargetLocation(TopTraceLocation.X - DistanceOffset.X, TopTraceLocation.Y - DistanceOffset.Y, GrabHeight);

	const FRotator WallRotation = (-WallNormal2).Rotation();

	const FRotator TargetRotation(0.0, WallRotation.Yaw, 0.0);

	const FVector NewLocation = FMath::VInterpConstantTo(Character->GetActorLocation(), TargetLocation, 0.2f, 75.0f);

	const FRotator NewRotation = FMath::RInterpConstantTo(Character->GetActorRotation(), TargetRotation, 0.2f, 75.0f);

	const bool bReachedLocation = NewLocation.Equals(TargetLocation, 1.0f);
	
	const bool bReachedRotation = NewRotation.Equals(TargetRotation, 1.0f);
	
	Character->SetActorLocationAndRotation(NewLocation, NewRotation, false, nullptr, ETeleportType::None);

	if (bReachedLocation && bReachedRotation)
	{
		Character->SetActorLocationAndRotation(TargetLocation, TargetRotation, false, nullptr, ETeleportType::None);
		bCanMove = true;
		GetWorld()->GetTimerManager().ClearTimer(TransitionTimerHandle);
	}
}

void UTest2Component::Detach()
{
	if (!HasValidCharacter()) return;

	CharacterMovement->GravityScale = 1.5f;
	CharacterMovement->MaxWalkSpeed = 600.0f;
	CharacterMovement->MaxFlySpeed = 600.0f;
	CharacterMovement->SetPlaneConstraintEnabled(false);
	bGrabbed = false;
	bOnBar = false;
	CharacterMovement->bOrientRotationToMovement = true;
	CharacterMovement->bUseControllerDesiredRotation = true;
	CharacterMovement->SetMovementMode(MOVE_Walking);
	bCanMove = true;

	if (UWorld* World = GetWorld())
	{
		if (!World->GetTimerManager().IsTimerActive(DetachTimerHandle))
		{
			World->GetTimerManager().SetTimer(
				DetachTimerHandle,
				this,
				&ThisClass::StopGrabTimer,
				0.2f,
				false);
		}
	}
}

void UTest2Component::Shimmy(double Direction)
{
	if (!HasValidCharacter() || !bGrabbed || !bCanMove) return;

	CharacterMovement->bOrientRotationToMovement = false;
	CharacterMovement->bUseControllerDesiredRotation = false;

	if (FMath::IsNearlyZero(Direction, 0.05))
	{
		CharacterMovement->StopMovementImmediately();
		return;
	}

	FVector MoveDirection = BarMoveDirection.GetSafeNormal();
	if (FVector::DotProduct(MoveDirection, Character->GetActorRightVector()) < 0.0f) MoveDirection *= -1.0f;
	if (MoveDirection.IsNearlyZero()) { UE_LOG(LogTemp, Warning, TEXT("Shimmy失败：横杆移动方向接近零")); return; }

	if (FVector::DotProduct(MoveDirection, Character->GetActorRightVector()) < 0.0f) MoveDirection *= -1.0f;
	
	const float DirectionSign = Direction > 0.0 ? 1.0f : -1.0f;
	const FVector SideDirection = MoveDirection * DirectionSign;
	const FVector TraceStart =
		Character->GetActorLocation() +
		SideDirection * 40.0f +
		FVector(
			0.0f,
			0.0f,
			bOnBar
				? 145.0f
				: TopImpactPoint.Z - Character->GetActorLocation().Z);
	const FVector TraceEnd = TraceStart + Character->GetActorForwardVector() * 40.0f;

	TArray<AActor*> ActorsToIgnore;
	FHitResult ShimmyHit;

	const bool bShimmyHit = UKismetSystemLibrary::LineTraceSingle(this, TraceStart, TraceEnd, UEngineTypes::ConvertToTraceType(ECC_Visibility), false, ActorsToIgnore, EDrawDebugTrace::ForDuration, ShimmyHit, true, FLinearColor::Red, FLinearColor::Green, 5.0f);

	if (!bShimmyHit)
	{
		CharacterMovement->StopMovementImmediately();
		return;
	}

	Character->AddMovementInput(MoveDirection, Direction);
}

void UTest2Component::LedgeJump()
{
	if (!HasValidCharacter() || !IsValid(MotionWarpingComponent) || !IsValid(LedgeClimbMontage)) return;

	bCanMove = false;
	CharacterMovement->StopMovementImmediately();
	CharacterMovement->SetPlaneConstraintEnabled(false);
	
	ClimbTargetLocation = TopImpactPoint - WallNormal2 * (CapsuleComponent->GetScaledCapsuleRadius() + 20.0) + FVector(0.0, 0.0, 10.0);

	ClimbTargetRotation = FRotator(0.0, (-WallNormal2).Rotation().Yaw, 0.0);

	CharacterMovement->SetMovementMode(MOVE_Flying);

	MotionWarpingComponent->AddOrUpdateWarpTargetFromLocationAndRotation(FName(TEXT("ClimbEnd")), ClimbTargetLocation, ClimbTargetRotation);

	UAnimInstance* AnimInstance = MeshComponent->GetAnimInstance();

	if (!IsValid(AnimInstance)) return;

	AnimInstance->Montage_Play(LedgeClimbMontage, 1.0f, EMontagePlayReturnType::MontageLength, 0.0f, true);

	FOnMontageEnded MontageEndedDelegate;
	MontageEndedDelegate.BindUObject(this, &ThisClass::OnLedgeClimbMontageEnded);

	AnimInstance->Montage_SetEndDelegate(MontageEndedDelegate,LedgeClimbMontage);
}

void UTest2Component::OnLedgeClimbMontageEnded(UAnimMontage* Montage, bool bInterrupted) 
{
	if (Montage != LedgeClimbMontage || bInterrupted) return;

	if (IsValid(MotionWarpingComponent)) MotionWarpingComponent->RemoveWarpTarget(FName(TEXT("ClimbEnd")));

	Detach();
}

void UTest2Component::BarJump()
{
	if (!HasValidCharacter()) return;

	Character->LaunchCharacter(Character->GetActorForwardVector() * -300.0, false, false);

	if (UWorld* World = GetWorld())
	{
		if (!World->GetTimerManager().IsTimerActive(BarJumpTimerHandle))
		{
			World->GetTimerManager().SetTimer(BarJumpTimerHandle, this, &ThisClass::FinishBarJump, 0.2f, false);
		}
	}
	
}

void UTest2Component::FinishBarJump()
{
	if (!HasValidCharacter()) return;

	Detach();

	Character->LaunchCharacter(FVector(0.0, 0.0, 1000.0), false, false);
}
