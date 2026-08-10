#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "GameplayTagContainer.h"
#include "AnimNotify_ContextualExecution.generated.h"

class UGameplayEffect;

UCLASS(meta = (DisplayName = "Contextual Execution"))
class GASTESTDEMO1_API UAnimNotify_ContextualExecution : public UAnimNotify
{
	GENERATED_BODY()

public:
	UAnimNotify_ContextualExecution();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Execution")
	FName TargetRole = TEXT("Victim");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Execution", meta = (ClampMin = "0.0"))
	float Damage = 999.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Execution")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Execution")
	FGameplayTag SetByCallerDataTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Execution")
	bool bSkipDeathAnimation = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Execution", meta = (ClampMin = "0.0"))
	float DestroyDelay = 0.2f;

protected:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

private:
	AActor* ResolveExecutionTarget(AActor* Owner) const;
	AActor* ResolveInstigator(AActor* Owner, AActor* TargetActor) const;
};
