// Fill out your copyright notice in the Description page of Project Settings.


#include "GASTestDemo1/Public/Characters/T_BaseCharacter.h"
#include "AbilitySystemComponent.h"

// Sets default values
AT_BaseCharacter::AT_BaseCharacter()
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	
	// 即使角色不可见，也始终更新动画姿势并刷新骨骼变换
	// 确保专用服务器能够正常获取最新的骨骼位置
	GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
}

UAbilitySystemComponent* AT_BaseCharacter::GetAbilitySystemComponent() const
{
	return nullptr;
}

void AT_BaseCharacter::GiveStartupAbilities()
{
	if (!IsValid(GetAbilitySystemComponent())) return;
	
	for (const auto& Ability : StartupAbilities)
	{
		FGameplayAbilitySpec AbilitySpec = FGameplayAbilitySpec(Ability);
		GetAbilitySystemComponent()->GiveAbility(AbilitySpec);
	}
}

void AT_BaseCharacter::InitializeAttributes() const
{
	// 如果没有设置，会直接报错并停止运行。
	checkf(IsValid(InitializeAttributesEffect), TEXT("InitializeAttributesEffect not set."));

	FGameplayEffectContextHandle ContextHandle = GetAbilitySystemComponent()->MakeEffectContext();
	FGameplayEffectSpecHandle SpecHandle = GetAbilitySystemComponent()->MakeOutgoingSpec(InitializeAttributesEffect, 1.0f, ContextHandle);
	GetAbilitySystemComponent()->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
}

