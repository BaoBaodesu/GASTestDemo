#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/T_GameplayAbility.h"
#include "GameplayTagContainer.h"
#include "Player/Components/T_GrabComponent.h"
#include "T_Grab.generated.h"

class UAbilityTask_WaitGameplayEvent;

UCLASS()
class GASTESTDEMO1_API UT_Grab : public UT_GameplayAbility
{
	GENERATED_BODY()

public:
	UT_Grab();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

private:
	void CreateEventTasks();
	void EndEventTasks();
	void HandleGrabStarted(ET_GrabType GrabType);
	void HandleGrabEnded();
	void RemoveGrabTags();

	UFUNCTION()
	void HandleStopCatch(FGameplayEventData Payload);

	UFUNCTION()
	void HandleRelease(FGameplayEventData Payload);

	UFUNCTION()
	void HandleMove(FGameplayEventData Payload);

	UFUNCTION()
	void HandleJump(FGameplayEventData Payload);

	UPROPERTY(Transient)
	TObjectPtr<UT_GrabComponent> GrabComponent;

	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_WaitGameplayEvent> StopCatchTask;

	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_WaitGameplayEvent> ReleaseTask;

	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_WaitGameplayEvent> MoveTask;

	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_WaitGameplayEvent> JumpTask;

	FGameplayTag CurrentGrabTypeTag;
	bool bGrabTagsApplied{false};
	bool bHasGrabStarted{false};
	bool bEndingFromGrabComponent{false};
	bool bEndingAbility{false};
};
