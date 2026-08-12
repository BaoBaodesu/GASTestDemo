#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/T_GameplayAbility.h"
#include "TimerManager.h"
#include "T_Throw.generated.h"

class AT_ThrowTrajectoryPreview;
class AT_Throwable;
class UAbilityTask_PlayMontageAndWait;
class UAbilityTask_WaitDelay;
class UAbilityTask_WaitGameplayEvent;
class UAbilityTask_WaitInputRelease;
class UAnimMontage;
class UT_AimingComponent;
class UT_InventoryComponent;

UCLASS()
class GASTESTDEMO1_API UT_Throw : public UT_GameplayAbility
{
	GENERATED_BODY()

public:
	UT_Throw();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

private:
	UT_InventoryComponent* GetOwnerInventory() const;
	TSubclassOf<AT_Throwable> ResolveThrowableClass() const;
	bool CalculateThrowParameters(FVector& OutSpawnLocation, FVector& OutThrowDirection, bool bLogMissingSocket = false) const;
	void StartThrowPreview();
	void UpdateThrowPreview();
	void StopThrowPreview();
	void ExecuteThrow();
	void ConsumeThrownItem();
	void SetHeldThrowableHidden(bool bHidden);
	void FinishThrowMontage();
	void FinishAbility(bool bWasCancelled);

	UFUNCTION()
	void OnChargeReady(FGameplayEventData Payload);

	UFUNCTION()
	void OnInputReleased(float TimeHeld);

	UFUNCTION()
	void OnMaxChargeReached();

	UFUNCTION()
	void OnThrowReleaseEvent(FGameplayEventData Payload);

	UFUNCTION()
	void OnMontageCompleted();

	UFUNCTION()
	void OnMontageBlendOut();

	UFUNCTION()
	void OnMontageCancelled();

	UPROPERTY(EditDefaultsOnly, Category = "Throw|Animation")
	TObjectPtr<UAnimMontage> ThrowMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Throw|Projectile")
	TSubclassOf<AT_Throwable> ThrowableClass;

	UPROPERTY(EditDefaultsOnly, Category = "Throw|Projectile")
	FName HandSocketName{TEXT("hand_rThrow")};

	UPROPERTY(EditDefaultsOnly, Category = "Throw|Projectile")
	float ThrowPitchOffset{10.f};

	UPROPERTY(EditDefaultsOnly, Category = "Throw|Charge", meta = (ClampMin = "0.1"))
	float MaxChargeDuration{5.f};

	UPROPERTY(Transient)
	TObjectPtr<UT_AimingComponent> AimingComponent;

	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_PlayMontageAndWait> MontageTask;

	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_WaitGameplayEvent> ChargeReadyEventTask;

	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_WaitGameplayEvent> ReleaseEventTask;

	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_WaitInputRelease> WaitInputReleaseTask;

	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_WaitDelay> MaxChargeDelayTask;

	UPROPERTY(Transient)
	TObjectPtr<AT_ThrowTrajectoryPreview> ThrowPreview;

	FTimerHandle ThrowPreviewTimerHandle;

	bool bCharging{false};
	bool bReleaseRequested{false};
	bool bThrowExecuted{false};
	bool bHeldThrowableHidden{false};
};
