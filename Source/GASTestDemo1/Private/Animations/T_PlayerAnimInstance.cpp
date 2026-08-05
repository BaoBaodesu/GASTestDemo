#include "Animations/T_PlayerAnimInstance.h"

#include "Characters/T_PlayerCharacter.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "KismetAnimationLibrary.h"
#include "Player/Components/T_AimingComponent.h"

void UT_PlayerAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	CacheReferences();
}

void UT_PlayerAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (Character.Get() != TryGetPawnOwner()) CacheReferences();
	if (!IsValid(Character) || !IsValid(MovementComponent)) { ResetAnimationData(); return; }

	Velocity = MovementComponent->Velocity;
	CurrentAcceleration = MovementComponent->GetCurrentAcceleration();
	Rotation = Character->GetActorRotation();
	BaseAimRotation = Character->GetBaseAimRotation();
	bOrientRotationToMovement = MovementComponent->bOrientRotationToMovement;
	bIsFalling = MovementComponent->IsFalling();
	bIsCrouching = MovementComponent->IsCrouching();

	bGrabbed = IsValid(GrabComponent) && GrabComponent->IsGrabbed();
	GrabType = IsValid(GrabComponent) ? GrabComponent->GetGrabType() : ET_GrabType::None;
	SignedBarSpeed = IsValid(GrabComponent) && bGrabbed && GrabComponent->CanMove() ? GrabComponent->GetShimmyDirection() : 0.f;

	bAiming = IsValid(AimingComponent) && AimingComponent->IsAiming();

	UpdateProjectSpecificData();
}

void UT_PlayerAnimInstance::NativeThreadSafeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeThreadSafeUpdateAnimation(DeltaSeconds);

	GroundSpeed = Velocity.Size2D();
	ShouldMove = GroundSpeed > MovementThreshold && !CurrentAcceleration.IsNearlyZero();

	const float CalculatedDirection = UKismetAnimationLibrary::CalculateDirection(Velocity, Rotation);
	Direction = bOrientRotationToMovement ? FMath::Clamp(CalculatedDirection, -45.f, 45.f) : CalculatedDirection;

	const FRotator AimDelta = (BaseAimRotation - Rotation).GetNormalized();
	Pitch = AimDelta.Pitch;
	Yaw = AimDelta.Yaw;
}

void UT_PlayerAnimInstance::CacheReferences()
{
	Character = Cast<ACharacter>(TryGetPawnOwner());
	MovementComponent = IsValid(Character) ? Character->GetCharacterMovement() : nullptr;
	GrabComponent = IsValid(Character) ? Character->FindComponentByClass<UT_GrabComponent>() : nullptr;
	AimingComponent = IsValid(Character) ? Character->FindComponentByClass<UT_AimingComponent>() : nullptr;
}

void UT_PlayerAnimInstance::UpdateProjectSpecificData()
{
	const AT_PlayerCharacter* PlayerCharacter = Cast<AT_PlayerCharacter>(Character);
	bHasPistolGun = IsValid(PlayerCharacter) && PlayerCharacter->HasPistolGun();
}

void UT_PlayerAnimInstance::ResetAnimationData()
{
	Velocity = FVector::ZeroVector;
	CurrentAcceleration = FVector::ZeroVector;
	Rotation = FRotator::ZeroRotator;
	BaseAimRotation = FRotator::ZeroRotator;
	GroundSpeed = 0.f;
	Direction = 0.f;
	Pitch = 0.f;
	Yaw = 0.f;
	SignedBarSpeed = 0.f;
	ShouldMove = false;
	bIsFalling = false;
	bIsCrouching = false;
	bGrabbed = false;
	bHasPistolGun = false;
	bAiming = false;
	bOrientRotationToMovement = true;
	GrabType = ET_GrabType::None;
}
