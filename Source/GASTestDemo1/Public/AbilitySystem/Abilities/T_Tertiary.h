#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/T_GameplayAbility.h"
#include "T_Tertiary.generated.h"

class UAnimInstance;
class UBrainComponent;
class UContextualAnimSceneActorComponent;
class UContextualAnimSceneAsset;
class USkeletalMeshComponent;

UCLASS()
class GASTESTDEMO1_API UT_Tertiary : public UT_GameplayAbility
{
	GENERATED_BODY()

public:
	UT_Tertiary();

protected:
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData
	) override;

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled
	) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tertiary")
	TObjectPtr<UContextualAnimSceneAsset> SceneAsset;

	// ABP_Shooter 无 DefaultSlot 时，临时切到该 AnimBP 以显示 AM_Vic_Test1
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tertiary|Victim")
	TSubclassOf<UAnimInstance> VictimFallbackAnimClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tertiary", meta = (ClampMin = "0.0"))
	float SearchRadius = 1000.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tertiary")
	float SearchForwardOffset = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tertiary")
	FName AttackerRole = TEXT("Attacker");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tertiary")
	FName VictimRole = TEXT("Victim");

	// 玩家相对敌人身后的站位距离（cm）
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tertiary|Warp", meta = (ClampMin = "0.0"))
	float AttackerBehindDistance = 110.f;

private:
	UFUNCTION()
	void OnLeftContextualAnimScene(UContextualAnimSceneActorComponent* SceneActorComponent);

	UFUNCTION()
	void OnVictimLeftContextualAnimScene(UContextualAnimSceneActorComponent* SceneActorComponent);

	void UnbindSceneLeftDelegate();
	bool PrepareVictimForContextualScene(AActor* VictimActor);
	void ClearVictimMotionWarpTargets(AActor* VictimActor) const;
	void OverrideAttackerWarpBehindVictim(AActor* AttackerActor, AActor* VictimActor, bool bSnapTransform);
	void PrepareAttackerStanceForContextualScene(AActor* AttackerActor);
	void RestoreAttackerStance();
	void RestoreAttackerMovementOrientation();
	void BindVictimLeaveDelegate();
	void RestoreVictimAnimInstance(const TCHAR* Reason);

	TWeakObjectPtr<AActor> CachedVictimActor;
	TWeakObjectPtr<USkeletalMeshComponent> CachedVictimMesh;
	TWeakObjectPtr<UContextualAnimSceneActorComponent> CachedVictimSceneComp;
	TWeakObjectPtr<UBrainComponent> CachedVictimBrain;
	TSubclassOf<UAnimInstance> CachedVictimPreviousAnimClass;
	bool bVictimAnimSwapped{false};
	bool bVictimBrainPaused{false};
	bool bVictimLeaveBound{false};
	bool bAttackerWasCrouched{false};
	bool bAttackerMovementOrientationCached{false};
	bool bCachedOrientRotationToMovement{true};
};
