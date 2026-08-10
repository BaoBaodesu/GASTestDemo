#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "AnimNotifyState_ComboHit.generated.h"

class USkeletalMeshComponent;
class UT_PrimaryComboAbility;

USTRUCT()
struct FComboHitWindowState
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FVector> PrevSocketLocations;

	UPROPERTY()
	TArray<TWeakObjectPtr<AActor>> ActorsAlreadyHit;
};

/**
 * 玩家连击命中窗口：按 Socket 做帧间球形 Sweep，窗口内同一目标只结算一次。
 * 与 ComboWindow（输入窗口）分离，在各段攻击蒙太奇上单独配置检测骨骼。
 */
UCLASS(meta = (DisplayName = "Combo Hit"))
class GASTESTDEMO1_API UAnimNotifyState_ComboHit : public UAnimNotifyState
{
	GENERATED_BODY()

public:

	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

protected:

	// 命中检测 Socket 列表（每段蒙太奇 Notify 实例各自配置，如 hand_r / hand_l / foot_r）
	UPROPERTY(EditAnywhere, Category = "Combo|Hit")
	TArray<FName> SocketNames;

	// 球形检测半径
	UPROPERTY(EditAnywhere, Category = "Combo|Hit", meta = (ClampMin = "1.0"))
	float SphereRadius{30.f};

	// Socket 局部空间偏移（加在 Socket 变换之后）
	UPROPERTY(EditAnywhere, Category = "Combo|Hit")
	FVector SocketLocationOffset{FVector::ZeroVector};

	UPROPERTY(EditAnywhere, Category = "Combo|Debug")
	bool bDrawDebug{false};

private:

	FVector GetSocketWorldLocation(const USkeletalMeshComponent* MeshComp, FName SocketName) const;
	void CollectSocketLocations(const USkeletalMeshComponent* MeshComp, TArray<FVector>& OutLocations) const;
	UT_PrimaryComboAbility* FindActiveComboAbility(AActor* Owner) const;
	void ProcessHitsForSocket(USkeletalMeshComponent* MeshComp, const FVector& Start, const FVector& End, FComboHitWindowState& WindowState);

	// 按 Mesh 区分窗口状态，避免 Notify CDO 共享导致串状态
	TMap<TWeakObjectPtr<USkeletalMeshComponent>, FComboHitWindowState> ActiveWindows;
};
