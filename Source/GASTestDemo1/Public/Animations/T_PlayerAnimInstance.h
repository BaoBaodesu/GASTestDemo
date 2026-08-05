#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"

#include "Player/Components/T_GrabComponent.h"

#include "T_PlayerAnimInstance.generated.h"

class ACharacter;
class UCharacterMovementComponent;
class UT_AimingComponent;

UCLASS()
class GASTESTDEMO1_API UT_PlayerAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	virtual void NativeThreadSafeUpdateAnimation(float DeltaSeconds) override;

private:
	void CacheReferences();
	void UpdateProjectSpecificData();
	void ResetAnimationData();

private:
	UPROPERTY(Transient, BlueprintReadOnly, Category="Animation|References", meta=(AllowPrivateAccess="true"))
	TObjectPtr<ACharacter> Character;

	UPROPERTY(Transient, BlueprintReadOnly, Category="Animation|References", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UCharacterMovementComponent> MovementComponent;

	UPROPERTY(Transient)
	TObjectPtr<UT_GrabComponent> GrabComponent;

	UPROPERTY(Transient)
	TObjectPtr<UT_AimingComponent> AimingComponent;

	UPROPERTY(BlueprintReadOnly, Category="Animation|Movement", meta=(AllowPrivateAccess="true"))
	FVector Velocity{FVector::ZeroVector};

	UPROPERTY(BlueprintReadOnly, Category="Animation|Movement", meta=(AllowPrivateAccess="true"))
	float GroundSpeed{0.f};

	UPROPERTY(BlueprintReadOnly, Category="Animation|Movement", meta=(AllowPrivateAccess="true"))
	float Direction{0.f};

	UPROPERTY(BlueprintReadOnly, Category="Animation|Movement", meta=(AllowPrivateAccess="true"))
	bool ShouldMove{false};

	UPROPERTY(BlueprintReadOnly, Category="Animation|Movement", meta=(AllowPrivateAccess="true"))
	bool bIsFalling{false};

	UPROPERTY(BlueprintReadOnly, Category="Animation|Movement", meta=(AllowPrivateAccess="true"))
	bool bIsCrouching{false};

	UPROPERTY(BlueprintReadOnly, Category="Animation|Movement", meta=(AllowPrivateAccess="true"))
	FRotator Rotation{FRotator::ZeroRotator};

	UPROPERTY(BlueprintReadOnly, Category="Animation|Traversal", meta=(AllowPrivateAccess="true"))
	bool bGrabbed{false};

	UPROPERTY(BlueprintReadOnly, Category="Animation|Traversal", meta=(AllowPrivateAccess="true"))
	ET_GrabType GrabType{ET_GrabType::None};

	UPROPERTY(BlueprintReadOnly, Category="Animation|Traversal", meta=(AllowPrivateAccess="true"))
	float SignedBarSpeed{0.f};

	UPROPERTY(BlueprintReadOnly, Category="Animation|Weapon", meta=(AllowPrivateAccess="true"))
	bool bHasPistolGun{false};

	UPROPERTY(BlueprintReadOnly, Category="Animation|Weapon", meta=(AllowPrivateAccess="true"))
	bool bAiming{false};

	UPROPERTY(BlueprintReadOnly, Category="Animation|Aim", meta=(AllowPrivateAccess="true"))
	float Pitch{0.f};

	UPROPERTY(BlueprintReadOnly, Category="Animation|Aim", meta=(AllowPrivateAccess="true"))
	float Yaw{0.f};

	UPROPERTY(EditDefaultsOnly, Category="Animation|Movement", meta=(ClampMin="0.0"))
	float MovementThreshold{0.1f};

	FVector CurrentAcceleration{FVector::ZeroVector};
	FRotator BaseAimRotation{FRotator::ZeroRotator};
	bool bOrientRotationToMovement{true};
};
