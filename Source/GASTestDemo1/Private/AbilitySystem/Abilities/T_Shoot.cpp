#include "AbilitySystem/Abilities/T_Shoot.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Characters/T_PlayerCharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameplayEffect.h"
#include "GameplayTags/TTags.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Player/Components/T_AimingComponent.h"

UT_Shoot::UT_Shoot()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	SetAssetTags(FGameplayTagContainer(TTags::TAbilities::Shoot.GetTag()));
	ActivationRequiredTags.AddTag(TTags::State::Aiming);
	ActivationOwnedTags.AddTag(TTags::State::Action::Shooting);
}

void UT_Shoot::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	AT_PlayerCharacter* PlayerCharacter = Cast<AT_PlayerCharacter>(GetAvatarActorFromActorInfo());
	UAbilitySystemComponent* AbilitySystemComponent = GetAbilitySystemComponentFromActorInfo();
	if (!IsValid(PlayerCharacter) || !IsValid(AbilitySystemComponent) || !AbilitySystemComponent->HasMatchingGameplayTag(TTags::State::Aiming)) { EndAbility(Handle, ActorInfo, ActivationInfo, true, true); return; }

	AimingComponent = PlayerCharacter->FindComponentByClass<UT_AimingComponent>();
	WeaponMesh = PlayerCharacter->GetEquippedWeaponMesh();
	if (!IsValid(AimingComponent) || !IsValid(WeaponMesh) || !IsValid(FireMontage)) { UE_LOG(LogTemp, Warning, TEXT("T_Shoot prerequisites: AimingComponent=%s, WeaponMesh=%s, FireMontage=%s."), IsValid(AimingComponent) ? TEXT("Valid") : TEXT("Invalid"), IsValid(WeaponMesh) ? TEXT("Valid") : TEXT("Invalid"), IsValid(FireMontage) ? TEXT("Valid") : TEXT("Invalid")); EndAbility(Handle, ActorInfo, ActivationInfo, true, true); return; }
	if (!WeaponMesh->DoesSocketExist(MuzzleSocketName)) { UE_LOG(LogTemp, Warning, TEXT("T_Shoot could not find muzzle socket '%s'."), *MuzzleSocketName.ToString()); EndAbility(Handle, ActorInfo, ActivationInfo, true, true); return; }

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
	if (!IsValid(AimingComponent) || !IsValid(WeaponMesh)) return;

	FVector AimPoint;
	FHitResult CameraHit;
	if (!AimingComponent->GetCameraAimPoint(AimPoint, CameraHit)) return;

	UWorld* World = GetWorld();
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!IsValid(World) || !IsValid(AvatarActor)) return;

	const FVector MuzzleLocation = WeaponMesh->GetSocketTransform(MuzzleSocketName, RTS_World).GetLocation();
	const FVector AimDirection = (AimPoint - MuzzleLocation).GetSafeNormal();
	if (AimDirection.IsNearlyZero()) return;
	const FVector TraceEnd = AimPoint;

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(TShootMuzzleTrace), true, AvatarActor);
	if (IsValid(WeaponMesh->GetOwner()) && WeaponMesh->GetOwner() != AvatarActor) QueryParams.AddIgnoredActor(WeaponMesh->GetOwner());
	FHitResult HitResult;
	const bool bHit = World->LineTraceSingleByChannel(HitResult, MuzzleLocation, TraceEnd, TraceChannel, QueryParams);
	const FVector FinalPoint = bHit ? HitResult.ImpactPoint : AimPoint;
	const FVector ShotDirection = (FinalPoint - MuzzleLocation).GetSafeNormal();

	if (IsValid(MuzzleSystem)) UNiagaraFunctionLibrary::SpawnSystemAttached(MuzzleSystem, WeaponMesh, MuzzleSocketName, FVector::ZeroVector, FRotator::ZeroRotator, EAttachLocation::KeepRelativeOffset, true, true, ENCPoolMethod::AutoRelease, true);
	if (IsValid(TracerSystem))
	{
		UNiagaraComponent* TracerComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, TracerSystem, MuzzleLocation, FRotator::ZeroRotator, FVector::OneVector, true, false, ENCPoolMethod::AutoRelease, true);
		if (IsValid(TracerComponent)) { TracerComponent->SetVariablePosition(TracerStartParameter, MuzzleLocation); TracerComponent->SetVariablePosition(TracerEndParameter, FinalPoint); TracerComponent->Activate(true); }
	}
	if (bHit && IsValid(ImpactSystem)) UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, ImpactSystem, HitResult.ImpactPoint, HitResult.ImpactNormal.Rotation(), FVector::OneVector, true, true, ENCPoolMethod::AutoRelease, true);

	if (!bHit || !IsValid(HitResult.GetActor()) || !IsValid(DamageEffectClass) || Damage <= 0.f) return;
	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(HitResult.GetActor());
	if (!IsValid(SourceASC) || !IsValid(TargetASC)) return;

	FGameplayEffectContextHandle ContextHandle = SourceASC->MakeEffectContext();
	ContextHandle.AddHitResult(HitResult, true);
	FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(DamageEffectClass, GetAbilityLevel(), ContextHandle);
	if (!SpecHandle.IsValid()) return;
	SpecHandle.Data->SetSetByCallerMagnitude(TTags::SetByCaller::Projectile, Damage);
	SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
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
	bShotExecuted = false;
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
