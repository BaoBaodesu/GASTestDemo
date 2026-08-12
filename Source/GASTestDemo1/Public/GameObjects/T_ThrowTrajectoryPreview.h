#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "T_ThrowTrajectoryPreview.generated.h"

class UInstancedStaticMeshComponent;
class USceneComponent;
class UStaticMeshComponent;
struct FPredictProjectilePathResult;

UCLASS()
class GASTESTDEMO1_API AT_ThrowTrajectoryPreview : public AActor
{
	GENERATED_BODY()

public:
	AT_ThrowTrajectoryPreview();

	void UpdatePath(const FPredictProjectilePathResult& PathResult);

private:
	UPROPERTY(VisibleAnywhere, Category = "Throw|Preview")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, Category = "Throw|Preview")
	TObjectPtr<UInstancedStaticMeshComponent> PathPoints;

	UPROPERTY(VisibleAnywhere, Category = "Throw|Preview")
	TObjectPtr<UStaticMeshComponent> LandingMarker;
};
