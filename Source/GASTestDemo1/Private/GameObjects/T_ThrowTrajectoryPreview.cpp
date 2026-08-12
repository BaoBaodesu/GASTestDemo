#include "GameObjects/T_ThrowTrajectoryPreview.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStaticsTypes.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

AT_ThrowTrajectoryPreview::AT_ThrowTrajectoryPreview()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;
	SetCanBeDamaged(false);

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	PathPoints = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("PathPoints"));
	PathPoints->SetupAttachment(SceneRoot);
	PathPoints->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PathPoints->SetCanEverAffectNavigation(false);
	PathPoints->SetCastShadow(false);

	LandingMarker = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LandingMarker"));
	LandingMarker->SetupAttachment(SceneRoot);
	LandingMarker->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	LandingMarker->SetCanEverAffectNavigation(false);
	LandingMarker->SetCastShadow(false);
	LandingMarker->SetVisibility(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> PathPointMeshAsset(
		TEXT("/Game/ParagonMinions/FX/Meshes/Shapes/SM_Sphere.SM_Sphere"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> LandingMarkerMeshAsset(
		TEXT("/Game/ParagonMinions/FX/Meshes/Shapes/SM_1MeterRing.SM_1MeterRing"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> PreviewMaterialAsset(
		TEXT("/Game/GASTestDemo/FX/Throw/M_ThrowPreview.M_ThrowPreview"));

	PathPoints->SetStaticMesh(PathPointMeshAsset.Object);
	LandingMarker->SetStaticMesh(LandingMarkerMeshAsset.Object);
	if (IsValid(PreviewMaterialAsset.Object))
	{
		PathPoints->SetMaterial(0, PreviewMaterialAsset.Object);
		LandingMarker->SetMaterial(0, PreviewMaterialAsset.Object);
	}
}

void AT_ThrowTrajectoryPreview::UpdatePath(const FPredictProjectilePathResult& PathResult)
{
	PathPoints->ClearInstances();
	for (int32 PointIndex = 1; PointIndex < PathResult.PathData.Num(); ++PointIndex)
	{
		PathPoints->AddInstance(
			FTransform(FRotator::ZeroRotator, PathResult.PathData[PointIndex].Location, FVector(0.06f)),
			true);
	}

	if (!PathResult.HitResult.bBlockingHit)
	{
		LandingMarker->SetVisibility(false);
		return;
	}

	const FVector ImpactNormal = PathResult.HitResult.ImpactNormal.GetSafeNormal();
	LandingMarker->SetWorldLocationAndRotation(
		PathResult.HitResult.ImpactPoint + ImpactNormal * 2.f,
		FRotationMatrix::MakeFromZ(ImpactNormal).Rotator());
	LandingMarker->SetVisibility(true);
}
