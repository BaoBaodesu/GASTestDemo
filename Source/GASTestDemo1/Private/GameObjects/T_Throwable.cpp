#include "GameObjects/T_Throwable.h"

#include "AI/T_GuardAlertSubsystem.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Sound/SoundBase.h"

AT_Throwable::AT_Throwable()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	bReplicates = true;
	SetReplicateMovement(true);
	InitialLifeSpan = 15.f;

	SphereCollision = CreateDefaultSubobject<USphereComponent>(TEXT("SphereCollision"));
	SetRootComponent(SphereCollision);
	SphereCollision->InitSphereRadius(15.f);
	SphereCollision->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	SphereCollision->SetCollisionObjectType(ECC_WorldDynamic);
	SphereCollision->SetCollisionResponseToAllChannels(ECR_Block);
	SphereCollision->SetNotifyRigidBodyCollision(true);
	SphereCollision->SetCanEverAffectNavigation(false);

	StaticMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMesh"));
	StaticMesh->SetupAttachment(SphereCollision);
	StaticMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	StaticMesh->SetCanEverAffectNavigation(false);

	TrailComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("TrailComponent"));
	TrailComponent->SetupAttachment(SphereCollision);
	TrailComponent->SetAutoActivate(false);

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->UpdatedComponent = SphereCollision;
	ProjectileMovement->InitialSpeed = 1200.f;
	ProjectileMovement->MaxSpeed = 1600.f;
	ProjectileMovement->ProjectileGravityScale = 1.f;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->bShouldBounce = true;
	ProjectileMovement->Bounciness = 0.35f;
	ProjectileMovement->Friction = 0.5f;
	ProjectileMovement->BounceVelocityStopSimulatingThreshold = 40.f;
	ProjectileMovement->bForceSubStepping = true;
	ProjectileMovement->bAutoActivate = false;
}

void AT_Throwable::BeginPlay()
{
	Super::BeginPlay();

	SphereCollision->OnComponentHit.AddDynamic(this, &AT_Throwable::HandleSphereHit);
	ProjectileMovement->OnProjectileStop.AddDynamic(this, &AT_Throwable::HandleProjectileStop);
	if (AActor* OwnerActor = GetOwner())
	{
		SphereCollision->IgnoreActorWhenMoving(OwnerActor, true);
	}
	if (APawn* InstigatorPawn = GetInstigator())
	{
		SphereCollision->IgnoreActorWhenMoving(InstigatorPawn, true);
	}
}

void AT_Throwable::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!HasAuthority() || MaxThrowDistance <= 0.f || !ProjectileMovement->IsActive()) return;
	if (FVector::DistSquared(GetActorLocation(), LaunchLocation) < FMath::Square(MaxThrowDistance)) return;

	SetTrailActive(false);
	Destroy();
}

void AT_Throwable::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (IsValid(TrailComponent)) TrailComponent->DeactivateImmediate();
	Super::EndPlay(EndPlayReason);
}

void AT_Throwable::LaunchThrowable(FVector Direction, float Speed)
{
	if (!HasAuthority() || Direction.IsNearlyZero()) return;

	const float LaunchSpeed = GetLaunchSpeed(Speed);
	if (LaunchSpeed <= KINDA_SMALL_NUMBER) return;
	LaunchLocation = GetActorLocation();
	ProjectileMovement->Velocity = Direction.GetSafeNormal() * LaunchSpeed;
	ProjectileMovement->Activate(true);
	SetActorTickEnabled(MaxThrowDistance > 0.f);
	SetTrailActive(true);
	ForceNetUpdate();
}

float AT_Throwable::GetPredictionCollisionRadius() const
{
	return IsValid(SphereCollision) ? SphereCollision->GetScaledSphereRadius() : 0.f;
}

float AT_Throwable::GetPredictionGravityScale() const
{
	return IsValid(ProjectileMovement) ? ProjectileMovement->ProjectileGravityScale : 1.f;
}

float AT_Throwable::GetLaunchSpeed(float SpeedOverride) const
{
	if (!IsValid(ProjectileMovement)) return 0.f;

	const float DesiredSpeed = FMath::Max(SpeedOverride > 0.f ? SpeedOverride : ProjectileMovement->InitialSpeed, 0.f);
	return ProjectileMovement->MaxSpeed > 0.f
		? FMath::Min(DesiredSpeed, ProjectileMovement->MaxSpeed)
		: DesiredSpeed;
}

void AT_Throwable::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AT_Throwable, bTrailActive);
}

bool AT_Throwable::IsImpactSignificant(
	float ImpactSpeed,
	float MinimumImpactSpeed,
	float CurrentTime,
	float LastImpactTime,
	float ImpactCooldown)
{
	return ImpactSpeed >= FMath::Max(MinimumImpactSpeed, 0.f)
		&& CurrentTime - LastImpactTime >= FMath::Max(ImpactCooldown, 0.f);
}

void AT_Throwable::HandleProjectileStop(const FHitResult& ImpactResult)
{
	if (HasAuthority()) SetTrailActive(false);
}

void AT_Throwable::SetTrailActive(bool bActive)
{
	if (bTrailActive == bActive) return;

	bTrailActive = bActive;
	OnRep_TrailActive();
	if (HasAuthority()) ForceNetUpdate();
}

void AT_Throwable::OnRep_TrailActive()
{
	if (GetNetMode() == NM_DedicatedServer || !IsValid(TrailComponent)) return;

	if (bTrailActive && IsValid(TrailSystem))
	{
		TrailComponent->SetAsset(TrailSystem);
		TrailComponent->Activate(true);
		return;
	}

	TrailComponent->DeactivateImmediate();
}

void AT_Throwable::HandleSphereHit(
	UPrimitiveComponent* HitComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComponent,
	FVector NormalImpulse,
	const FHitResult& Hit)
{
	if (!HasAuthority() || bImpactHandled || OtherActor == GetOwner() || OtherActor == GetInstigator()) return;

	const float ImpactSpeed = ProjectileMovement->Velocity.Size();
	const float CurrentTime = GetWorld()->GetTimeSeconds();
	if (!IsImpactSignificant(ImpactSpeed, MinimumImpactSpeed, CurrentTime, LastImpactTime, ImpactCooldown)) return;
	LastImpactTime = CurrentTime;

	if (bDestroyOnImpact)
	{
		bImpactHandled = true;
		MulticastBreakEffects(Hit.ImpactPoint, Hit.ImpactNormal, ImpactSpeed);
	}
	else
	{
		MulticastImpactEffects(Hit.ImpactPoint, Hit.ImpactNormal, ImpactSpeed);
	}

	if (bReportImpactNoiseToAI && (!bOnlyReportFirstImpactToAI || !bHasReportedImpactNoise))
	{
		if (UT_GuardAlertSubsystem* AlertSubsystem = GetWorld()->GetSubsystem<UT_GuardAlertSubsystem>())
		{
			AActor* NoiseInstigator = GetInstigator() ? static_cast<AActor*>(GetInstigator()) : GetOwner();
			AlertSubsystem->ReportThrowableImpactNoise(
				Hit.ImpactPoint,
				NoiseInstigator,
				ImpactNoiseLoudness,
				ImpactNoiseMaxRange);
			bHasReportedImpactNoise = true;
		}
	}

	if (bDestroyOnImpact)
	{
		SetTrailActive(false);
		SphereCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		ProjectileMovement->StopMovementImmediately();
		ProjectileMovement->Deactivate();
		SetLifeSpan(FMath::Max(DestroyDelayAfterImpact, 0.01f));
	}
}

void AT_Throwable::MulticastImpactEffects_Implementation(
	FVector ImpactLocation,
	FVector ImpactNormal,
	float ImpactSpeed)
{
	PlayImpactEffects(ImpactLocation, ImpactNormal, ImpactSpeed);
}

void AT_Throwable::MulticastBreakEffects_Implementation(
	FVector ImpactLocation,
	FVector ImpactNormal,
	float ImpactSpeed)
{
	PlayImpactEffects(ImpactLocation, ImpactNormal, ImpactSpeed);
}

void AT_Throwable::PlayImpactEffects(
	FVector ImpactLocation,
	FVector ImpactNormal,
	float ImpactSpeed)
{
	if (GetNetMode() != NM_DedicatedServer)
	{
		if (IsValid(ImpactSound))
		{
			UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, ImpactLocation, ImpactSoundVolume);
		}
		if (IsValid(ImpactSystem))
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(
				this,
				ImpactSystem,
				ImpactLocation,
				ImpactNormal.Rotation(),
				FVector::OneVector,
				true,
				true,
				ENCPoolMethod::AutoRelease,
				true);
		}
	}
	if (bDestroyOnImpact && IsValid(TrailComponent)) TrailComponent->DeactivateImmediate();
	if (bHideMeshOnImpact && IsValid(StaticMesh))
	{
		StaticMesh->SetVisibility(false, true);
	}
	OnThrowableImpact(ImpactLocation, ImpactNormal, ImpactSpeed);
}
