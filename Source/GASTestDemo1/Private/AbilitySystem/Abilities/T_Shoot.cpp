#include "AbilitySystem/Abilities/T_Shoot.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/T_AttributeSet.h"
#include "AI/Abilities/T_GuardAmmoLibrary.h"
#include "Characters/T_EnemyCharacter.h"
#include "Characters/T_PlayerCharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameObjects/T_PlayerProjectile.h"
#include "Player/Components/T_ProjectileShooterComponent.h"
#include "GameplayEffect.h"
#include "GameplayTags/TTags.h"
#include "Kismet/GameplayStatics.h"
#include "Player/Components/T_AimingComponent.h"
#include "Sound/SoundBase.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	// 瞄准命中点为敌人且落在头部骨骼附近时，判定为头部命中
	bool IsEnemyHeadHit(const FHitResult& CameraHit, FName HeadBoneName, float ToleranceRadius)
	{
		AActor* HitActor = CameraHit.GetActor();
		if (!IsValid(HitActor) || !HitActor->IsA<AT_EnemyCharacter>()) return false;
		if (HeadBoneName.IsNone() || ToleranceRadius <= 0.f) return false;

		ACharacter* HitCharacter = Cast<ACharacter>(HitActor);
		USkeletalMeshComponent* HitMesh = IsValid(HitCharacter) ? HitCharacter->GetMesh() : nullptr;
		if (!IsValid(HitMesh) || HitMesh->GetBoneIndex(HeadBoneName) == INDEX_NONE) return false;

		const FVector HeadLocation = HitMesh->GetBoneLocation(HeadBoneName);
		return FVector::Dist(CameraHit.ImpactPoint, HeadLocation) <= ToleranceRadius;
	}
}

UT_Shoot::UT_Shoot()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	SetAssetTags(FGameplayTagContainer(TTags::TAbilities::Shoot.GetTag()));
	ActivationRequiredTags.AddTag(TTags::State::Aiming);
	ActivationOwnedTags.AddTag(TTags::State::Action::Shooting);
	ActivationBlockedTags.AddTag(TTags::State::Action::Reloading);

	static ConstructorHelpers::FObjectFinder<UAnimMontage> FireMontageAsset(TEXT("/Game/Characters/Mannequins/Anims/Pistol/MM_Pistol_Fire_Montage.MM_Pistol_Fire_Montage"));
	FireMontage = FireMontageAsset.Object;
	static ConstructorHelpers::FObjectFinder<USoundBase> DryFireSoundAsset(TEXT("/Game/VisualSandbox/Audio/Cue/Weapons/DryFire_Cue.DryFire_Cue"));
	DryFireSound = DryFireSoundAsset.Object;
	static ConstructorHelpers::FClassFinder<AT_PlayerProjectile> ProjectileClassAsset(TEXT("/Game/GASTestDemo/GameObjects/Projectile/BP_Proj_Bullet1.BP_Proj_Bullet1_C"));
	ProjectileClass = ProjectileClassAsset.Class;
	static ConstructorHelpers::FClassFinder<UGameplayEffect> DamageEffectClassAsset(TEXT("/Game/GASTestDemo/AbilitySystem/GameplayEffects/Player/GE_PlayerDanage.GE_PlayerDanage_C"));
	DamageEffectClass = DamageEffectClassAsset.Class;
}

void UT_Shoot::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	AT_PlayerCharacter* PlayerCharacter = Cast<AT_PlayerCharacter>(GetAvatarActorFromActorInfo());
	UAbilitySystemComponent* AbilitySystemComponent = GetAbilitySystemComponentFromActorInfo();
	if (!IsValid(PlayerCharacter) || !IsValid(AbilitySystemComponent) || !AbilitySystemComponent->HasMatchingGameplayTag(TTags::State::Aiming)) { EndAbility(Handle, ActorInfo, ActivationInfo, true, true); return; }

	AimingComponent = PlayerCharacter->FindComponentByClass<UT_AimingComponent>();
	WeaponMesh = PlayerCharacter->GetEquippedWeaponMesh();
	ProjectileShooterComponent = IsValid(WeaponMesh) && IsValid(WeaponMesh->GetOwner()) ? WeaponMesh->GetOwner()->FindComponentByClass<UT_ProjectileShooterComponent>() : nullptr;
	if (!IsValid(AimingComponent) || !IsValid(WeaponMesh) || !IsValid(ProjectileShooterComponent) || !IsValid(ProjectileClass) || !IsValid(FireMontage))
	{
		UE_LOG(LogTemp, Warning, TEXT("T_Shoot prerequisites: AimingComponent=%s, WeaponMesh=%s, ProjectileShooterComponent=%s, ProjectileClass=%s, FireMontage=%s."), IsValid(AimingComponent) ? TEXT("Valid") : TEXT("Invalid"), IsValid(WeaponMesh) ? TEXT("Valid") : TEXT("Invalid"), IsValid(ProjectileShooterComponent) ? TEXT("Valid") : TEXT("Invalid"), IsValid(ProjectileClass) ? TEXT("Valid") : TEXT("Invalid"), IsValid(FireMontage) ? TEXT("Valid") : TEXT("Invalid"));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UT_AttributeSet* AttributeSet = Cast<UT_AttributeSet>(PlayerCharacter->GetAttributeSet());
	if (IsValid(AttributeSet) && AttributeSet->GetMagazineAmmo() <= 0.f)
	{
		// 空膛：播放空弹音效且不发射
		if (IsValid(DryFireSound))
		{
			UGameplayStatics::SpawnSoundAttached(DryFireSound, WeaponMesh, NAME_None, FVector::ZeroVector, EAttachLocation::KeepRelativeOffset, true);
		}
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	bShotExecuted = true;
	ExecuteShot();
	FireEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, TTags::Events::Player::Shoot::Fire, nullptr, true, true);
	if (!IsValid(FireEventTask)) { EndAbility(Handle, ActorInfo, ActivationInfo, true, true); return; }
	FireEventTask->EventReceived.AddDynamic(this, &ThisClass::OnFireEvent);
	FireEventTask->ReadyForActivation();

	MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, FireMontage, 1.f, NAME_None, false);
	if (!IsValid(MontageTask)) { EndAbility(Handle, ActorInfo, ActivationInfo, true, true); return; }
	MontageTask->OnCompleted.AddDynamic(this, &ThisClass::OnMontageCompleted);
	MontageTask->OnBlendOut.AddDynamic(this, &ThisClass::OnMontageCancelled);
	MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::OnMontageCancelled);
	MontageTask->OnCancelled.AddDynamic(this, &ThisClass::OnMontageCancelled);
	MontageTask->ReadyForActivation();
}

void UT_Shoot::OnFireEvent(FGameplayEventData Payload)
{
	if (bShotExecuted) return;
	bShotExecuted = true;
	ExecuteShot();
}

void UT_Shoot::ExecuteShot()
{
	if (!IsValid(AimingComponent) || !IsValid(ProjectileShooterComponent) || !IsValid(ProjectileClass)) return;

	FVector AimPoint;
	FHitResult CameraHit;
	if (!AimingComponent->GetCameraAimPoint(AimPoint, CameraHit)) return;
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!IsValid(AvatarActor)) return;

	// 命中敌人且瞄准点在敌人头部骨骼附近时按头部伤害倍率结算
	const bool bHeadshot = IsEnemyHeadHit(CameraHit, HeadBoneName, HeadshotToleranceRadius);
	AT_PlayerProjectile* Projectile = ProjectileShooterComponent->FireProjectile(AimPoint, ProjectileClass, DamageEffectClass, Damage, AvatarActor, bHeadshot);
	if (!IsValid(Projectile)) return;

	// 射击成功才扣除 1 发弹匣弹药（仅权威端会生成投射物）
	AT_PlayerCharacter* PlayerCharacter = Cast<AT_PlayerCharacter>(AvatarActor);
	UT_AttributeSet* AttributeSet = IsValid(PlayerCharacter) ? Cast<UT_AttributeSet>(PlayerCharacter->GetAttributeSet()) : nullptr;
	if (IsValid(AttributeSet)) UT_GuardAmmoLibrary::ApplyShotCost(AttributeSet);
}

void UT_Shoot::OnMontageCompleted()
{
	if (!bShotExecuted)
	{
		bShotExecuted = true;
		ExecuteShot();
	}
	FinishAbility(false);
}

void UT_Shoot::OnMontageCancelled()
{
	FinishAbility(true);
}

void UT_Shoot::FinishAbility(bool bWasCancelled)
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, bWasCancelled);
}

void UT_Shoot::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (IsValid(FireEventTask)) FireEventTask->EndTask();
	FireEventTask = nullptr;
	MontageTask = nullptr;
	AimingComponent = nullptr;
	WeaponMesh = nullptr;
	ProjectileShooterComponent = nullptr;
	bShotExecuted = false;
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
