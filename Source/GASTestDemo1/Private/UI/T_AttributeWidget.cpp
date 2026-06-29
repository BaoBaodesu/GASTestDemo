// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/T_AttributeWidget.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Components/WidgetComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/PlayerController.h"
#include <string>
/**
* 当属性变化时调用
* Pair.Key 是当前属性
* Pair.Value 是最大属性
*/
 
void UT_AttributeWidget::OnAttributeChange(const TTuple<FGameplayAttribute, FGameplayAttribute>& Pair, UT_AttributeSet* AttributeSet, float OldValue)
{
	const float AttributeValue = Pair.Key.GetNumericValue(AttributeSet);
	const float MaxAttributeValue = Pair.Value.GetNumericValue(AttributeSet);
	
	BP_OnAttributeChange(AttributeValue, MaxAttributeValue, OldValue);
}

/**
* 判断当前 Widget 绑定的属性是否和传入的属性组合一致
*/
bool UT_AttributeWidget::MatchesAttributes(const TTuple<FGameplayAttribute, FGameplayAttribute>& Pair) const
{
	return Attribute == Pair.Key && MaxAttribute == Pair.Value;
}


void UT_AttributeWidget::SpawnDamageNumbers(float NewValue, float OldValue)
{
	if (OldValue <= 0.0f) { return; }
	if (NewValue >= OldValue) { return; }

	const float DamageValue = FMath::Abs(NewValue - OldValue);

	if (DamageValue <= 0.0f) { return; }

	const int32 DamageInt = FMath::FloorToInt(DamageValue);
	const FString DamageString = FString::FromInt(DamageInt);

	float TextOffset = 0.0f;

	for (int32 Index = 0; Index < DamageString.Len(); Index++)
	{
		const FString DigitString = FString(1, &DamageString[Index]);
		const int32 DigitIndex = FCString::Atoi(*DigitString);

		if (!NumberTextures.IsValidIndex(DigitIndex)) { continue; }
		if (!IsValid(NumberTextures[DigitIndex])) { continue; }

		UNiagaraComponent* NumberSystem = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			GetWorld(),
			NumberSystemTemplate,
			AvatarActor->GetActorLocation() + FVector(0.0f, 0.0f, VerticalOffset),
			FRotator::ZeroRotator,
			FVector(1.0f),
			true,
			true,
			ENCPoolMethod::None,
			true
		);

		if (!IsValid(NumberSystem)) { continue; }
		
		APlayerController* PlayerController = GetOwningPlayer();

		if (IsValid(PlayerController) && IsValid(PlayerController->PlayerCameraManager))
		{
			const FVector RightVector = PlayerController->PlayerCameraManager->GetCameraRotation().Quaternion().GetRightVector();

			NumberSystem->AddLocalOffset(RightVector * TextOffset);
		}
		
		NumberSystem->SetVariableTexture(FName(TEXT("User.Digit")), NumberTextures[DigitIndex]);

		TextOffset += NumberSpacing;
	}
}
