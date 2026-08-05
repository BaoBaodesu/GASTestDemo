#include "AbilitySystem/Abilities/T_Shoot.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystemComponent.h"
#include "Characters/T_PlayerCharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameObjects/T_PlayerProjectile.h"
#include "GameObjects/T_ProjectileShooterComponent.h"
#include "GameplayEffect.h"
#include "GameplayTags/TTags.h"
#include "Player/Components/T_AimingComponent.h"
#include "UObject/ConstructorHelpers.h"

UT_Shoot::UT_Shoot()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	SetAssetTags(FGameplayTagContainer(TTags::TAbilities::Shoot.GetTag()));
	ActivationRequiredTags.AddTag(TTags::State::Aiming);
	ActivationOwnedTags.AddTag(TTags::State::Action::Shooting);

	static ConstructorHelpers::FObjectFinder<UAnimMontage> FireMontageAsset(TEXT("/Game/Characters/Mannequins/Anims/Pistol/MM_Pistol_Fire_Montage.MM_Pistol_Fire_Montage"));
	FireMontage = FireMontageAsset.Object;
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

	bShotExecuted = true;
	ExecuteShot();
	FireEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, TTags::Events::Player::Shoot::Fire, nullptr, true, true);
	if (!IsValid(FireEventTask)) { EndAbility(Handle, ActorInfo, ActivationInfo, true, true); return; }
	FireEventTask->EventReceived.AddDynamic(this, &ThisClass::OnFireEvent);
	FireEventTask->ReadyForActivation();

	MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, FireMontage, 1.f, NAME_None, false);
	if (!IsValid(MontageTask)) { EndAbility(Handle, ActorInfo, ActivationInfo, true, true); return; }
	MontageTask->OnCompleted.AddDynamic(this, &ThisClass::OnMontageCompleted);
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
	ProjectileShooterComponent->FireProjectile(AimPoint, ProjectileClass, DamageEffectClass, Damage, AvatarActor);
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
