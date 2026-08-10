// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"

#include "T_AttributeSet.generated.h"

#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FAttributesInitialized);

UCLASS()
class GASTESTDEMO1_API UT_AttributeSet : public UAttributeSet
{
	GENERATED_BODY()
	
public:
	UT_AttributeSet();

	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;
	
	UPROPERTY(BlueprintAssignable)
	FAttributesInitialized OnAttributesInitialized;

	UPROPERTY(ReplicatedUsing = OnRep_AttributesInitialized)
	bool bAttributesInitialized = false;

	UFUNCTION()
	void OnRep_AttributesInitialized();
	
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Health)
	FGameplayAttributeData Health;
	
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxHealth)
	FGameplayAttributeData MaxHealth;
	
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Mana)
	FGameplayAttributeData Mana;
	
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxMana)
	FGameplayAttributeData MaxMana;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MagazineAmmo)
	FGameplayAttributeData MagazineAmmo;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxMagazineAmmo)
	FGameplayAttributeData MaxMagazineAmmo;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_ReserveAmmo)
	FGameplayAttributeData ReserveAmmo;
	
	UFUNCTION()
	void OnRep_Health(const FGameplayAttributeData& OldValue);
	
	UFUNCTION()
	void OnRep_MaxHealth(const FGameplayAttributeData& OldValue);
	
	UFUNCTION()
	void OnRep_Mana(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_MagazineAmmo(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_MaxMagazineAmmo(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_ReserveAmmo(const FGameplayAttributeData& OldValue);
	
	UFUNCTION()
	void OnRep_MaxMana(const FGameplayAttributeData& OldValue);
	
	ATTRIBUTE_ACCESSORS(ThisClass, Health);
	ATTRIBUTE_ACCESSORS(ThisClass, MaxHealth);
	ATTRIBUTE_ACCESSORS(ThisClass, Mana);
	ATTRIBUTE_ACCESSORS(ThisClass, MaxMana);
	ATTRIBUTE_ACCESSORS(ThisClass, MagazineAmmo);
	ATTRIBUTE_ACCESSORS(ThisClass, MaxMagazineAmmo);
	ATTRIBUTE_ACCESSORS(ThisClass, ReserveAmmo);
};
