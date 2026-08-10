#include "UI/WeaponState/UT_ThirdPersonCrosshairWidget.h"

#include "AbilitySystem/T_AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "Animation/WidgetAnimation.h"
#include "Characters/T_PlayerCharacter.h"
#include "GameplayTags/TTags.h"

void UT_ThirdPersonCrosshairWidget::NativeConstruct()
{
	Super::NativeConstruct();

	BindToAbilitySystem();
}

void UT_ThirdPersonCrosshairWidget::NativeDestruct()
{
	if (IsValid(AbilitySystemComponent) && HitEventDelegateHandle.IsValid())
	{
		FGameplayTagContainer HitTagContainer;
		HitTagContainer.AddTag(TTags::Events::Player::Shoot::Hit);
		AbilitySystemComponent->RemoveGameplayEventTagContainerDelegate(HitTagContainer, HitEventDelegateHandle);
		HitEventDelegateHandle.Reset();
	}
	AbilitySystemComponent = nullptr;

	if (IsValid(PlayerCharacter))
	{
		PlayerCharacter->OnASCInitialized.RemoveDynamic(this, &ThisClass::OnPlayerASCInitialized);
		PlayerCharacter = nullptr;
	}

	Super::NativeDestruct();
}

void UT_ThirdPersonCrosshairWidget::BindToAbilitySystem()
{
	PlayerCharacter = Cast<AT_PlayerCharacter>(GetOwningPlayerPawn());
	if (!IsValid(PlayerCharacter)) return;

	AbilitySystemComponent = PlayerCharacter->GetAbilitySystemComponent();
	if (!IsValid(AbilitySystemComponent) || HitEventDelegateHandle.IsValid())
	{
		if (!IsValid(AbilitySystemComponent))
			PlayerCharacter->OnASCInitialized.AddUniqueDynamic(this, &ThisClass::OnPlayerASCInitialized);
		return;
	}

	FGameplayTagContainer HitTagContainer;
	HitTagContainer.AddTag(TTags::Events::Player::Shoot::Hit);
	HitEventDelegateHandle = AbilitySystemComponent->AddGameplayEventTagContainerDelegate(
		HitTagContainer,
		FGameplayEventTagMulticastDelegate::FDelegate::CreateUObject(this, &ThisClass::OnHitMarkEvent));
}

void UT_ThirdPersonCrosshairWidget::OnPlayerASCInitialized(UAbilitySystemComponent* ASC, UAttributeSet* AS)
{
	if (!IsValid(ASC) || HitEventDelegateHandle.IsValid()) return;

	if (IsValid(PlayerCharacter))
		PlayerCharacter->OnASCInitialized.RemoveDynamic(this, &ThisClass::OnPlayerASCInitialized);

	AbilitySystemComponent = ASC;

	FGameplayTagContainer HitTagContainer;
	HitTagContainer.AddTag(TTags::Events::Player::Shoot::Hit);
	HitEventDelegateHandle = ASC->AddGameplayEventTagContainerDelegate(
		HitTagContainer,
		FGameplayEventTagMulticastDelegate::FDelegate::CreateUObject(this, &ThisClass::OnHitMarkEvent));
}

void UT_ThirdPersonCrosshairWidget::OnHitMarkEvent(FGameplayTag EventTag, const FGameplayEventData* Payload)
{
	if (EventTag != TTags::Events::Player::Shoot::Hit) return;

	if (UWidgetAnimation* HitMarkAnimation = HitMarkAnim.Get())
	{
		PlayAnimation(HitMarkAnimation, 0.f, 1, EUMGSequencePlayMode::Forward, 1.f, true);
	}
}
