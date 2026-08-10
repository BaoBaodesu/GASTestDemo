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

	void InitializeProjectile(TSubclassOf<UGameplayEffect> InDamageEffectClass, float InDamage, bool bInHeadshot,
		UNiagaraSystem* InTrailSystem, UNiagaraSystem* InImpactSystem,
		USoundBase* InImpactSound, USoundAttenuation* InImpactSoundAttenuation,
		AActor* WeaponActor);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// 让本发投射物在移动扫描中忽略指定 Actor，用于无敌窗口内的穿身
	void SetMoveIgnoredActor(AActor* Actor, bool bIgnore);

protected:
	virtual void BeginPlay() override;

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

	// 命中敌人头部骨骼时的伤害倍率（默认 2 倍）
	UPROPERTY(EditDefaultsOnly, Category="Projectile|Damage")
	float HeadshotDamageMultiplier{2.f};

	float Damage{0.f};
	bool bHeadshot{false};
	bool bImpactHandled{false};
};
