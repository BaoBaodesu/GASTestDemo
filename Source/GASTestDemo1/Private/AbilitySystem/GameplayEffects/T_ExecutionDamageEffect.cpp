#include "AbilitySystem/GameplayEffects/T_ExecutionDamageEffect.h"

#include "AbilitySystem/T_AttributeSet.h"
#include "GameplayTags/TTags.h"

UT_ExecutionDamageEffect::UT_ExecutionDamageEffect()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	FSetByCallerFloat SetByCallerMagnitude;
	SetByCallerMagnitude.DataTag = TTags::SetByCaller::Melee.GetTag();

	FGameplayModifierInfo& HealthModifier = Modifiers.AddDefaulted_GetRef();
	HealthModifier.Attribute = UT_AttributeSet::GetHealthAttribute();
	HealthModifier.ModifierOp = EGameplayModOp::AddBase;
	HealthModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(SetByCallerMagnitude);
}
