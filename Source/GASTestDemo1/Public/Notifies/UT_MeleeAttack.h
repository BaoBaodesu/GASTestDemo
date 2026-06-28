// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "UT_MeleeAttack.generated.h"

/**
 * 
 */
class USkeletalMeshComponent;
UCLASS()
class GASTESTDEMO1_API UT_MeleeAttack : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference) override;
	
protected:
	// 是否绘制调试线
	UPROPERTY(EditAnywhere, Category = "Crash|Debug")
	bool bDrawDebug = true;
	
	// 攻击检测使用的插槽名
	UPROPERTY(EditAnywhere, Category = "Crash|Socket")
	FName SocketName;
	
	// 球形检测向前延伸的距离
	UPROPERTY(EditAnywhere, Category = "Crash|Socket")
	float SocketExtensionOffset = 100.0f;
	
	// 球形检测半径
	UPROPERTY(EditAnywhere, Category = "Crash|Socket")
	float SphereTraceRadius = 30.0f;
	// 是否绘制调试线

private:
	TArray<FHitResult> PerformSphereTrace(USkeletalMeshComponent* MeshComp) const;
	
	void SendEventsToActors(const TArray<FHitResult>& Hits, USkeletalMeshComponent* MeshComp) const;
};
