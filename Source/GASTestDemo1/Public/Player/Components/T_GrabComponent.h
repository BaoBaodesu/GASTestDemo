#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TimerManager.h"
#include "T_GrabComponent.generated.h"

class ACharacter;
class UAnimMontage;
class UCapsuleComponent;
class UMotionWarpingComponent;
class USkeletalMeshComponent;

UENUM(BlueprintType)
enum class ET_GrabType : uint8
{
	None,
	Wall,
	Bar
};

UENUM(BlueprintType)
enum class ET_GrabState : uint8
{
	None,
	Transitioning,
	Hanging,
	Climbing,
	BarJumping
};

DECLARE_MULTICAST_DELEGATE_OneParam(FOnGrabStarted, ET_GrabType);
DECLARE_MULTICAST_DELEGATE(FOnGrabEnded);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class GASTESTDEMO1_API UT_GrabComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UT_GrabComponent();

	UFUNCTION(BlueprintCallable, Category="Grab")
	void StartGrabCheck();

	UFUNCTION(BlueprintCallable, Category="Grab")
	void StopGrabCheck();

	UFUNCTION(BlueprintCallable, Category="Grab")
	void TryGrab();

	UFUNCTION(BlueprintCallable, Category="Grab")
	void Shimmy(float Direction);

	UFUNCTION(BlueprintCallable, Category="Grab")
	void Detach();

	UFUNCTION(BlueprintCallable, Category="Grab")
	void LedgeJump();

	UFUNCTION(BlueprintCallable, Category="Grab")
	void BarJump();

	UFUNCTION(BlueprintPure, Category="Grab")
	bool IsGrabbed() const { return bGrabbed; }

	UFUNCTION(BlueprintPure, Category="Grab")
	bool CanMove() const { return bCanMove; }

	UFUNCTION(BlueprintPure, Category="Grab")
	bool IsOnBar() const { return GrabType == ET_GrabType::Bar; }

	UFUNCTION(BlueprintPure, Category="Grab")
	ET_GrabType GetGrabType() const { return GrabType; }

	UFUNCTION(BlueprintPure, Category="Grab")
	float GetShimmyDirection() const { return ShimmyDirection; }

	FOnGrabStarted OnGrabStarted;
	FOnGrabEnded OnGrabEnded;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	bool HasValidCharacter() const;
	void BoundsCheck();
	void BeginTransition();
	void UpdateTransition();
	void FinishTransition(const FVector& TargetLocation, const FRotator& TargetRotation);
	void FinishBarJump();
	void OnLedgeClimbMontageEnded(UAnimMontage* Montage, bool bInterrupted);
	void CacheMovementSettings();
	void RestoreMovementSettings();
	void ClearGrabData();
	void DetachInternal();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Grab", meta=(ClampMin="0.001", AllowPrivateAccess="true"))
	float GrabCheckInterval{0.01f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Grab|Debug", meta=(AllowPrivateAccess="true"))
	bool bDrawDebug{false};

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Grab", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UAnimMontage> LedgeClimbMontage;

	UPROPERTY(BlueprintReadOnly, Category="Grab|State", meta=(AllowPrivateAccess="true"))
	bool bGrabbed{false};

	UPROPERTY(BlueprintReadOnly, Category="Grab|State", meta=(AllowPrivateAccess="true"))
	bool bCanMove{true};

	UPROPERTY(BlueprintReadOnly, Category="Grab|State", meta=(AllowPrivateAccess="true"))
	ET_GrabType GrabType{ET_GrabType::None};

	UPROPERTY(BlueprintReadOnly, Category="Grab|State", meta=(AllowPrivateAccess="true"))
	ET_GrabState GrabState{ET_GrabState::None};

	UPROPERTY(BlueprintReadOnly, Category="Grab|State", meta=(AllowPrivateAccess="true"))
	float ShimmyDirection{0.0f};

	UPROPERTY(BlueprintReadOnly, Category="Grab|State", meta=(AllowPrivateAccess="true"))
	double GrabHeight{0.0};

	UPROPERTY(BlueprintReadOnly, Category="Grab|State", meta=(AllowPrivateAccess="true"))
	double DistanceToGrab{0.0};

	UPROPERTY(BlueprintReadOnly, Category="Grab|State", meta=(AllowPrivateAccess="true"))
	double GrabOffset{0.0};

	UPROPERTY(BlueprintReadOnly, Category="Grab|State", meta=(AllowPrivateAccess="true"))
	FVector WallNormal{FVector::ZeroVector};

	UPROPERTY(BlueprintReadOnly, Category="Grab|State", meta=(AllowPrivateAccess="true"))
	FVector LowerCheckPoint{FVector::ZeroVector};

	UPROPERTY(BlueprintReadOnly, Category="Grab|State", meta=(AllowPrivateAccess="true"))
	FVector TopImpactPoint{FVector::ZeroVector};

	UPROPERTY(BlueprintReadOnly, Category="Grab|State", meta=(AllowPrivateAccess="true"))
	FVector GrabMoveDirection{FVector::ZeroVector};

	UPROPERTY(BlueprintReadOnly, Category="Grab|State", meta=(AllowPrivateAccess="true"))
	FVector ClimbTargetLocation{FVector::ZeroVector};

	UPROPERTY(BlueprintReadOnly, Category="Grab|State", meta=(AllowPrivateAccess="true"))
	FRotator ClimbTargetRotation{FRotator::ZeroRotator};

	UPROPERTY(BlueprintReadOnly, Category="Grab|State", meta=(AllowPrivateAccess="true"))
	FVector TopTraceLocation{FVector::ZeroVector};

	UPROPERTY(Transient)
	TObjectPtr<ACharacter> Character;

	UPROPERTY(Transient)
	TObjectPtr<UCharacterMovementComponent> CharacterMovement;

	UPROPERTY(Transient)
	TObjectPtr<UCapsuleComponent> CapsuleComponent;

	UPROPERTY(Transient)
	TObjectPtr<USkeletalMeshComponent> MeshComponent;

	UPROPERTY(Transient)
	TObjectPtr<UMotionWarpingComponent> MotionWarpingComponent;

	FTimerHandle GrabCheckTimerHandle;
	FTimerHandle TransitionTimerHandle;
	FTimerHandle BarJumpTimerHandle;

	float CachedGravityScale{1.0f};
	float CachedMaxWalkSpeed{0.0f};
	float CachedMaxFlySpeed{0.0f};
	bool bCachedOrientRotationToMovement{false};
	bool bCachedUseControllerDesiredRotation{false};
	bool bCachedPlaneConstraintEnabled{false};
	FVector CachedPlaneConstraintNormal{FVector::ZeroVector};
	FVector CachedPlaneConstraintOrigin{FVector::ZeroVector};
	TEnumAsByte<EMovementMode> CachedMovementMode{MOVE_None};
	uint8 CachedCustomMovementMode{0};
	bool bMovementSettingsCached{false};
	bool bGrabStartedBroadcast{false};
	bool bDetachInProgress{false};
};
