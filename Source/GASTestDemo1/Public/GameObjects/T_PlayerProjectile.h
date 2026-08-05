#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "T_PlayerProjectile.generated.h"

class UGameplayEffect;
class UNiagaraComponent;
class UNiagaraSystem;
class UProjectileMovementComponent;
class USoundAttenuation;
class USoundBase;
class USphereComponent;

UCLASS()
class GASTESTDEMO1_API AT_PlayerProjectile : public AActor
{
	GENERATED_BODY()

public:
	AT_PlayerProjectile();

	void InitializeProjectile(TSubclassOf<UGameplayEffect> InDamageEffectClass, float InDamage,
		UNiagaraSystem* InTrailSystem, UNiagaraSystem* InImpactSystem,
		USoundBase* InImpactSound, USoundAttenuation* InImpactSoundAttenuation,
		AActor* WeaponActor);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	UFUNCTION()
	void OnProjectileHit(UPrimitiveComponent* HitComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComponent, FVector NormalImpulse, const FHitResult& Hit);

	UFUNCTION()
	void OnRep_TrailSystem();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastPlayImpactEffects(FVector ImpactPoint, FVector ImpactNormal);

	UPROPERTY(VisibleAnywhere, Category="Projectile")
	TObjectPtr<USphereComponent> CollisionComponent;

	UPROPERTY(VisibleAnywhere, Category="Projectile")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

	UPROPERTY(VisibleAnywhere, Category="Projectile|FX")
	TObjectPtr<UNiagaraComponent> TrailComponent;

	UPROPERTY(ReplicatedUsing=OnRep_TrailSystem)
	TObjectPtr<UNiagaraSystem> TrailSystem;

	UPROPERTY(Replicated)
	TObjectPtr<UNiagaraSystem> ImpactSystem;

	UPROPERTY(Replicated)
	TObjectPtr<USoundBase> ImpactSound;

	UPROPERTY(Replicated)
	TObjectPtr<USoundAttenuation> ImpactSoundAttenuation;

	UPROPERTY()
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	float Damage{0.f};
	bool bImpactHandled{false};
};
