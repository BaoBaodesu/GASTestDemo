#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayTagContainer.h"
#include "UT_ThirdPersonCrosshairWidget.generated.h"

class AT_PlayerCharacter;
class UAbilitySystemComponent;
class UAttributeSet;
class UWidgetAnimation;
struct FGameplayEventData;

/**
 * 第三人称瞄准准心：监听玩家命中确认事件并播放命中标记动画。
 */
UCLASS()
class GASTESTDEMO1_API UT_ThirdPersonCrosshairWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

protected:
	UPROPERTY(Transient, meta=(BindWidgetAnim))
	TObjectPtr<UWidgetAnimation> HitMarkAnim;

private:
	void BindToAbilitySystem();
	void OnHitMarkEvent(FGameplayTag EventTag, const FGameplayEventData* Payload);

	UFUNCTION()
	void OnPlayerASCInitialized(UAbilitySystemComponent* ASC, UAttributeSet* AS);

	UPROPERTY(Transient)
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(Transient)
	TObjectPtr<AT_PlayerCharacter> PlayerCharacter;

	FDelegateHandle HitEventDelegateHandle;
};
