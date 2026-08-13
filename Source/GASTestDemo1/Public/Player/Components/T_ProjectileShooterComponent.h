#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "T_ProjectileShooterComponent.generated.h"

class AT_PlayerProjectile;
class UAnimMontage;
class UGameplayEffect;
class UNiagaraSystem;
class USoundAttenuation;
class USoundBase;

UCLASS(ClassGroup=(Weapon), meta=(BlueprintSpawnableComponent))
class GASTESTDEMO1_API UT_ProjectileShooterComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	UT_ProjectileShooterComponent();

	AT_PlayerProjectile* FireProjectile(const FVector& AimPoint,
		TSubclassOf<AT_PlayerProjectile> ProjectileClass,
		TSubclassOf<UGameplayEffect> DamageEffectClass, float Damage, AActor* SourceActor,
		bool bHeadshot = false, float SpreadHalfAngleDegrees = 0.f,
		bool bForbiddenPistolShot = false);

	static FVector ApplySpreadToDirection(const FVector& Direction, float SpreadHalfAngleDegrees);

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(EditDefaultsOnly, Category="Projectile Shooter|FX")
	TObjectPtr<UNiagaraSystem> MuzzleSystem;

	UPROPERTY(EditDefaultsOnly, Category="Projectile Shooter|FX")
	TObjectPtr<UNiagaraSystem> TrailSystem;

	UPROPERTY(EditDefaultsOnly, Category="Projectile Shooter|FX")
	TObjectPtr<UNiagaraSystem> ImpactSystem;

	UPROPERTY(EditDefaultsOnly, Category="Projectile Shooter|FX")
	TObjectPtr<UNiagaraSystem> ShellEjectionSystem;

	UPROPERTY(EditDefaultsOnly, Category="Projectile Shooter|Sound")
	TObjectPtr<USoundBase> FireSound;

	UPROPERTY(EditDefaultsOnly, Category="Projectile Shooter|Sound")
	TObjectPtr<USoundBase> ImpactSound;

	UPROPERTY(EditDefaultsOnly, Category="Projectile Shooter|Sound")
	TObjectPtr<USoundAttenuation> FireSoundAttenuation;

	UPROPERTY(EditDefaultsOnly, Category="Projectile Shooter|Sound")
	TObjectPtr<USoundAttenuation> ImpactSoundAttenuation;

	UPROPERTY(EditDefaultsOnly, Category="Projectile Shooter|Weapon")
	FName MuzzleSocketName{TEXT("Muzzle")};

	UPROPERTY(EditDefaultsOnly, Category="Projectile Shooter|Weapon")
	FName ShellEjectionSocketName{TEXT("Eject")};

	UPROPERTY(EditDefaultsOnly, Category="Projectile Shooter|Animation")
	TObjectPtr<UAnimMontage> WeaponFireMontage;
};
