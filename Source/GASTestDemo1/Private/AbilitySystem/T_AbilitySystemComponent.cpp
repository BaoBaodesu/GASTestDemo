// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/T_AbilitySystemComponent.h"

#include "AbilitySystem/Abilities/Enemy/T_HitReact.h"
#include "AbilitySystem/Abilities/T_PrimaryComboAbility.h"
#include "GameplayTags/TTags.h"

void UT_AbilitySystemComponent::OnGiveAbility(FGameplayAbilitySpec& AbilitySpec)
{
	Super::OnGiveAbility(AbilitySpec);
	
	HandleAutoActivatedAbility(AbilitySpec);
}

void UT_AbilitySystemComponent::OnRep_ActivateAbilities()
{
	Super::OnRep_ActivateAbilities();
	
	FScopedAbilityListLock AbilityListLock(*this);
	for (const FGameplayAbilitySpec& AbilitySpec : GetActivatableAbilities())
	{
		HandleAutoActivatedAbility(AbilitySpec);
	}
}

void UT_AbilitySystemComponent::SetAbilityLevel(TSubclassOf<UGameplayAbility> AbilityClass, int32 Level)
{
	if (IsValid(GetAvatarActor()) && !GetAvatarActor()->HasAuthority()) return;

	if (FGameplayAbilitySpec* AbilitySpec = FindAbilitySpecFromClass(AbilityClass))
	{
		AbilitySpec->Level = Level;
		MarkAbilitySpecDirty(*AbilitySpec);
	}
}

void UT_AbilitySystemComponent::AddToAbilityLevel(TSubclassOf<UGameplayAbility> AbilityClass, int32 Level)
{
	if (IsValid(GetAvatarActor()) && !GetAvatarActor()->HasAuthority()) return;

	if (FGameplayAbilitySpec* AbilitySpec = FindAbilitySpecFromClass(AbilityClass))
	{
		AbilitySpec->Level += Level;
		MarkAbilitySpecDirty(*AbilitySpec);
	}
}

void UT_AbilitySystemComponent::HandleAutoActivatedAbility(const FGameplayAbilitySpec& AbilitySpec)
{
	if (!IsValid(AbilitySpec.Ability)) return;

	// HitReact is event-driven; parent BP ActivateOnGiven would sticky-own HitReact tags
	if (AbilitySpec.Ability->IsA(UT_HitReact::StaticClass())) return;
	
	for (const FGameplayTag& Tag : AbilitySpec.Ability->GetAssetTags())
	{
		if (Tag.MatchesTagExact(TTags::TAbilities::ActivateOnGiven))
		{
			TryActivateAbility(AbilitySpec.Handle);
			return;
		}
	}
}

FGameplayTag UT_AbilitySystemComponent::ResolveRoutedInputTag(const FGameplayTag& InputTag) const
{
	if (!InputTag.MatchesTagExact(TTags::TAbilities::Primary)) return InputTag;
	if (HasMatchingGameplayTag(TTags::State::Action::Throwing)) return TTags::TAbilities::Throw;
	if (!HasMatchingGameplayTag(TTags::State::Aiming)) return InputTag;
	return HasMatchingGameplayTag(TTags::State::ThrowableEquipped) ? TTags::TAbilities::Throw : TTags::TAbilities::Shoot;
}

void UT_AbilitySystemComponent::AbilityInputTagPressed(const FGameplayTag& InputTag)
{
	if (!InputTag.IsValid()) return;
	const FGameplayTag RoutedInputTag = ResolveRoutedInputTag(InputTag);

	if (RoutedInputTag.MatchesTagExact(TTags::TAbilities::Shoot) || RoutedInputTag.MatchesTagExact(TTags::TAbilities::Reload))
	{
		FGameplayTagContainer MeleeAbilityTags;
		MeleeAbilityTags.AddTag(TTags::TAbilities::Primary);
		MeleeAbilityTags.AddTag(TTags::TAbilities::Tertiary);
		CancelAbilities(&MeleeAbilityTags);
	}

	bool bPreferPrimaryCombo = false;
	if (RoutedInputTag.MatchesTagExact(TTags::TAbilities::Primary))
	{
		for (const FGameplayAbilitySpec& Spec : GetActivatableAbilities())
		{
			if (IsValid(Spec.Ability) && Spec.Ability->IsA(UT_PrimaryComboAbility::StaticClass())
				&& Spec.Ability->GetAssetTags().HasTagExact(TTags::TAbilities::Primary))
			{
				bPreferPrimaryCombo = true;
				break;
			}
		}
	}

	for (FGameplayAbilitySpec& AbilitySpec : GetActivatableAbilities())
	{
		if (!AbilitySpec.Ability) continue;
		if (!AbilitySpec.Ability->GetAssetTags().HasTagExact(RoutedInputTag)) continue;
		if (bPreferPrimaryCombo && !AbilitySpec.Ability->IsA(UT_PrimaryComboAbility::StaticClass())) continue;

		AbilitySpec.InputPressed = true;
		if (AbilitySpec.IsActive())
		{
			AbilitySpecInputPressed(AbilitySpec);
		}
		else
		{
			TryActivateAbility(AbilitySpec.Handle);
		}
	}
}

void UT_AbilitySystemComponent::AbilityInputTagReleased(const FGameplayTag& InputTag)
{
	if (!InputTag.IsValid()) return;
	const FGameplayTag RoutedInputTag = ResolveRoutedInputTag(InputTag);

	for (FGameplayAbilitySpec& AbilitySpec : GetActivatableAbilities())
	{
		if (!AbilitySpec.Ability) continue;
		if (!AbilitySpec.Ability->GetAssetTags().HasTagExact(RoutedInputTag)) continue;
		AbilitySpec.InputPressed = false;
		if (AbilitySpec.IsActive())
		{
			AbilitySpecInputReleased(AbilitySpec);

PRAGMA_DISABLE_DEPRECATION_WARNINGS
			TArray<UGameplayAbility*> AbilityInstances = AbilitySpec.GetAbilityInstances();
			const FGameplayAbilityActivationInfo& ActivationInfo = AbilityInstances.IsEmpty() ? AbilitySpec.ActivationInfo : AbilityInstances.Last()->GetCurrentActivationInfoRef();
PRAGMA_ENABLE_DEPRECATION_WARNINGS
			InvokeReplicatedEvent(EAbilityGenericReplicatedEvent::InputReleased, AbilitySpec.Handle, ActivationInfo.GetActivationPredictionKey());
		}
	}
}
