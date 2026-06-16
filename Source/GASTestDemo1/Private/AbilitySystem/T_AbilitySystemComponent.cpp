// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/T_AbilitySystemComponent.h"

#include "GameplayTags/TTags.h"


// 当 Ability 被授予给 AbilitySystemComponent 时调用。
void UT_AbilitySystemComponent::OnGiveAbility(FGameplayAbilitySpec& AbilitySpec)
{
	Super::OnGiveAbility(AbilitySpec);
	
	HandleAutoActivatedAbility(AbilitySpec);
}

// 当可激活 Ability 列表在客户端同步完成时调用。
void UT_AbilitySystemComponent::OnRep_ActivateAbilities()
{
	Super::OnRep_ActivateAbilities();
	
	// 给 Ability 列表加锁，防止遍历时 Ability 列表被修改
	FScopedAbilityListLock AbilityListLock(*this);
	// 遍历当前 ASC 中所有可激活的 Ability
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

// 检查传入的 Ability 是否带有 ActivateOnGiven 标签。
void UT_AbilitySystemComponent::HandleAutoActivatedAbility(const FGameplayAbilitySpec& AbilitySpec)
{
	if (!IsValid(AbilitySpec.Ability)) return;
	
	for (const FGameplayTag& Tag : AbilitySpec.Ability->GetAssetTags())
	{
		if (Tag.MatchesTagExact(TTags::TAbilities::ActivateOnGiven))
		{
			// 尝试激活这个 Ability
			TryActivateAbility(AbilitySpec.Handle);
			return;
		}
	}
}
