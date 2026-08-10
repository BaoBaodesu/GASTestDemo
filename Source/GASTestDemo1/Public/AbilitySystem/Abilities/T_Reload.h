#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/T_GameplayAbility.h"
#include "T_Reload.generated.h"

class UAbilityTask_PlayMontageAndWait;
class UAbilityTask_WaitGameplayEvent;
class UAnimMontage;
class UT_AttributeSet;

/**
 * 玩家换弹能力：播放换弹蒙太奇与换弹音效，
 * 蒙太奇内 AnimNotify（Events.Player.Reload.Complete）触发时完成弹药转移。
 */
UCLASS()
class GASTESTDEMO1_API UT_Reload : public UT_GameplayAbility
{
	GENERATED_BODY()

public:
	UT_Reload();

	// 激活期间持有/阻挡标签（供测试与蓝图检查）
	const FGameplayTagContainer& GetActivationOwnedTags() const { return ActivationOwnedTags; }
	const FGameplayTagContainer& GetActivationBlockedTags() const { return ActivationBlockedTags; }

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

private:
	void CompleteReload();

	UFUNCTION()
	void OnReloadCompleteEvent(FGameplayEventData Payload);

	UFUNCTION()
	void OnMontageCompleted();

	UFUNCTION()
	void OnMontageBlendOut();

	UFUNCTION()
	void OnMontageCancelled();

	UPROPERTY(EditDefaultsOnly, Category="Reload|Animation")
	TObjectPtr<UAnimMontage> ReloadMontage;

	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_PlayMontageAndWait> MontageTask;

	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_WaitGameplayEvent> ReloadEventTask;

	TWeakObjectPtr<UT_AttributeSet> CachedAttributeSet;

	bool bAmmoTransferred{false};
};
