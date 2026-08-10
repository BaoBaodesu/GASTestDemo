#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TimerManager.h"
#include "UT_EquipmentCardWidget.generated.h"

class AT_PlayerCharacter;
class UAbilitySystemComponent;
class UAttributeSet;
class UImage;
class UT_InventoryComponent;
class UT_ItemDefinition;
class UT_AttributeSet;
class UTextBlock;
struct FGameplayEventData;
struct FGameplayTag;
struct FOnAttributeChangeData;

/**
 * 武器状态卡片：仅在装备武器时显示，展示武器图标、名称与弹匣/备弹。
 */
UCLASS()
class GASTESTDEMO1_API UT_EquipmentCardWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

protected:
	UFUNCTION()
	void OnEquippedItemChanged(UT_ItemDefinition* ItemDefinition);

	UFUNCTION()
	void OnPlayerASCInitialized(UAbilitySystemComponent* ASC, UAttributeSet* AS);

private:
	void OnReserveAmmoChanged(const FOnAttributeChangeData& AttributeChangeData);
	void OnMagazineAmmoChanged(const FOnAttributeChangeData& AttributeChangeData);
	void BindToCharacter();
	void BindHitMarkEvents();
	void OnHitMarkEvent(FGameplayTag EventTag, const FGameplayEventData* Payload);
	void HideHitMark();
	void RefreshAmmoFromTimer();
	void RefreshAmmoText();
	void UpdateEquipmentDisplay(UT_ItemDefinition* ItemDefinition);
	TArray<UWidget*> FindWidgetsByPrefix(const FString& Prefix) const;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UImage> ItemCard;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> ItemName;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> ReserveAmount;

	UPROPERTY(Transient)
	TObjectPtr<UImage> AmmunitionEmpty;

	UPROPERTY(Transient)
	TObjectPtr<UImage> HitMark;

	UPROPERTY(Transient)
	TObjectPtr<UT_InventoryComponent> Inventory;

	UPROPERTY(Transient)
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(Transient)
	TObjectPtr<UT_AttributeSet> AttributeSet;

	UPROPERTY(Transient)
	TObjectPtr<AT_PlayerCharacter> PlayerCharacter;

	FTimerHandle AmmoRefreshTimer;
	FTimerHandle HitMarkTimer;
	FDelegateHandle HitEventDelegateHandle;
};
