#include "Player/Components/T_GrabComponent.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Kismet/KismetSystemLibrary.h"
#include "MotionWarpingComponent.h"
#include "UObject/ConstructorHelpers.h"

UT_GrabComponent::UT_GrabComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	static ConstructorHelpers::FObjectFinder<UAnimMontage> LedgeClimbMontageObject(TEXT("/Game/GASTestDemo/Characters/PlayerCharacters/Animations/Test/Traversal/Climb/anim_LedgeClimb_ClimbUp_Montage.anim_LedgeClimb_ClimbUp_Montage"));
	if (LedgeClimbMontageObject.Succeeded()) LedgeClimbMontage = LedgeClimbMontageObject.Object;
}

void UT_GrabComponent::BeginPlay()
{
	Super::BeginPlay();

	Character = Cast<ACharacter>(GetOwner());
	if (!IsValid(Character)) { UE_LOG(LogTemp, Error, TEXT("T_GrabComponent只能添加到Character上")); return; }

	CharacterMovement = Character->GetCharacterMovement();
	CapsuleComponent = Character->GetCapsuleComponent();
	MeshComponent = Character->GetMesh();
	MotionWarpingComponent = Character->FindComponentByClass<UMotionWarpingComponent>();
	if (!IsValid(CharacterMovement) || !IsValid(CapsuleComponent) || !IsValid(MeshComponent)) UE_LOG(LogTemp, Error, TEXT("T_GrabComponent获取角色组件失败"));
}

void UT_GrabComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(GrabCheckTimerHandle);
		World->GetTimerManager().ClearTimer(TransitionTimerHandle);
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

bool UT_GrabComponent::HasValidCharacter() const
{
	return IsValid(Character) && IsValid(CharacterMovement) && IsValid(CapsuleComponent) && IsValid(MeshComponent);
}

void UT_GrabComponent::StartGrabCheck()
{
	if (!HasValidCharacter() || !GetWorld() || bGrabbed) return;
	GetWorld()->GetTimerManager().SetTimer(GrabCheckTimerHandle, this, &ThisClass::TryGrab, GrabCheckInterval, true);
}

void UT_GrabComponent::StopGrabCheck()
{
	if (!GetWorld()) return;
	GetWorld()->GetTimerManager().ClearTimer(GrabCheckTimerHandle);
}

void UT_GrabComponent::TryGrab()
{
	if (!HasValidCharacter() || bGrabbed || CharacterMovement->Velocity.Z >= 0.0) return;

	TArray<AActor*> ActorsToIgnore;
	FHitResult FirstHit;
	const FVector FirstTraceBase = Character->GetActorLocation() + Character->GetActorForwardVector() * 45.0;
	const bool bFirstHit = UKismetSystemLibrary::BoxTraceSingle(this, FirstTraceBase + FVector(0.0, 0.0, 100.0), FirstTraceBase + FVector(0.0, 0.0, 50.0), FVector(25.0, 5.0, 5.0), Character->GetActorRotation(), ETraceTypeQuery::TraceTypeQuery1, false, ActorsToIgnore, bDrawDebug ? EDrawDebugTrace::ForDuration : EDrawDebugTrace::None, FirstHit, true, FLinearColor::Red, FLinearColor::Green, 5.0f);
	if (!bFirstHit || !IsValid(FirstHit.GetActor())) return;
	if (FirstHit.bStartPenetrating && !FirstHit.GetActor()->ActorHasTag(FName(TEXT("Bar")))) return;

	if (FirstHit.GetActor()->ActorHasTag(FName(TEXT("Bar"))))
	{
		GrabType = ET_GrabType::Bar;
		GrabOffset = 0.0;
	}
	else
	{
		GrabType = ET_GrabType::Wall;
		GrabOffset = 120.0;
	}

	WallNormal = FVector(FirstHit.ImpactNormal.X, FirstHit.ImpactNormal.Y, 0.0).GetSafeNormal(0.0001);

	FHitResult TopHit;
	const FVector SecondTraceOffsetLocation = Character->GetActorLocation() + Character->GetActorForwardVector() * 5.0;
	const FVector SecondTraceStart = FVector(SecondTraceOffsetLocation.X, SecondTraceOffsetLocation.Y, FirstHit.ImpactPoint.Z);
	const FVector SecondTraceEnd = FirstHit.Location + Character->GetActorForwardVector() * 5.0;
	const bool bTopHit = UKismetSystemLibrary::BoxTraceSingle(this, SecondTraceStart, SecondTraceEnd, FVector(5.0), FRotator::ZeroRotator, ETraceTypeQuery::TraceTypeQuery1, false, ActorsToIgnore, bDrawDebug ? EDrawDebugTrace::ForDuration : EDrawDebugTrace::None, TopHit, true, FLinearColor::Red, FLinearColor::Green, 5.0f);
	if (!bTopHit) return;

	TopTraceLocation = TopHit.Location;
	TopImpactPoint = TopHit.ImpactPoint;
	LowerCheckPoint = FVector(TopHit.ImpactPoint.X, TopHit.ImpactPoint.Y, TopHit.ImpactPoint.Z - GrabOffset);

	FHitResult LowerHit;
	const bool bLowerHit = UKismetSystemLibrary::BoxTraceSingle(this, LowerCheckPoint - Character->GetActorForwardVector() * 20.0, LowerCheckPoint + Character->GetActorForwardVector() * 60.0, FVector(5.0, 10.0, 10.0), FRotator::ZeroRotator, ETraceTypeQuery::TraceTypeQuery1, false, ActorsToIgnore, bDrawDebug ? EDrawDebugTrace::ForDuration : EDrawDebugTrace::None, LowerHit, true, FLinearColor::Red, FLinearColor::Green, 5.0f);
	if (!bLowerHit) return;

	WallNormal = LowerHit.ImpactNormal.GetSafeNormal2D();
	GrabMoveDirection = FVector::CrossProduct(LowerHit.Normal, FVector::UpVector).GetSafeNormal();
	BoundsCheck();
}

void UT_GrabComponent::BoundsCheck()
{
	if (!HasValidCharacter()) return;

	TArray<AActor*> ActorsToIgnore;
	FHitResult RightHit;
	const FVector RightTraceStart = Character->GetActorLocation() + Character->GetActorRightVector() * 30.0 + FVector(0.0, 0.0, 75.0);
	const FVector RightTraceEnd = RightTraceStart + Character->GetActorForwardVector() * 50.0;
	const bool bRightHit = UKismetSystemLibrary::BoxTraceSingle(this, RightTraceStart, RightTraceEnd, FVector(5.0, 5.0, 25.0), Character->GetActorRotation(), ETraceTypeQuery::TraceTypeQuery1, false, ActorsToIgnore, bDrawDebug ? EDrawDebugTrace::ForDuration : EDrawDebugTrace::None, RightHit, true, FLinearColor::Red, FLinearColor::Green, 5.0f);
	if (!bRightHit) return;

	FHitResult LeftHit;
	const FVector LeftTraceStart = Character->GetActorLocation() + Character->GetActorRightVector() * -30.0 + FVector(0.0, 0.0, 75.0);
	const FVector LeftTraceEnd = LeftTraceStart + Character->GetActorForwardVector() * 50.0;
	const bool bLeftHit = UKismetSystemLibrary::BoxTraceSingle(this, LeftTraceStart, LeftTraceEnd, FVector(5.0, 5.0, 25.0), Character->GetActorRotation(), ETraceTypeQuery::TraceTypeQuery1, false, ActorsToIgnore, bDrawDebug ? EDrawDebugTrace::ForDuration : EDrawDebugTrace::None, LeftHit, true, FLinearColor::Red, FLinearColor::Green, 5.0f);
	if (bLeftHit) BeginTransition();
}

void UT_GrabComponent::BeginTransition()
{
	if (!HasValidCharacter() || !GetWorld()) return;

	if (GrabType == ET_GrabType::Bar)
	{
		GrabHeight = TopImpactPoint.Z - 146.0;
		DistanceToGrab = CapsuleComponent->GetScaledCapsuleRadius() - 42.0f;
	}
	else
	{
		GrabHeight = TopImpactPoint.Z - CapsuleComponent->GetUnscaledCapsuleHalfHeight() + 35.0;
		DistanceToGrab = 25.0;
	}

	CacheMovementSettings();
	bCanMove = false;
	bGrabbed = true;
	GrabState = ET_GrabState::Transitioning;
	CharacterMovement->StopMovementImmediately();
	CharacterMovement->GravityScale = 0.0f;
	CharacterMovement->MaxFlySpeed = 70.0f;
	CharacterMovement->bOrientRotationToMovement = false;
	CharacterMovement->bUseControllerDesiredRotation = false;
	CharacterMovement->SetPlaneConstraintNormal(FVector::UpVector);
	CharacterMovement->SetPlaneConstraintOrigin(FVector(0.0, 0.0, GrabHeight));
	CharacterMovement->SetPlaneConstraintEnabled(true);
	CharacterMovement->SetMovementMode(MOVE_Flying);
	StopGrabCheck();
	GetWorld()->GetTimerManager().SetTimer(TransitionTimerHandle, this, &ThisClass::UpdateTransition, 0.01f, true);
}

void UT_GrabComponent::UpdateTransition()
{
	if (!HasValidCharacter() || !GetWorld()) return;

	const FVector DistanceOffset = Character->GetActorForwardVector() * DistanceToGrab;
	const FVector TargetLocation = FVector(TopTraceLocation.X - DistanceOffset.X, TopTraceLocation.Y - DistanceOffset.Y, GrabHeight);
	const FRotator WallRotation = (-WallNormal).Rotation();
	const FRotator TargetRotation = FRotator(0.0, WallRotation.Yaw, 0.0);
	const FVector NewLocation = FMath::VInterpConstantTo(Character->GetActorLocation(), TargetLocation, 0.2f, 75.0f);
	const FRotator NewRotation = FMath::RInterpConstantTo(Character->GetActorRotation(), TargetRotation, 0.2f, 75.0f);
	const bool bReachedLocation = NewLocation.Equals(TargetLocation, 1.0f);
	const bool bReachedRotation = NewRotation.Equals(TargetRotation, 1.0f);

	Character->SetActorLocationAndRotation(NewLocation, NewRotation, false, nullptr, ETeleportType::None);
	if (bReachedLocation && bReachedRotation) FinishTransition(TargetLocation, TargetRotation);
}

void UT_GrabComponent::FinishTransition(const FVector& TargetLocation, const FRotator& TargetRotation)
{
	if (!HasValidCharacter() || !GetWorld()) return;

	Character->SetActorLocationAndRotation(TargetLocation, TargetRotation, false, nullptr, ETeleportType::None);
	bCanMove = true;
	GrabState = ET_GrabState::Hanging;
	GetWorld()->GetTimerManager().ClearTimer(TransitionTimerHandle);
	if (!bGrabStartedBroadcast)
	{
		bGrabStartedBroadcast = true;
		OnGrabStarted.Broadcast(GrabType);
	}
}

void UT_GrabComponent::Detach()
{
	DetachInternal();
}

void UT_GrabComponent::DetachInternal()
{
	if (!HasValidCharacter() || bDetachInProgress) return;

	bDetachInProgress = true;
	StopGrabCheck();
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(TransitionTimerHandle);
		World->GetTimerManager().ClearTimer(BarJumpTimerHandle);
	}
	if (IsValid(MotionWarpingComponent)) MotionWarpingComponent->RemoveWarpTarget(FName(TEXT("ClimbEnd")));

	const bool bShouldBroadcastGrabEnded = bGrabbed;
	RestoreMovementSettings();
	ClearGrabData();
	bDetachInProgress = false;
	if (bShouldBroadcastGrabEnded) OnGrabEnded.Broadcast();
}

void UT_GrabComponent::Shimmy(float Direction)
{
	ShimmyDirection = Direction;
	if (!HasValidCharacter() || !bGrabbed || !bCanMove) return;

	CharacterMovement->bOrientRotationToMovement = false;
	CharacterMovement->bUseControllerDesiredRotation = false;
	if (FMath::IsNearlyZero(Direction, 0.05f))
	{
		ShimmyDirection = 0.0f;
		CharacterMovement->StopMovementImmediately();
		return;
	}

	FVector MoveDirection = GrabMoveDirection.GetSafeNormal();
	if (FVector::DotProduct(MoveDirection, Character->GetActorRightVector()) < 0.0f) MoveDirection *= -1.0f;
	if (MoveDirection.IsNearlyZero()) { ShimmyDirection = 0.0f; UE_LOG(LogTemp, Warning, TEXT("Shimmy失败：抓握移动方向接近零")); return; }

	const float DirectionSign = Direction > 0.0f ? 1.0f : -1.0f;
	const FVector SideDirection = MoveDirection * DirectionSign;
	const FVector TraceStart = Character->GetActorLocation() + SideDirection * 40.0f + FVector(0.0f, 0.0f, IsOnBar() ? 145.0f : GrabHeight - Character->GetActorLocation().Z);
	const FVector TraceEnd = TraceStart + Character->GetActorForwardVector() * 40.0f;

	TArray<AActor*> ActorsToIgnore;
	FHitResult ShimmyHit;
	const bool bShimmyHit = UKismetSystemLibrary::LineTraceSingle(this, TraceStart, TraceEnd, UEngineTypes::ConvertToTraceType(ECC_Visibility), false, ActorsToIgnore, bDrawDebug ? EDrawDebugTrace::ForDuration : EDrawDebugTrace::None, ShimmyHit, true, FLinearColor::Red, FLinearColor::Green, 5.0f);
	if (!bShimmyHit)
	{
		ShimmyDirection = 0.0f;
		CharacterMovement->StopMovementImmediately();
		return;
	}

	Character->AddMovementInput(MoveDirection, Direction);
}

void UT_GrabComponent::LedgeJump()
{
	if (!HasValidCharacter() || !bGrabbed || !bCanMove || GrabState != ET_GrabState::Hanging || IsOnBar() || !IsValid(MotionWarpingComponent) || !IsValid(LedgeClimbMontage)) return;

	bCanMove = false;
	ShimmyDirection = 0.0f;
	GrabState = ET_GrabState::Climbing;
	CharacterMovement->StopMovementImmediately();
	CharacterMovement->SetPlaneConstraintEnabled(false);
	ClimbTargetLocation = TopImpactPoint - WallNormal * (CapsuleComponent->GetScaledCapsuleRadius() + 20.0) + FVector(0.0, 0.0, 10.0);
	ClimbTargetRotation = FRotator(0.0, (-WallNormal).Rotation().Yaw, 0.0);
	CharacterMovement->SetMovementMode(MOVE_Flying);
	MotionWarpingComponent->AddOrUpdateWarpTargetFromLocationAndRotation(FName(TEXT("ClimbEnd")), ClimbTargetLocation, ClimbTargetRotation);

	UAnimInstance* AnimInstance = MeshComponent->GetAnimInstance();
	if (!IsValid(AnimInstance)) { DetachInternal(); return; }

	if (AnimInstance->Montage_Play(LedgeClimbMontage, 1.0f, EMontagePlayReturnType::MontageLength, 0.0f, true) <= 0.0f) { DetachInternal(); return; }
	FOnMontageEnded MontageEndedDelegate;
	MontageEndedDelegate.BindUObject(this, &ThisClass::OnLedgeClimbMontageEnded);
	AnimInstance->Montage_SetEndDelegate(MontageEndedDelegate, LedgeClimbMontage);
}

void UT_GrabComponent::OnLedgeClimbMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (Montage != LedgeClimbMontage) return;
	DetachInternal();
}

void UT_GrabComponent::BarJump()
{
	if (!HasValidCharacter() || !bGrabbed || !bCanMove || GrabState != ET_GrabState::Hanging || !IsOnBar()) return;

	bCanMove = false;
	ShimmyDirection = 0.0f;
	GrabState = ET_GrabState::BarJumping;
	CharacterMovement->StopMovementImmediately();
	Character->LaunchCharacter(Character->GetActorForwardVector() * -300.0, false, false);
	if (UWorld* World = GetWorld())
	{
		if (!World->GetTimerManager().IsTimerActive(BarJumpTimerHandle)) World->GetTimerManager().SetTimer(BarJumpTimerHandle, this, &ThisClass::FinishBarJump, 0.2f, false);
	}
}

void UT_GrabComponent::FinishBarJump()
{
	if (!HasValidCharacter()) return;
	DetachInternal();
	Character->LaunchCharacter(FVector(0.0, 0.0, 1000.0), false, false);
}

void UT_GrabComponent::CacheMovementSettings()
{
	if (!HasValidCharacter() || bMovementSettingsCached) return;

	CachedGravityScale = CharacterMovement->GravityScale;
	CachedMaxWalkSpeed = CharacterMovement->MaxWalkSpeed;
	CachedMaxFlySpeed = CharacterMovement->MaxFlySpeed;
	bCachedOrientRotationToMovement = CharacterMovement->bOrientRotationToMovement;
	bCachedUseControllerDesiredRotation = CharacterMovement->bUseControllerDesiredRotation;
	bCachedPlaneConstraintEnabled = CharacterMovement->bConstrainToPlane;
	CachedPlaneConstraintNormal = CharacterMovement->GetPlaneConstraintNormal();
	CachedPlaneConstraintOrigin = CharacterMovement->GetPlaneConstraintOrigin();
	CachedMovementMode = CharacterMovement->MovementMode;
	CachedCustomMovementMode = CharacterMovement->CustomMovementMode;
	bMovementSettingsCached = true;
}

void UT_GrabComponent::RestoreMovementSettings()
{
	if (!HasValidCharacter()) return;

	CharacterMovement->StopMovementImmediately();
	if (!bMovementSettingsCached)
	{
		CharacterMovement->GravityScale = 1.5f;
		CharacterMovement->MaxWalkSpeed = 600.0f;
		CharacterMovement->MaxFlySpeed = 600.0f;
		CharacterMovement->SetPlaneConstraintEnabled(false);
		CharacterMovement->bOrientRotationToMovement = true;
		CharacterMovement->bUseControllerDesiredRotation = true;
		CharacterMovement->SetMovementMode(MOVE_Walking);
		return;
	}

	CharacterMovement->GravityScale = CachedGravityScale;
	CharacterMovement->MaxWalkSpeed = CachedMaxWalkSpeed;
	CharacterMovement->MaxFlySpeed = CachedMaxFlySpeed;
	CharacterMovement->bOrientRotationToMovement = bCachedOrientRotationToMovement;
	CharacterMovement->bUseControllerDesiredRotation = bCachedUseControllerDesiredRotation;
	CharacterMovement->SetPlaneConstraintNormal(CachedPlaneConstraintNormal);
	CharacterMovement->SetPlaneConstraintOrigin(CachedPlaneConstraintOrigin);
	CharacterMovement->SetPlaneConstraintEnabled(bCachedPlaneConstraintEnabled);
	CharacterMovement->SetMovementMode(CachedMovementMode, CachedCustomMovementMode);
	bMovementSettingsCached = false;
}

void UT_GrabComponent::ClearGrabData()
{
	bGrabbed = false;
	bCanMove = true;
	GrabType = ET_GrabType::None;
	GrabState = ET_GrabState::None;
	ShimmyDirection = 0.0f;
	GrabHeight = 0.0;
	DistanceToGrab = 0.0;
	GrabOffset = 0.0;
	WallNormal = FVector::ZeroVector;
	LowerCheckPoint = FVector::ZeroVector;
	TopImpactPoint = FVector::ZeroVector;
	GrabMoveDirection = FVector::ZeroVector;
	ClimbTargetLocation = FVector::ZeroVector;
	ClimbTargetRotation = FRotator::ZeroRotator;
	TopTraceLocation = FVector::ZeroVector;
	bGrabStartedBroadcast = false;
}
