#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/EngineTypes.h"
#include "T_AimingComponent.generated.h"

class ACharacter;
class UAnimInstance;
class UCameraComponent;
class UCharacterMovementComponent;
class USpringArmComponent;
class UUserWidget;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class GASTESTDEMO1_API UT_AimingComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UT_AimingComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, Category="Aiming")
	void StartAiming();

	UFUNCTION(BlueprintCallable, Category="Aiming")
	void StopAiming();

	void SetUnaimedMaxWalkSpeed(float MaxWalkSpeed);

	UFUNCTION(BlueprintPure, Category="Aiming")
	bool IsAiming() const { return bAiming; }

	UFUNCTION(BlueprintPure, Category="Aiming|Input")
	float GetLookInputMultiplier() const { return bAiming ? AimingMouseSpeedMultiplier : 1.f; }

	UFUNCTION(BlueprintCallable, Category="Aiming")
	bool GetCameraAimPoint(FVector& OutAimPoint, FHitResult& OutCameraHit) const;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void UpdateAnimationState();
	void RestoreMovementSettings();
	void ShowCrosshair();
	void HideCrosshair();

	UPROPERTY(EditAnywhere, Category="Aiming|UI")
	TSubclassOf<UUserWidget> CrosshairWidgetClass;

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> CrosshairWidget;

	UPROPERTY(EditAnywhere, Category="Aiming|Camera", meta=(ClampMin="0.0"))
	float AimingArmLength{120.f};

	UPROPERTY(EditAnywhere, Category="Aiming|Camera")
	FVector AimingSocketOffset{0.f, 40.f, -25.f};

	UPROPERTY(EditAnywhere, Category="Aiming|Camera", meta=(ClampMin="5.0", ClampMax="170.0"))
	float AimingFOV{40.f};

	UPROPERTY(EditAnywhere, Category="Aiming|Camera", meta=(ClampMin="0.01"))
	float CameraInterpSpeed{10.f};

	UPROPERTY(EditAnywhere, Category="Aiming|Rotation")
	bool bRotateCharacterWhileAiming{true};

	UPROPERTY(EditAnywhere, Category="Aiming|Rotation")
	float AimingYawOffset{0.f};

	UPROPERTY(EditAnywhere, Category="Aiming|Rotation", meta=(ClampMin="0.01"))
	float CharacterRotationInterpSpeed{12.f};

	UPROPERTY(EditAnywhere, Category="Aiming|Movement", meta=(ClampMin="0.0"))
	float AimingMaxWalkSpeed{150.f};

	UPROPERTY(EditAnywhere, Category="Aiming|Input", meta=(ClampMin="0.0"))
	float AimingMouseSpeedMultiplier{0.5f};

	UPROPERTY(EditAnywhere, Category="Aiming|Trace", meta=(ClampMin="1.0"))
	float TraceDistance{10000.f};

	UPROPERTY(EditAnywhere, Category="Aiming|Trace", meta=(ClampMin="0.0"))
	float CameraTraceStartOffset{10.f};

	UPROPERTY(EditAnywhere, Category="Aiming|Trace")
	TEnumAsByte<ECollisionChannel> TraceChannel{ECC_Visibility};

	UPROPERTY(EditAnywhere, Category="Aiming|Debug")
	bool bDrawDebug{false};

	UPROPERTY(EditAnywhere, Category="Aiming|Debug", meta=(ClampMin="0.0"))
	float DebugDrawTime{2.f};

	UPROPERTY(Transient)
	TObjectPtr<ACharacter> Character;

	UPROPERTY(Transient)
	TObjectPtr<UCharacterMovementComponent> MovementComponent;

	UPROPERTY(Transient)
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(Transient)
	TObjectPtr<UCameraComponent> FollowCamera;

	UPROPERTY(Transient)
	TObjectPtr<UAnimInstance> AnimInstance;

	float NormalArmLength{0.f};
	float NormalFOV{90.f};
	FVector NormalSocketOffset{FVector::ZeroVector};
	float CachedMaxWalkSpeed{0.f};
	bool bAiming{false};
	bool bHasCachedMovementSettings{false};
	bool bCachedOrientRotationToMovement{true};
	bool bCachedUseControllerDesiredRotation{false};
	bool bCachedUseControllerRotationYaw{false};
};
