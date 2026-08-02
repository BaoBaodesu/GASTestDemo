#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/T_GameplayAbility.h"
#include "Engine/EngineTypes.h"
#include "T_Shoot.generated.h"

class UAbilityTask_PlayMontageAndWait;
class UAbilityTask_WaitGameplayEvent;
class UAnimMontage;
class UGameplayEffect;
class UNiagaraSystem;
class USkeletalMeshComponent;
class UT_AimingComponent;

UCLASS()
class GASTESTDEMO1_API UT_Shoot : public UT_GameplayAbility
{
	GENERATED_BODY()

public:
	UT_Shoot();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

private:
	void ExecuteShot();
	void FinishAbility(bool bWasCancelled);

	UFUNCTION()
	void OnFireEvent(FGameplayEventData Payload);

	UFUNCTION()
	void OnMontageCompleted();

	UFUNCTION()
	void OnMontageCancelled();

	UPROPERTY(EditDefaultsOnly, Category="Shoot|Animation")
	TObjectPtr<UAnimMontage> FireMontage;

	UPROPERTY(EditDefaultsOnly, Category="Shoot|Damage")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	UPROPERTY(EditDefaultsOnly, Category="Shoot|Damage")
	float Damage{10.f};

	UPROPERTY(EditDefaultsOnly, Category="Shoot|Trace")
	float TraceDistance{10000.f};

	UPROPERTY(EditDefaultsOnly, Category="Shoot|Trace")
	TEnumAsByte<ECollisionChannel> TraceChannel{ECC_Visibility};

	UPROPERTY(EditDefaultsOnly, Category="Shoot|Weapon")
	FName MuzzleSocketName{TEXT("Muzzle")};

	UPROPERTY(EditDefaultsOnly, Category="Shoot|Niagara")
	TObjectPtr<UNiagaraSystem> MuzzleSystem;

	UPROPERTY(EditDefaultsOnly, Category="Shoot|Niagara")
	TObjectPtr<UNiagaraSystem> TracerSystem;

	UPROPERTY(EditDefaultsOnly, Category="Shoot|Niagara")
	TObjectPtr<UNiagaraSystem> ImpactSystem;

	UPROPERTY(EditDefaultsOnly, Category="Shoot|Niagara")
	FName TracerStartParameter{TEXT("User.Start")};

	UPROPERTY(EditDefaultsOnly, Category="Shoot|Niagara")
	FName TracerEndParameter{TEXT("User.End")};

	UPROPERTY(Transient)
	TObjectPtr<UT_AimingComponent> AimingComponent;

	UPROPERTY(Transient)
	TObjectPtr<USkeletalMeshComponent> WeaponMesh;

	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_WaitGameplayEvent> FireEventTask;

	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_PlayMontageAndWait> MontageTask;

	bool bShotExecuted{false};
};
