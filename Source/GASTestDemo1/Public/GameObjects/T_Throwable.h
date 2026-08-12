#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "T_Throwable.generated.h"

class UProjectileMovementComponent;
class UNiagaraComponent;
class UNiagaraSystem;
class USoundBase;
class USphereComponent;
class UStaticMeshComponent;

UCLASS(Blueprintable)
class GASTESTDEMO1_API AT_Throwable : public AActor
{
	GENERATED_BODY()

public:
	AT_Throwable();

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Crash|Throwable")
	void LaunchThrowable(FVector Direction, float Speed = 0.f);

	float GetPredictionCollisionRadius() const;
	float GetPredictionGravityScale() const;
	float GetLaunchSpeed(float SpeedOverride = 0.f) const;
	float GetMaxThrowDistance() const { return MaxThrowDistance; }

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	static bool IsImpactSignificant(
		float ImpactSpeed,
		float MinimumImpactSpeed,
		float CurrentTime,
		float LastImpactTime,
		float ImpactCooldown);

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintImplementableEvent, Category = "Crash|Throwable", meta = (DisplayName = "On Throwable Impact"))
	void OnThrowableImpact(FVector ImpactLocation, FVector ImpactNormal, float ImpactSpeed);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Crash|Throwable")
	TObjectPtr<USphereComponent> SphereCollision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Crash|Throwable")
	TObjectPtr<UStaticMeshComponent> StaticMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Crash|Throwable")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Crash|Throwable|FX")
	TObjectPtr<UNiagaraComponent> TrailComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Crash|Throwable|FX")
	TObjectPtr<UNiagaraSystem> TrailSystem;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Crash|Throwable|FX")
	TObjectPtr<UNiagaraSystem> ImpactSystem;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Crash|Throwable|Impact")
	TObjectPtr<USoundBase> ImpactSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Crash|Throwable|Impact", meta = (ClampMin = "0.0"))
	float MinimumImpactSpeed = 150.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Crash|Throwable|Impact", meta = (ClampMin = "0.0"))
	float ImpactCooldown = 0.15f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Crash|Throwable|Impact", meta = (ClampMin = "0.0"))
	float ImpactSoundVolume = 1.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Crash|Throwable|AI")
	bool bReportImpactNoiseToAI = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Crash|Throwable|AI")
	bool bOnlyReportFirstImpactToAI = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Crash|Throwable|AI", meta = (ClampMin = "0.0"))
	float ImpactNoiseLoudness = 1.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Crash|Throwable|AI", meta = (ClampMin = "0.0"))
	float ImpactNoiseMaxRange = 1500.f;

	/** 从出手点计算的最大飞行距离；设为 0 时不限制。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Crash|Throwable|Movement", meta = (ClampMin = "0.0", Units = "cm"))
	float MaxThrowDistance = 3000.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Crash|Throwable|Impact")
	bool bDestroyOnImpact = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Crash|Throwable|Impact")
	bool bHideMeshOnImpact = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Crash|Throwable|Impact", meta = (ClampMin = "0.0"))
	float DestroyDelayAfterImpact = 0.1f;

private:
	UFUNCTION()
	void HandleSphereHit(
		UPrimitiveComponent* HitComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComponent,
		FVector NormalImpulse,
		const FHitResult& Hit);

	UFUNCTION()
	void HandleProjectileStop(const FHitResult& ImpactResult);

	UFUNCTION()
	void OnRep_TrailActive();

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastImpactEffects(FVector ImpactLocation, FVector ImpactNormal, float ImpactSpeed);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastBreakEffects(FVector ImpactLocation, FVector ImpactNormal, float ImpactSpeed);

	void PlayImpactEffects(FVector ImpactLocation, FVector ImpactNormal, float ImpactSpeed);
	void SetTrailActive(bool bActive);

	UPROPERTY(ReplicatedUsing = OnRep_TrailActive)
	bool bTrailActive = false;

	float LastImpactTime = -BIG_NUMBER;
	FVector LaunchLocation = FVector::ZeroVector;
	bool bHasReportedImpactNoise = false;
	bool bImpactHandled = false;
};
