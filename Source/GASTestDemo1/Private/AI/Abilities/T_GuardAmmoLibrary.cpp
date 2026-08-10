// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/Abilities/T_GuardAmmoLibrary.h"

#include "AbilitySystem/T_AttributeSet.h"

namespace
{
	// 服务器权威直接写入复制属性（AI 弹药无 GE 聚合，直接更新 Base/Current 值）
	void SetAttributeDataValue(FGameplayAttributeData& Data, float Value)
	{
		Data.SetBaseValue(Value);
		Data.SetCurrentValue(Value);
	}
}

float UT_GuardAmmoLibrary::GetReloadAmount(const UT_AttributeSet* AttributeSet)
{
	if (!IsValid(AttributeSet)) return 0.f;

	return GetReloadAmount(AttributeSet->GetMagazineAmmo(), AttributeSet->GetMaxMagazineAmmo(), AttributeSet->GetReserveAmmo());
}

float UT_GuardAmmoLibrary::GetReloadAmount(float MagazineAmmo, float MaxMagazineAmmo, float ReserveAmmo)
{
	const float Missing = FMath::Max(0.f, MaxMagazineAmmo - MagazineAmmo);
	return FMath::Min(Missing, FMath::Max(0.f, ReserveAmmo));
}

bool UT_GuardAmmoLibrary::ApplyShotCost(UT_AttributeSet* AttributeSet)
{
	if (!IsValid(AttributeSet) || AttributeSet->GetMagazineAmmo() <= 0.f) return false;

	SetAttributeDataValue(AttributeSet->MagazineAmmo, FMath::Max(0.f, AttributeSet->GetMagazineAmmo() - 1.f));
	return true;
}

bool UT_GuardAmmoLibrary::ApplyReload(UT_AttributeSet* AttributeSet)
{
	if (!IsValid(AttributeSet)) return false;

	const float ReloadAmount = GetReloadAmount(AttributeSet);
	if (ReloadAmount <= 0.f) return false;

	SetAttributeDataValue(AttributeSet->MagazineAmmo, AttributeSet->GetMagazineAmmo() + ReloadAmount);
	SetAttributeDataValue(AttributeSet->ReserveAmmo, FMath::Max(0.f, AttributeSet->GetReserveAmmo() - ReloadAmount));
	return true;
}
