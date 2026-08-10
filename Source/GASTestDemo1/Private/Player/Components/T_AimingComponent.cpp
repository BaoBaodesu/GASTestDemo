#include "Player/Components/T_AimingComponent.h"

#include "Animation/AnimInstance.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Camera/CameraComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameplayTags/TTags.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "GameObjects/T_PickUpItems.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

UT_AimingComponent::UT_AimingComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	static ConstructorHelpers::FClassFinder<UUserWidget> CrosshairWidgetClassFinder(TEXT("/Game/GASTestDemo/UI/WeaponState/WBP_ThirdPerson_Crosshair1"));
	if (CrosshairWidgetClassFinder.Succeeded()) CrosshairWidgetClass = CrosshairWidgetClassFinder.Class;
}

void UT_AimingComponent::BeginPlay()
{
	Super::BeginPlay();

	Character = Cast<ACharacter>(GetOwner());
	if (!IsValid(Character)) { UE_LOG(LogTemp, Error, TEXT("T_AimingComponent requires an ACharacter owner.")); SetComponentTickEnabled(false); return; }

	MovementComponent = Character->GetCharacterMovement();
	CameraBoom = Character->FindComponentByClass<USpringArmComponent>();
	FollowCamera = Character->FindComponentByClass<UCameraComponent>();
	AnimInstance = IsValid(Character->GetMesh()) ? Character->GetMesh()->GetAnimInstance() : nullptr;
	if (!IsValid(MovementComponent) || !IsValid(CameraBoom) || !IsValid(FollowCamera)) { UE_LOG(LogTemp, Error, TEXT("T_AimingComponent could not find CharacterMovement, SpringArm or CameraComponent.")); SetComponentTickEnabled(false); return; }

	NormalArmLength = CameraBoom->TargetArmLength;
	NormalSocketOffset = CameraBoom->SocketOffset;
	NormalFOV = FollowCamera->FieldOfView;
	GetWorld()->GetTimerManager().SetTimerForNextTick(this, &ThisClass::HideCrosshair);
}

void UT_AimingComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopAiming();
	Super::EndPlay(EndPlayReason);
}

void UT_AimingComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!IsValid(Character) || !IsValid(CameraBoom) || !IsValid(FollowCamera)) return;
	if (bAiming)
	{
		UAbilitySystemComponent* AbilitySystemComponent = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Character);
		if (IsValid(AbilitySystemComponent) && !AbilitySystemComponent->HasMatchingGameplayTag(TTags::State::Aiming)) { StopAiming(); return; }
	}

	CameraBoom->TargetArmLength = FMath::FInterpTo(CameraBoom->TargetArmLength, bAiming ? AimingArmLength : NormalArmLength, DeltaTime, CameraInterpSpeed);
	CameraBoom->SocketOffset = FMath::VInterpTo(CameraBoom->SocketOffset, bAiming ? AimingSocketOffset : NormalSocketOffset, DeltaTime, CameraInterpSpeed);
	FollowCamera->SetFieldOfView(FMath::FInterpTo(FollowCamera->FieldOfView, bAiming ? AimingFOV : NormalFOV, DeltaTime, CameraInterpSpeed));
	if (!bAiming || !bRotateCharacterWhileAiming || !IsValid(MovementComponent)) return;
	if (MovementComponent->MovementMode != MOVE_Walking && MovementComponent->MovementMode != MOVE_NavWalking) return;
	if (IsValid(AnimInstance) && AnimInstance->IsAnyMontagePlaying()) return;

	AController* Controller = Character->GetController();
	if (!IsValid(Controller)) return;
	Character->SetActorRotation(FMath::RInterpTo(Character->GetActorRotation(), FRotator(0.f, Controller->GetControlRotation().Yaw + AimingYawOffset, 0.f), DeltaTime, CharacterRotationInterpSpeed));
}

void UT_AimingComponent::StartAiming()
{
	if (bAiming || !IsValid(Character) || !IsValid(MovementComponent)) return;
	bCachedOrientRotationToMovement = MovementComponent->bOrientRotationToMovement;
	bCachedUseControllerDesiredRotation = MovementComponent->bUseControllerDesiredRotation;
	bCachedUseControllerRotationYaw = Character->bUseControllerRotationYaw;
	CachedMaxWalkSpeed = MovementComponent->MaxWalkSpeed;
	bHasCachedMovementSettings = true;
	bAiming = true;
	UpdateAnimationState();
	MovementComponent->bOrientRotationToMovement = false;
	MovementComponent->bUseControllerDesiredRotation = false;
	MovementComponent->MaxWalkSpeed = AimingMaxWalkSpeed;
	Character->bUseControllerRotationYaw = false;
	GetWorld()->GetTimerManager().SetTimerForNextTick(this, &ThisClass::ShowCrosshair);
}

void UT_AimingComponent::StopAiming()
{
	if (!bAiming && !bHasCachedMovementSettings) return;
	bAiming = false;
	UpdateAnimationState();
	HideCrosshair();
	RestoreMovementSettings();
}

void UT_AimingComponent::UpdateAnimationState()
{
	if (!IsValid(Character)) return;

	if (UFunction* AnimationFunction = Character->FindFunction(bAiming ? TEXT("AimFoward") : TEXT("StopAimingFoward"))) Character->ProcessEvent(AnimationFunction, nullptr);
}

void UT_AimingComponent::ShowCrosshair()
{
	if (!bAiming || !IsValid(Character) || !Character->IsLocallyControlled() || !CrosshairWidgetClass) return;

	TArray<UUserWidget*> CrosshairWidgets;
	UWidgetBlueprintLibrary::GetAllWidgetsOfClass(this, CrosshairWidgets, CrosshairWidgetClass, false);
	CrosshairWidget = CrosshairWidgets.IsEmpty() ? CreateWidget<UUserWidget>(Cast<APlayerController>(Character->GetController()), CrosshairWidgetClass) : CrosshairWidgets[0];
	if (!IsValid(CrosshairWidget)) return;

	CrosshairWidget->SetVisibility(ESlateVisibility::Visible);
	if (!CrosshairWidget->IsInViewport()) CrosshairWidget->AddToViewport();
}

void UT_AimingComponent::HideCrosshair()
{
	if (bAiming || !CrosshairWidgetClass) return;

	TArray<UUserWidget*> CrosshairWidgets;
	UWidgetBlueprintLibrary::GetAllWidgetsOfClass(this, CrosshairWidgets, CrosshairWidgetClass, false);
	for (UUserWidget* Widget : CrosshairWidgets)
	{
		if (IsValid(Widget)) Widget->RemoveFromParent();
	}
	CrosshairWidget = nullptr;
}

void UT_AimingComponent::RestoreMovementSettings()
{
	if (!bHasCachedMovementSettings || !IsValid(Character) || !IsValid(MovementComponent)) return;
	MovementComponent->bOrientRotationToMovement = bCachedOrientRotationToMovement;
	MovementComponent->bUseControllerDesiredRotation = bCachedUseControllerDesiredRotation;
	MovementComponent->MaxWalkSpeed = CachedMaxWalkSpeed;
	Character->bUseControllerRotationYaw = bCachedUseControllerRotationYaw;
	bHasCachedMovementSettings = false;
}

bool UT_AimingComponent::GetCameraAimPoint(FVector& OutAimPoint, FHitResult& OutCameraHit) const
{
	OutAimPoint = FVector::ZeroVector;
	OutCameraHit = FHitResult();
	if (!IsValid(Character)) return false;

	APlayerController* PlayerController = Cast<APlayerController>(Character->GetController());
	UWorld* World = GetWorld();
	if (!IsValid(PlayerController) || !IsValid(World)) return false;

	int32 ViewportSizeX = 0;
	int32 ViewportSizeY = 0;
	PlayerController->GetViewportSize(ViewportSizeX, ViewportSizeY);
	if (ViewportSizeX <= 0 || ViewportSizeY <= 0) return false;

	FVector WorldLocation;
	FVector WorldDirection;
	if (!PlayerController->DeprojectScreenPositionToWorld(ViewportSizeX * 0.5f, ViewportSizeY * 0.5f, WorldLocation, WorldDirection)) return false;

	WorldDirection = WorldDirection.GetSafeNormal();
	const FVector TraceStart = WorldLocation + WorldDirection * CameraTraceStartOffset;
	const FVector TraceEnd = TraceStart + WorldDirection * TraceDistance;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(TAimingCameraTrace), true, Character);
	bool bHit = false;
	for (int32 TraceIndex = 0; TraceIndex < 16; ++TraceIndex)
	{
		FHitResult CandidateHit;
		if (!World->LineTraceSingleByChannel(CandidateHit, TraceStart, TraceEnd, TraceChannel, QueryParams)) break;

		const AT_PickUpItems* PickUpItem = Cast<AT_PickUpItems>(CandidateHit.GetActor());
		if (IsValid(PickUpItem) && !PickUpItem->IsPhysicalCollisionComponent(CandidateHit.GetComponent()))
		{
			QueryParams.AddIgnoredComponent(CandidateHit.GetComponent());
			continue;
		}

		OutCameraHit = CandidateHit;
		bHit = true;
		break;
	}
	OutAimPoint = bHit ? OutCameraHit.ImpactPoint : TraceEnd;
	if (bDrawDebug) DrawDebugLine(World, TraceStart, OutAimPoint, bHit ? FColor::Cyan : FColor::Blue, false, DebugDrawTime, 0, 1.f);
	return true;
}
