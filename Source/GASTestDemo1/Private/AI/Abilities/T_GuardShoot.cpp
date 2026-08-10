// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/Abilities/T_GuardShoot.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "AbilitySystemComponent.h"
#include "AI/Abilities/T_GuardAim.h"
#include "AI/Abilities/T_GuardAmmoLibrary.h"
#include "AI/T_ShooterAIController.h"
#include "AbilitySystem/T_AttributeSet.h"
#include "Characters/T_GuardCharacter.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "GameObjects/T_PlayerProjectile.h"
#include "GameObjects/T_ProjectileShooterComponent.h"
#include "GameplayEffect.h"
#include "GameplayTags/TTags.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	const FGameplayTag& GetGuardDeadTag()
	{
		static const FGameplayTag DeadTag = FGameplayTag::RequestGameplayTag(TEXT("TTags.Status.Dead"));
		return DeadTag;
	}
}

UT_GuardShoot::UT_GuardShoot()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
	SetAssetTags(FGameplayTagContainer(TTags::TAbilities::Enemy::Shoot.GetTag()));
	ActivationRequiredTags.AddTag(TTags::State::Aiming);
	ActivationOwnedTags.AddTag(TTags::State::Action::Shooting);
	ActivationBlockedTags.AddTag(TTags::State::Action::Reloading);
	ActivationBlockedTags.AddTag(TTags::State::Action::HitReact);
	const FGameplayTag& LocalDeadTag = GetGuardDeadTag();
	if (LocalDeadTag.IsValid()) ActivationBlockedTags.AddTag(LocalDeadTag);

	static ConstructorHelpers::FObjectFinder<UAnimMontage> FireMontageAsset(TEXT("/Game/Characters/Mannequins/Anims/Pistol/MM_Pistol_Fire_Montage.MM_Pistol_Fire_Montage"));
	FireMontage = FireMontageAsset.Object;
	static ConstructorHelpers::FClassFinder<AT_PlayerProjectile> ProjectileClassAsset(TEXT("/Game/GASTestDemo/GameObjects/Projectile/BP_Proj_Bullet1.BP_Proj_Bullet1_C"));
	ProjectileClass = ProjectileClassAsset.Class;
	static ConstructorHelpers::FClassFinder<UGameplayEffect> DamageEffectClassAsset(TEXT("/Game/GASTestDemo/AbilitySystem/GameplayEffects/Enemy/GE_Enemy_Ranged_Damage.GE_Enemy_Ranged_Damage_C"));
	DamageEffectClass = DamageEffectClassAsset.Class;
}

bool UT_GuardShoot::IsInterruptingTag(const FGameplayTag& Tag)
{
	if (!Tag.IsValid()) return false;

	return Tag == TTags::State::Action::Reloading
		|| Tag == TTags::State::Action::HitReact
		|| Tag == GetGuardDeadTag();
}

void UT_GuardShoot::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	AT_GuardCharacter* Guard = Cast<AT_GuardCharacter>(GetAvatarActorFromActorInfo());
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!IsValid(Guard) || !IsValid(ASC) || !ASC->HasMatchingGameplayTag(TTags::State::Aiming))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	AT_ShooterAIController* ShooterController = Cast<AT_ShooterAIController>(Guard->GetController());
	AActor* Target = IsValid(ShooterController) ? ShooterController->GetCurrentTarget() : nullptr;
	if (!Guard->IsWeaponReady() || !IsValid(Target) || !IsValid(ProjectileClass) || !IsValid(FireMontage))
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: UT_GuardShoot 前置条件不满足 (WeaponReady=%s, Target=%s, ProjectileClass=%s, FireMontage=%s)。"),
			*GetName(), Guard->IsWeaponReady() ? TEXT("true") : TEXT("false"), IsValid(Target) ? TEXT("true") : TEXT("false"),
			IsValid(ProjectileClass) ? TEXT("true") : TEXT("false"), IsValid(FireMontage) ? TEXT("true") : TEXT("false"));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 注册中断标签监听：死亡/受击/换弹出现时立即中断射击
	AbilitySystemComponent = ASC;
	DeadTag = GetGuardDeadTag();
	ReloadTagHandle = ASC->RegisterGameplayTagEvent(TTags::State::Action::Reloading).AddUObject(this, &ThisClass::OnInterruptTagChanged);
	HitReactTagHandle = ASC->RegisterGameplayTagEvent(TTags::State::Action::HitReact).AddUObject(this, &ThisClass::OnInterruptTagChanged);
	if (DeadTag.IsValid()) DeadTagHandle = ASC->RegisterGameplayTagEvent(DeadTag).AddUObject(this, &ThisClass::OnInterruptTagChanged);

	if (!ExecuteShot(Target))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, FireMontage, 1.f, NAME_None, false);
	if (!IsValid(MontageTask))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	MontageTask->OnCompleted.AddDynamic(this, &ThisClass::OnMontageCompleted);
	MontageTask->OnBlendOut.AddDynamic(this, &ThisClass::OnMontageCancelled);
	MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::OnMontageCancelled);
	MontageTask->OnCancelled.AddDynamic(this, &ThisClass::OnMontageCancelled);
	MontageTask->ReadyForActivation();
}

bool UT_GuardShoot::ExecuteShot(AActor* Target)
{
	AT_GuardCharacter* Guard = Cast<AT_GuardCharacter>(GetAvatarActorFromActorInfo());
	if (!IsValid(Guard) || !Guard->IsWeaponReady() || !IsValid(Target) || bShotFired)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: ExecuteShot 前置条件不满足 (Guard=%s WeaponReady=%s Target=%s bShotFired=%s)。"),
			*GetName(),
			IsValid(Guard) ? TEXT("true") : TEXT("false"),
			Guard && Guard->IsWeaponReady() ? TEXT("true") : TEXT("false"),
			IsValid(Target) ? TEXT("true") : TEXT("false"),
			bShotFired ? TEXT("true") : TEXT("false"));
		return false;
	}

	UT_ProjectileShooterComponent* Shooter = Guard->GetProjectileShooterComponent();
	const FVector TargetLocation = Target->GetActorLocation();
	// AimHeightOffset 相对脚底；GetActorLocation 是胶囊中心，需减去半高
	FVector AimPoint = TargetLocation;
	if (const ACharacter* TargetCharacter = Cast<ACharacter>(Target))
	{
		if (const UCapsuleComponent* Capsule = TargetCharacter->GetCapsuleComponent())
		{
			AimPoint.Z = TargetLocation.Z - Capsule->GetScaledCapsuleHalfHeight() + AimHeightOffset;
		}
	}
	else
	{
		AimPoint.Z = TargetLocation.Z + AimHeightOffset;
	}
	AT_PlayerProjectile* Projectile = Shooter->FireProjectile(AimPoint, ProjectileClass, DamageEffectClass, Damage, Guard);
	if (!IsValid(Projectile))
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: ExecuteShot 投射物生成失败，跳过扣弹。"), *GetName());
		return false;
	}

	bShotFired = true;

	UT_AttributeSet* AttributeSet = Cast<UT_AttributeSet>(Guard->GetAttributeSet());
	if (IsValid(AttributeSet)) UT_GuardAmmoLibrary::ApplyShotCost(AttributeSet);
	return true;
}

void UT_GuardShoot::OnInterruptTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	if (NewCount > 0 && IsInterruptingTag(Tag))
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
	}
}

void UT_GuardShoot::OnMontageCompleted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UT_GuardShoot::OnMontageCancelled()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UT_GuardShoot::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (UAbilitySystemComponent* ASC = AbilitySystemComponent.Get())
	{
		if (ReloadTagHandle.IsValid()) ASC->UnregisterGameplayTagEvent(ReloadTagHandle, TTags::State::Action::Reloading);
		if (HitReactTagHandle.IsValid()) ASC->UnregisterGameplayTagEvent(HitReactTagHandle, TTags::State::Action::HitReact);
		if (DeadTagHandle.IsValid() && DeadTag.IsValid()) ASC->UnregisterGameplayTagEvent(DeadTagHandle, DeadTag);
	}

	AbilitySystemComponent = nullptr;
	DeadTag = FGameplayTag();
	ReloadTagHandle.Reset();
	HitReactTagHandle.Reset();
	DeadTagHandle.Reset();

	if (IsValid(MontageTask)) MontageTask->EndTask();
	MontageTask = nullptr;
	bShotFired = false;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);

	// 开火蒙太奇结束后恢复 ADS 举枪姿势（若 Aim 能力仍激活）
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
		{
			if (!Spec.IsActive()) continue;
			if (UT_GuardAim* AimAbility = Cast<UT_GuardAim>(Spec.GetPrimaryInstance()))
			{
				AimAbility->RestartAimPose();
				break;
			}
		}
	}
}
