// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "T_BaseCharacter.h"
#include "T_EnemyCharacter.generated.h"

class UAttributeSet;
class UT_ActorWidgetComponent;
UCLASS()
class GASTESTDEMO1_API AT_EnemyCharacter : public AT_BaseCharacter
{
	GENERATED_BODY()

public:

	AT_EnemyCharacter();
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	virtual UAttributeSet* GetAttributeSet() const override;
	
	// 移动半径范围
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Crash|AI")
	float AcceptanceRadius{500.0f};

	// 最短攻击间隔
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Crash|AI")
	float MinAttackDelay{0.1f};

	// 最长攻击间隔
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Crash|AI")
	float MaxAttackDelay{0.5f};
	
	// 搜索范围
	UPROPERTY(EditAnywhere, Category = "Crash|AI")
	float SearchRange = 1000.0f;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Crash|UI")
	TObjectPtr<UT_ActorWidgetComponent> LockOnWidget;
	
	UFUNCTION(BlueprintImplementableEvent)
	float GetTimelineLength();
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated)
	bool bIsBeingLaunched = false;

	void StopMovementUntilLanded();
protected:

	virtual void BeginPlay() override;
	virtual void HandleDeath() override;
	
private:

	UFUNCTION()
	void EnableMovementOnLanded(const FHitResult& Hit);
	
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<UAttributeSet> AttributeSet;
};
