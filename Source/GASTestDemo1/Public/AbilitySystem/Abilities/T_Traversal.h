#pragma once

#include "CoreMinimal.h"
#include "T_GameplayAbility.h"
#include "Player/Components/T_TraversalComponent.h"
#include "T_Traversal.generated.h"

class UAbilityTask_PlayMontageAndWait;
class UAnimMontage;
class UCharacterMovementComponent;
class AT_PlayerCharacter;
#if WITH_DEV_AUTOMATION_TESTS
class FTraversalSafeCompletionTest;
#endif
UCLASS()
class GASTESTDEMO1_API UT_Traversal : public UT_GameplayAbility
{
	GENERATED_BODY()

public:
	UT_Traversal();

	virtual bool CanActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags = nullptr,
		const FGameplayTagContainer* TargetTags = nullptr,
		FGameplayTagContainer* OptionalRelevantTags = nullptr
	) const override;

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData
	) override;

protected:
	UPROPERTY(Transient)
	TObjectPtr<AT_PlayerCharacter> PlayerCharacter;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Traversal")
	TObjectPtr<UAnimMontage> VaultMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Traversal")
	TObjectPtr<UAnimMontage> ClimbMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Traversal")
	TObjectPtr<UAnimMontage> MantleMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Traversal|Motion Warping")
	FName FrontLedgeWarpTargetName = TEXT("FrontLedge");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Traversal|Motion Warping")
	FName BackLedgeWarpTargetName = TEXT("BackLedge");

	TObjectPtr<UT_TraversalComponent> TraversalComponent;
	TObjectPtr<UCharacterMovementComponent> CharacterMovementComponent;
	ETraversalActionType CurrentActionType = ETraversalActionType::None;
	FTraversalCheckResult CurrentTraversalResult;
	FVector TraversalStartLocation = FVector::ZeroVector;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Traversal|Collision", meta = (ClampMin = "0.0", Units = "cm"))
	float MaximumLandingCorrectionDistance = 30.f;

	// 最终恢复完整胶囊时，让胶囊底部略高于地面，避免浮点误差造成初始穿透
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Traversal|Collision", meta = (ClampMin = "0.0", Units = "cm"))
	float LandingCollisionClearance = 2.f;

	UAnimMontage* GetTraversalMontage() const;
	void UpdateMotionWarpTargets(const FTraversalCheckResult& Result) const;
	void ClearMotionWarpTargets() const;
	void RestoreCharacterState(bool bWasCancelled, bool bRestoredToLanding);
	bool TryRestoreTraversalCollision(bool bWasCancelled);

#if WITH_DEV_AUTOMATION_TESTS
	friend class FTraversalSafeCompletionTest;
#endif

	UFUNCTION()
	void OnTraversalFinished();

	UFUNCTION()
	void OnTraversalCancelled();

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled
	) override;
};
