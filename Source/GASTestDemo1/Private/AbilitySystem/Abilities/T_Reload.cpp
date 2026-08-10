#include "AbilitySystem/Abilities/T_Reload.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/T_AttributeSet.h"
#include "AI/Abilities/T_GuardAmmoLibrary.h"
#include "Characters/T_PlayerCharacter.h"
#include "GameplayTags/TTags.h"
#include "UObject/ConstructorHelpers.h"

UT_Reload::UT_Reload()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	SetAssetTags(FGameplayTagContainer(TTags::TAbilities::Reload.GetTag()));
	ActivationOwnedTags.AddTag(TTags::State::Action::Reloading);
	ActivationBlockedTags.AddTag(TTags::State::Action::Attacking);
	ActivationBlockedTags.AddTag(TTags::State::Action::Rolling);
	ActivationBlockedTags.AddTag(TTags::State::Action::Traversing);
	ActivationBlockedTags.AddTag(TTags::State::Action::Grabbing);
	ActivationBlockedTags.AddTag(TTags::State::Action::HitReact);
	ActivationBlockedTags.AddTag(TTags::State::Action::Shooting);
	ActivationBlockedTags.AddTag(TTags::State::Action::Reloading);

	static ConstructorHelpers::FObjectFinder<UAnimMontage> ReloadMontageAsset(TEXT("/Game/Characters/Mannequins/Anims/Pistol/MM_Pistol_Reload_Montage.MM_Pistol_Reload_Montage"));
	ReloadMontage = ReloadMontageAsset.Object;
}

void UT_Reload::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	AT_PlayerCharacter* PlayerCharacter = Cast<AT_PlayerCharacter>(GetAvatarActorFromActorInfo());
	UAbilitySystemComponent* AbilitySystemComponent = GetAbilitySystemComponentFromActorInfo();
	if (!IsValid(PlayerCharacter) || !IsValid(AbilitySystemComponent))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UT_AttributeSet* AttributeSet = Cast<UT_AttributeSet>(PlayerCharacter->GetAttributeSet());
	if (!IsValid(AttributeSet)
		|| AttributeSet->GetMagazineAmmo() >= AttributeSet->GetMaxMagazineAmmo()
		|| AttributeSet->GetReserveAmmo() <= 0.f
		|| !PlayerCharacter->HasPistolGun()
		|| !IsValid(PlayerCharacter->GetEquippedWeaponMesh()))
	{
		UE_LOG(LogTemp, Warning, TEXT("T_Reload prerequisites: PistolGun=%s, WeaponMesh=%s, MagazineAmmo=%.0f, MaxMagazineAmmo=%.0f, ReserveAmmo=%.0f."),
			PlayerCharacter->HasPistolGun() ? TEXT("true") : TEXT("false"),
			IsValid(PlayerCharacter->GetEquippedWeaponMesh()) ? TEXT("true") : TEXT("false"),
			AttributeSet ? AttributeSet->GetMagazineAmmo() : -1.f,
			AttributeSet ? AttributeSet->GetMaxMagazineAmmo() : -1.f,
			AttributeSet ? AttributeSet->GetReserveAmmo() : -1.f);
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!IsValid(ReloadMontage))
	{
		UE_LOG(LogTemp, Warning, TEXT("T_Reload 未配置换弹蒙太奇。"));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	CachedAttributeSet = AttributeSet;
	bAmmoTransferred = false;

	UE_LOG(LogTemp, Log, TEXT("T_Reload 开始换弹：Magazine=%.0f, MaxMagazine=%.0f, Reserve=%.0f."),
		AttributeSet->GetMagazineAmmo(), AttributeSet->GetMaxMagazineAmmo(), AttributeSet->GetReserveAmmo());

	ReloadEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, TTags::Events::Player::Reload::Complete, nullptr, true, true);
	if (!IsValid(ReloadEventTask))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	ReloadEventTask->EventReceived.AddDynamic(this, &ThisClass::OnReloadCompleteEvent);
	ReloadEventTask->ReadyForActivation();

	MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, ReloadMontage, 1.f, NAME_None, false);
	if (!IsValid(MontageTask))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	MontageTask->OnCompleted.AddDynamic(this, &ThisClass::OnMontageCompleted);
	MontageTask->OnBlendOut.AddDynamic(this, &ThisClass::OnMontageBlendOut);
	MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::OnMontageCancelled);
	MontageTask->OnCancelled.AddDynamic(this, &ThisClass::OnMontageCancelled);
	MontageTask->ReadyForActivation();
}

void UT_Reload::OnReloadCompleteEvent(FGameplayEventData Payload)
{
	CompleteReload();
}

void UT_Reload::CompleteReload()
{
	if (bAmmoTransferred) return;
	bAmmoTransferred = true;

	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	UT_AttributeSet* AttributeSet = CachedAttributeSet.Get();
	if (!IsValid(AvatarActor) || !AvatarActor->HasAuthority() || !IsValid(AttributeSet)) return;

	const float MagazineBefore = AttributeSet->GetMagazineAmmo();
	const float ReserveBefore = AttributeSet->GetReserveAmmo();
	if (UT_GuardAmmoLibrary::ApplyReload(AttributeSet))
	{
		UE_LOG(LogTemp, Log, TEXT("T_Reload 弹药转移完成：Magazine %.0f -> %.0f, Reserve %.0f -> %.0f."),
			MagazineBefore, AttributeSet->GetMagazineAmmo(), ReserveBefore, AttributeSet->GetReserveAmmo());
	}
}

void UT_Reload::OnMontageCompleted()
{
	// 若未收到换弹完成通知（蒙太奇未配置 AnimNotify），在蒙太奇结束时兜底完成换弹
	if (!bAmmoTransferred)
	{
		UE_LOG(LogTemp, Warning, TEXT("T_Reload 未收到 Events.Player.Reload.Complete 通知，已在蒙太奇结束时兜底完成换弹。"));
		CompleteReload();
	}
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UT_Reload::OnMontageBlendOut()
{
	// 蒙太奇进入 blend-out 视为接近结束：若尚未收到完成通知，兜底完成换弹，不当作取消处理
	if (!bAmmoTransferred)
	{
		UE_LOG(LogTemp, Warning, TEXT("T_Reload 未收到 Events.Player.Reload.Complete 通知，已在蒙太奇 blend-out 时兜底完成换弹。"));
		CompleteReload();
	}
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UT_Reload::OnMontageCancelled()
{
	// 被打断不补充弹药
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UT_Reload::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (IsValid(ReloadEventTask)) ReloadEventTask->EndTask();
	ReloadEventTask = nullptr;
	if (IsValid(MontageTask)) MontageTask->EndTask();
	MontageTask = nullptr;
	CachedAttributeSet = nullptr;
	bAmmoTransferred = false;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
