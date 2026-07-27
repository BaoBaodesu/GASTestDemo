#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Player/Components/Test2Component.h"

namespace
{
	struct FTest2ShimmyTestWorld
	{
		FTest2ShimmyTestWorld()
		{
			World = UWorld::CreateWorld(
				EWorldType::Game,
				false,
				TEXT("Test2ShimmyTestWorld"));

			FWorldContext& WorldContext =
				GEngine->CreateNewWorldContext(EWorldType::Game);
			WorldContext.SetCurrentWorld(World);

			Character = World->SpawnActor<ACharacter>();
			PlayerController = World->SpawnActor<APlayerController>();
			PlayerController->Possess(Character);
			World->BeginPlay();

			Component = NewObject<UTest2Component>(Character);
			Character->AddInstanceComponent(Component);
			Component->RegisterComponent();
			CharacterMovement = Character->GetCharacterMovement();
			Component->Character = Character;
			Component->CharacterMovement = CharacterMovement;
			Component->CapsuleComponent = Character->GetCapsuleComponent();
			Component->MeshComponent = Character->GetMesh();
		}

		~FTest2ShimmyTestWorld()
		{
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(false);
		}

		void AddPositiveShimmySurface(
			const double HeightOffset = 145.0) const
		{
			AActor* SurfaceActor = World->SpawnActor<AActor>();
			UBoxComponent* SurfaceComponent =
				NewObject<UBoxComponent>(SurfaceActor);
			SurfaceActor->SetRootComponent(SurfaceComponent);
			SurfaceActor->AddInstanceComponent(SurfaceComponent);
			SurfaceComponent->SetBoxExtent(FVector(5.0, 20.0, 20.0));
			SurfaceComponent->SetCollisionEnabled(
				ECollisionEnabled::QueryOnly);
			SurfaceComponent->SetCollisionResponseToAllChannels(ECR_Block);
			SurfaceComponent->RegisterComponent();
			SurfaceActor->SetActorLocation(
				Character->GetActorLocation() +
				Character->GetActorRightVector() * 40.0 +
				Character->GetActorForwardVector() * 20.0 +
				FVector(0.0, 0.0, HeightOffset));
		}

		UWorld* World = nullptr;
		ACharacter* Character = nullptr;
		APlayerController* PlayerController = nullptr;
		UTest2Component* Component = nullptr;
		UCharacterMovementComponent* CharacterMovement = nullptr;
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTest2BarGrabMovementStateTest,
	"GASTestDemo1.Test2.Shimmy.BarGrabLocksHeightAndFacing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTest2BarGrabMovementStateTest::RunTest(const FString& Parameters)
{
	FTest2ShimmyTestWorld TestWorld;
	TestWorld.Component->GrabType = FName(TEXT("Bar"));
	TestWorld.CharacterMovement->bOrientRotationToMovement = true;
	TestWorld.CharacterMovement->bUseControllerDesiredRotation = true;

	TestWorld.Component->AlignGrab();

	TestEqual(
		TEXT("Bar 悬挂使用 Flying 处理无地面横移"),
		TestWorld.CharacterMovement->MovementMode,
		MOVE_Flying);
	TestEqual(
		TEXT("Bar 悬挂使用设定的横移速度"),
		TestWorld.CharacterMovement->GetMaxSpeed(),
		70.0f);
	TestFalse(
		TEXT("进入 Bar 悬挂时立即关闭朝移动方向旋转"),
		TestWorld.CharacterMovement->bOrientRotationToMovement);
	TestFalse(
		TEXT("进入 Bar 悬挂时立即关闭控制器期望旋转"),
		TestWorld.CharacterMovement->bUseControllerDesiredRotation);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTest2BarGrabConstrainsMovementHeightTest,
	"GASTestDemo1.Test2.Shimmy.ConstrainsMovementToGrabHeight",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTest2BarGrabConstrainsMovementHeightTest::RunTest(
	const FString& Parameters)
{
	FTest2ShimmyTestWorld TestWorld;
	TestWorld.Component->GrabType = FName(TEXT("Bar"));
	TestWorld.Component->TopImpactPoint =
		FVector(0.0, 0.0, 300.0);

	TestWorld.Component->AlignGrab();

	TestEqual(
		TEXT("Bar 横移始终约束在首次抓取高度"),
		TestWorld.CharacterMovement->ConstrainLocationToPlane(
			FVector(100.0, 100.0, 500.0)).Z,
		154.0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTest2ShimmyUsesBarDirectionTest,
	"GASTestDemo1.Test2.Shimmy.UsesBarDirection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTest2ShimmyUsesBarDirectionTest::RunTest(const FString& Parameters)
{
	FTest2ShimmyTestWorld TestWorld;
	TestWorld.Component->BarMoveDirection = FVector::YAxisVector * 2.0;
	TestWorld.Component->WallNormal2 = -FVector::XAxisVector;
	TestWorld.Component->GrabType = FName(TEXT("Bar"));
	TestWorld.Component->TopImpactPoint = FVector(0.0, 0.0, 155.0);
	TestWorld.Component->AlignGrab();
	TestWorld.World->GetTimerManager().ClearTimer(
		TestWorld.Component->TransitionTimerHandle);
	TestWorld.Component->bCanMove = true;
	TestWorld.AddPositiveShimmySurface();

	TestWorld.Component->Shimmy(1.0);

	TestTrue(
		TEXT("Bar 状态沿横杆切线方向提交移动输入"),
		TestWorld.Character->GetPendingMovementInputVector().Equals(
			FVector::YAxisVector,
			UE_KINDA_SMALL_NUMBER));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTest2ShimmyMovesWhileSuspendedTest,
	"GASTestDemo1.Test2.Shimmy.MovesWhileSuspended",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTest2ShimmyMovesWhileSuspendedTest::RunTest(
	const FString& Parameters)
{
	FTest2ShimmyTestWorld TestWorld;
	TestWorld.Component->BarMoveDirection = FVector::YAxisVector;
	TestWorld.Component->GrabType = FName(TEXT("Bar"));
	TestWorld.Component->TopImpactPoint = FVector(0.0, 0.0, 300.0);
	TestWorld.Character->SetActorLocation(FVector(0.0, 0.0, 160.0));
	TestWorld.Component->AlignGrab();
	TestWorld.World->GetTimerManager().ClearTimer(
		TestWorld.Component->TransitionTimerHandle);

	TestNotNull(
		TEXT("测试角色由 PlayerController 控制"),
		TestWorld.Character->GetController());
	TestWorld.PlayerController->UnPossess();
	TestWorld.CharacterMovement->bRunPhysicsWithNoController = true;
	TestWorld.CharacterMovement->Activate(true);
	TestWorld.AddPositiveShimmySurface();

	TestWorld.Component->Shimmy(1.0);
	TestTrue(
		TEXT("Shimmy 输入进入 CharacterMovement"),
		TestWorld.Character->GetPendingMovementInputVector().Y > 0.0);
	TestEqual(
		TEXT("Bar 悬空状态使用 Flying"),
		TestWorld.CharacterMovement->MovementMode,
		MOVE_Flying);
	TestWorld.CharacterMovement->TickComponent(
		0.1f,
		LEVELTICK_All,
		nullptr);
	TestTrue(
		TEXT("CharacterMovement Tick 后产生横向速度"),
		TestWorld.CharacterMovement->Velocity.Y > 0.0);

	TestTrue(
		TEXT("Bar 悬空状态下横移输入应实际改变角色位置"),
		TestWorld.Character->GetActorLocation().Y > 0.0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTest2ShimmyStopsWhenInputReleasedTest,
	"GASTestDemo1.Test2.Shimmy.StopsWhenInputReleased",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTest2ShimmyStopsWhenInputReleasedTest::RunTest(
	const FString& Parameters)
{
	FTest2ShimmyTestWorld TestWorld;
	TestWorld.Component->BarMoveDirection = FVector::YAxisVector;
	TestWorld.Component->GrabType = FName(TEXT("Bar"));
	TestWorld.Component->TopImpactPoint = FVector(0.0, 0.0, 300.0);
	TestWorld.Character->SetActorLocation(FVector(0.0, 0.0, 160.0));
	TestWorld.Component->AlignGrab();
	TestWorld.World->GetTimerManager().ClearTimer(
		TestWorld.Component->TransitionTimerHandle);
	TestWorld.PlayerController->UnPossess();
	TestWorld.CharacterMovement->bRunPhysicsWithNoController = true;
	TestWorld.CharacterMovement->Activate(true);
	TestWorld.AddPositiveShimmySurface();

	TestWorld.Component->Shimmy(1.0);
	TestWorld.CharacterMovement->TickComponent(
		0.1f,
		LEVELTICK_All,
		nullptr);
	TestTrue(
		TEXT("松开按键前角色已有横移速度"),
		TestWorld.CharacterMovement->Velocity.Y > 0.0);

	TestWorld.Component->Shimmy(0.0);

	TestTrue(
		TEXT("松开 Bar 横移按键后立即停止"),
		TestWorld.CharacterMovement->Velocity.IsNearlyZero());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTest2ShimmyStopsImmediatelyAtBarEdgeTest,
	"GASTestDemo1.Test2.Shimmy.StopsImmediatelyAtBarEdge",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTest2ShimmyStopsImmediatelyAtBarEdgeTest::RunTest(
	const FString& Parameters)
{
	FTest2ShimmyTestWorld TestWorld;
	TestWorld.Component->BarMoveDirection = FVector::YAxisVector;
	TestWorld.Component->GrabType = FName(TEXT("Bar"));
	TestWorld.Component->TopImpactPoint = FVector(0.0, 0.0, 300.0);
	TestWorld.Component->AlignGrab();
	TestWorld.World->GetTimerManager().ClearTimer(
		TestWorld.Component->TransitionTimerHandle);
	TestWorld.CharacterMovement->Velocity = FVector::YAxisVector * 70.0f;

	TestWorld.Component->Shimmy(1.0);

	TestTrue(
		TEXT("目标方向没有可抓取表面时立即停止横移"),
		TestWorld.CharacterMovement->Velocity.IsNearlyZero());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTest2WallShimmyUsesGrabHeightTest,
	"GASTestDemo1.Test2.Shimmy.WallUsesGrabHeight",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTest2WallShimmyUsesGrabHeightTest::RunTest(
	const FString& Parameters)
{
	FTest2ShimmyTestWorld TestWorld;
	TestWorld.Component->BarMoveDirection = FVector::YAxisVector;
	TestWorld.Component->TopImpactPoint = FVector(0.0, 0.0, 53.0);
	TestWorld.Component->bGrabbed = true;
	TestWorld.Component->bCanMove = true;
	TestWorld.Component->bOnBar = false;
	TestWorld.AddPositiveShimmySurface(53.0);

	TestWorld.Component->Shimmy(1.0);

	TestTrue(
		TEXT("Wall 横移使用抓取高度检测侧方墙面"),
		TestWorld.Character->GetPendingMovementInputVector().Y > 0.0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTest2WallGrabUsesWallFaceNormalTest,
	"GASTestDemo1.Test2.Grab.UsesWallFaceNormal",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTest2WallGrabUsesWallFaceNormalTest::RunTest(const FString& Parameters)
{
	FTest2ShimmyTestWorld TestWorld;
	TestWorld.CharacterMovement->Velocity = FVector(0.0, 0.0, -100.0);

	AActor* WallActor = TestWorld.World->SpawnActor<AActor>();
	UBoxComponent* WallComponent = NewObject<UBoxComponent>(WallActor);
	WallActor->SetRootComponent(WallComponent);
	WallActor->AddInstanceComponent(WallComponent);
	WallComponent->SetBoxExtent(FVector(10.0, 200.0, 110.0));
	WallComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	WallComponent->SetCollisionObjectType(ECC_WorldStatic);
	WallComponent->SetCollisionResponseToAllChannels(ECR_Block);
	WallComponent->RegisterComponent();
	WallActor->SetActorLocation(FVector(55.0, 0.0, -50.0));

	TestWorld.Component->GrabTrace();

	TestTrue(
		TEXT("墙沿检测完成后使用墙身朝向角色的水平法线"),
		TestWorld.Component->WallNormal2.Equals(
			-FVector::XAxisVector,
			UE_KINDA_SMALL_NUMBER));
	return true;
}

#endif
