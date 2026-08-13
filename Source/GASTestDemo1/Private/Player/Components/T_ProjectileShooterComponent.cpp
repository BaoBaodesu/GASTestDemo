#include "Player/Components/T_ProjectileShooterComponent.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Characters/T_PlayerCharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameObjects/T_PlayerProjectile.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Perception/AISense_Hearing.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundBase.h"
#include "UObject/ConstructorHelpers.h"

UT_ProjectileShooterComponent::UT_ProjectileShooterComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> MuzzleSystemAsset(TEXT("/Game/NW_MuzzleFX/Particle_FX/FXS_Pistol_MuzzleFlash.FXS_Pistol_MuzzleFlash"));
	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> TrailSystemAsset(TEXT("/Game/VisualSandbox/EpicAssets/Effects/Particles/Explosion/NS_Bullet_Trail.NS_Bullet_Trail"));
	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> ImpactSystemAsset(TEXT("/Game/VisualSandbox/FX/FXS_Default_Impact.FXS_Default_Impact"));
	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> ShellEjectionSystemAsset(TEXT("/Game/VisualSandbox/FX/FXS_Capsule_Ejection.FXS_Capsule_Ejection"));
	static ConstructorHelpers::FObjectFinder<USoundBase> FireSoundAsset(TEXT("/Game/VisualSandbox/Audio/Cue/Weapons/Gunshot_AssaultRifle_Cue.Gunshot_AssaultRifle_Cue"));
	static ConstructorHelpers::FObjectFinder<USoundBase> ImpactSoundAsset(TEXT("/Game/VisualSandbox/Audio/Cue/Impacts/Impact_Default_Cue.Impact_Default_Cue"));
	static ConstructorHelpers::FObjectFinder<USoundAttenuation> FireSoundAttenuationAsset(TEXT("/Game/VisualSandbox/Audio/Attenuations/A_Gunshot.A_Gunshot"));
	static ConstructorHelpers::FObjectFinder<USoundAttenuation> ImpactSoundAttenuationAsset(TEXT("/Game/VisualSandbox/Audio/Attenuations/A_Bullet_Impact.A_Bullet_Impact"));
	static ConstructorHelpers::FObjectFinder<UAnimMontage> WeaponFireMontageAsset(TEXT("/Game/VisualSandbox/EpicAssets/Weapons/Pistol/Animations/Weap_Pistol_Fire_Montage.Weap_Pistol_Fire_Montage"));
	MuzzleSystem = MuzzleSystemAsset.Object;
	TrailSystem = TrailSystemAsset.Object;
	ImpactSystem = ImpactSystemAsset.Object;
	ShellEjectionSystem = ShellEjectionSystemAsset.Object;
	FireSound = FireSoundAsset.Object;
	ImpactSound = ImpactSoundAsset.Object;
	FireSoundAttenuation = FireSoundAttenuationAsset.Object;
	ImpactSoundAttenuation = ImpactSoundAttenuationAsset.Object;
	WeaponFireMontage = WeaponFireMontageAsset.Object;
}

void UT_ProjectileShooterComponent::BeginPlay()
{
	Super::BeginPlay();
	if (GetNetMode() == NM_DedicatedServer) return;

	if (IsValid(FireSound)) UGameplayStatics::PrimeSound(FireSound);
	if (IsValid(ImpactSound)) UGameplayStatics::PrimeSound(ImpactSound);

	UNiagaraSystem* SystemsToPrecache[] = {MuzzleSystem, TrailSystem, ImpactSystem, ShellEjectionSystem};
	for (UNiagaraSystem* System : SystemsToPrecache)
	{
		if (IsValid(System)) System->PrecachePSOs();
	}
}

AT_PlayerProjectile* UT_ProjectileShooterComponent::FireProjectile(const FVector& AimPoint,
	TSubclassOf<AT_PlayerProjectile> ProjectileClass,
	TSubclassOf<UGameplayEffect> DamageEffectClass, float Damage, AActor* SourceActor,
	bool bHeadshot, float SpreadHalfAngleDegrees, bool bForbiddenPistolShot)
{
	UWorld* World = GetWorld();
	if (!IsValid(World) || !IsValid(SourceActor) || !SourceActor->HasAuthority() || !IsValid(ProjectileClass)) return nullptr;

	USkeletalMeshComponent* WeaponMesh = GetOwner() ? GetOwner()->FindComponentByClass<USkeletalMeshComponent>() : nullptr;
	if (!IsValid(WeaponMesh) || !WeaponMesh->DoesSocketExist(MuzzleSocketName))
	{
		UE_LOG(LogTemp, Warning, TEXT("ProjectileShooterComponent could not find weapon mesh or muzzle socket '%s'."), *MuzzleSocketName.ToString());
		return nullptr;
	}

	const FVector MuzzleLocation = WeaponMesh->GetSocketLocation(MuzzleSocketName);
	const FVector AimDirection = ApplySpreadToDirection((AimPoint - MuzzleLocation).GetSafeNormal(), SpreadHalfAngleDegrees);
	if (AimDirection.IsNearlyZero()) return nullptr;
	if (IsValid(WeaponFireMontage) && IsValid(WeaponMesh->GetAnimInstance())) WeaponMesh->GetAnimInstance()->Montage_Play(WeaponFireMontage);

	if (IsValid(MuzzleSystem)) UNiagaraFunctionLibrary::SpawnSystemAttached(MuzzleSystem, WeaponMesh, MuzzleSocketName, FVector::ZeroVector, FRotator::ZeroRotator, EAttachLocation::KeepRelativeOffset, true, true, ENCPoolMethod::AutoRelease, true);
	if (IsValid(FireSound)) UGameplayStatics::SpawnSoundAttached(FireSound, WeaponMesh, MuzzleSocketName, FVector::ZeroVector, EAttachLocation::KeepRelativeOffset, true, 1.f, 1.f, 0.f, FireSoundAttenuation);

	if (IsValid(WeaponMesh) && IsValid(ShellEjectionSystem) && WeaponMesh->DoesSocketExist(ShellEjectionSocketName))
	{
		UNiagaraFunctionLibrary::SpawnSystemAttached(ShellEjectionSystem, WeaponMesh, ShellEjectionSocketName, FVector::ZeroVector, FRotator::ZeroRotator, EAttachLocation::KeepRelativeOffset, true, true, ENCPoolMethod::AutoRelease, true);
	}

	const FTransform SpawnTransform(AimDirection.Rotation(), MuzzleLocation);
	AT_PlayerProjectile* Projectile = World->SpawnActorDeferred<AT_PlayerProjectile>(ProjectileClass, SpawnTransform, SourceActor, Cast<APawn>(SourceActor), ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!IsValid(Projectile)) return nullptr;
	Projectile->InitializeProjectile(DamageEffectClass, Damage, bHeadshot, TrailSystem, ImpactSystem, ImpactSound, ImpactSoundAttenuation, GetOwner(), bForbiddenPistolShot);
	UGameplayStatics::FinishSpawningActor(Projectile, SpawnTransform);
	if (SourceActor->IsA<AT_PlayerCharacter>())
	{
		UAISense_Hearing::ReportNoiseEvent(
			this,
			MuzzleLocation,
			1.f,
			SourceActor,
			1800.f,
			TEXT("GuardNoise.Gunshot"));
	}
	return Projectile;
}

FVector UT_ProjectileShooterComponent::ApplySpreadToDirection(const FVector& Direction, float SpreadHalfAngleDegrees)
{
	const FVector NormalizedDirection = Direction.GetSafeNormal();
	if (NormalizedDirection.IsNearlyZero() || SpreadHalfAngleDegrees <= 0.f) return NormalizedDirection;
	return FMath::VRandCone(NormalizedDirection, FMath::DegreesToRadians(SpreadHalfAngleDegrees));
}
