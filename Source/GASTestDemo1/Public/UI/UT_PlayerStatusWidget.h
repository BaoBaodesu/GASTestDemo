#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AbilitySystem/T_AttributeSet.h"
#include "UT_PlayerStatusWidget.generated.h"

class AT_PlayerCharacter;
class UAbilitySystemComponent;
class UAttributeSet;
class UT_AttributeWidget;
struct FOnAttributeChangeData;

/**
 * 玩家状态面板：驱动子控件中的 UT_AttributeWidget（血条/蓝条）跟随 GAS 属性变化。
 */
UCLASS()
class GASTESTDEMO1_API UT_PlayerStatusWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

protected:
	UFUNCTION()
	void OnPlayerASCInitialized(UAbilitySystemComponent* ASC, UAttributeSet* AS);

private:
	void BindToCharacter();
	void BindAttributeWidgets();
	void OnAttributeChanged(const FOnAttributeChangeData& AttributeChangeData, TWeakObjectPtr<UT_AttributeWidget> Widget, FGameplayAttribute Attribute, FGameplayAttribute MaxAttribute);

	UPROPERTY(Transient)
	TObjectPtr<AT_PlayerCharacter> BoundPlayerCharacter;

	UPROPERTY(Transient)
	TObjectPtr<UAbilitySystemComponent> BoundAbilitySystemComponent;

	UPROPERTY(Transient)
	TObjectPtr<UT_AttributeSet> BoundAttributeSet;

	TSet<FGameplayAttribute> BoundAttributeKeys;
};
