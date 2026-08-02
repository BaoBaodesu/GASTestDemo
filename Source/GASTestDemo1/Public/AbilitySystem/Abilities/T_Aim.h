#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/T_GameplayAbility.h"
#include "T_Aim.generated.h"

class UAbilityTask_WaitInputRelease;
class UT_AimingComponent;

UCLASS()
class GASTESTDEMO1_API UT_Aim : public UT_GameplayAbility
{
	GENERATED_BODY()

public:
	UT_Aim();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

private:
	UFUNCTION()
	void OnInputReleased(float TimeHeld);

	UPROPERTY(Transient)
	TObjectPtr<UT_AimingComponent> AimingComponent;

	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_WaitInputRelease> WaitInputReleaseTask;
};
