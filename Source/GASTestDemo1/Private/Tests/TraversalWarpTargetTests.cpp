#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/Abilities/T_Traversal.h"
#include "Characters/T_PlayerCharacter.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Player/Components/T_TraversalComponent.h"
#include "UObject/UnrealType.h"

namespace
{
	struct FTraversalWarpTargetTestWorld
	{
		FTraversalWarpTargetTestWorld()
		{
			World = UWorld::CreateWorld(EWorldType::Game, false, TEXT("TraversalWarpTargetTestWorld"));
			FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
			WorldContext.SetCurrentWorld(World);

			Character = World->SpawnActor<ACharacter>();
			Component = NewObject<UT_TraversalComponent>(Character);
			Character->AddInstanceComponent(Component);
			Component->RegisterComponent();
			World->BeginPlay();
			Character->SetActorLocation(FVector(0.f, 0.f, 100.f));
			FTraversalCheckResult CacheResult;
			Component->DetectTraversal(CacheResult);
		}

		~FTraversalWarpTargetTestWorld()
		{
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(false);
		}

		bool SetVectorProperty(const TCHAR* PropertyName, const FVector& Value) const
		{
			FStructProperty* Property = FindFProperty<FStructProperty>(UT_TraversalComponent::StaticClass(), PropertyName);
			if (!Property) return false;
			*Property->ContainerPtrToValuePtr<FVector>(Component) = Value;
			return true;
		}

		bool GetVectorProperty(const TCHAR* PropertyName, FVector& OutValue) const
		{
			FStructProperty* Property = FindFProperty<FStructProperty>(UT_TraversalComponent::StaticClass(), PropertyName);
			if (!Property) return false;
			OutValue = *Property->ContainerPtrToValuePtr<FVector>(Component);
			return true;
		}

		bool GetFloatProperty(const TCHAR* PropertyName, float& OutValue) const
		{
			FFloatProperty* Property = FindFProperty<FFloatProperty>(UT_TraversalComponent::StaticClass(), PropertyName);
			if (!Property) return false;
			OutValue = Property->GetPropertyValue_InContainer(Component);
			return true;
		}

		FTraversalCheckResult MakeResult(ETraversalActionType ActionType) const
		{
			FTraversalCheckResult Result;
			Result.ActionType = ActionType;
			Result.WallLocation = FVector(100.f, 0.f, 50.f);
			Result.WallNormal = FVector(-1.f, 0.f, 0.f);
			Result.TopLocation = FVector(100.f, 0.f, 250.f);
			Result.LandingLocation = FVector(150.f, 0.f, 346.f);
			Result.FarEdgeLocation = FVector(140.f, 0.f, 250.f);
			return Result;
		}

		UWorld* World = nullptr;
		ACharacter* Character = nullptr;
		UT_TraversalComponent* Component = nullptr;
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTraversalFallingStateBlocksAbilityTest,
	"GASTestDemo1.Traversal.Activation.FallingStateBlocksAbility",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTraversalFallingStateBlocksAbilityTest::RunTest(const FString& Parameters)
{
	FTraversalWarpTargetTestWorld TestWorld;
	AT_PlayerCharacter* PlayerCharacter = TestWorld.World->SpawnActor<AT_PlayerCharacter>();
	UAbilitySystemComponent* AbilitySystemComponent =
		NewObject<UAbilitySystemComponent>(PlayerCharacter);
	PlayerCharacter->AddInstanceComponent(AbilitySystemComponent);
	AbilitySystemComponent->RegisterComponent();
	AbilitySystemComponent->InitAbilityActorInfo(PlayerCharacter, PlayerCharacter);

	const FGameplayAbilitySpecHandle AbilityHandle = AbilitySystemComponent->GiveAbility(
		FGameplayAbilitySpec(UT_Traversal::StaticClass(), 1));
	const UT_Traversal* TraversalAbility = GetDefault<UT_Traversal>();
	const FGameplayAbilityActorInfo* ActorInfo =
		AbilitySystemComponent->AbilityActorInfo.Get();

	PlayerCharacter->bIsFalling = false;
	TestTrue(
		TEXT("非空中状态允许进入 Traversal 激活检查"),
		TraversalAbility->CanActivateAbility(AbilityHandle, ActorInfo));

	PlayerCharacter->bIsFalling = true;
	TestFalse(
		TEXT("bIsFalling 为 true 时禁止激活 Traversal"),
		TraversalAbility->CanActivateAbility(AbilityHandle, ActorInfo));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTraversalClimbWarpTargetTest,
	"GASTestDemo1.Traversal.WarpTarget.ClimbHeightAndOffset",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTraversalClimbWarpTargetTest::RunTest(const FString& Parameters)
{
	FTraversalWarpTargetTestWorld TestWorld;
	TestWorld.Character->GetCapsuleComponent()->SetCapsuleHalfHeight(88.f);
	FVector DefaultOffset;
	float RootToLedgeHeight = 0.f;
	TestTrue(TEXT("Climb 独立偏移属性存在"), TestWorld.GetVectorProperty(TEXT("ClimbFrontLedgeWarpOffset"), DefaultOffset));
	TestTrue(TEXT("Climb 根骨到墙沿高度属性存在"), TestWorld.GetFloatProperty(TEXT("ClimbRootToLedgeHeight"), RootToLedgeHeight));
	TestEqual(TEXT("Climb 默认根骨到墙沿高度匹配动画"), RootToLedgeHeight, 132.f);
	TestTrue(TEXT("Climb 旧的 Z=-220 偏移不再参与高度计算"), TestWorld.SetVectorProperty(TEXT("ClimbFrontLedgeWarpOffset"), FVector(-10.f, 0.f, -220.f)));
	FTraversalCheckResult Result = TestWorld.MakeResult(ETraversalActionType::Climb);
	Result.TopLocation.Z = 200.f;
	TestWorld.Component->BuildWarpTargets(Result);
	TestEqual(TEXT("Climb 墙顶 200 时根骨目标为 68"), Result.FrontLedgeWarpTarget.GetLocation().Z, 68.0);
	TestEqual(TEXT("Climb 根骨保持在墙外"), Result.FrontLedgeWarpTarget.GetLocation().X, 100.0 - TestWorld.Character->GetCapsuleComponent()->GetScaledCapsuleRadius() - 2.0 - 10.0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTraversalMantleWarpTargetTest,
	"GASTestDemo1.Traversal.WarpTarget.MantleHeightAndOffset",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTraversalMantleWarpTargetTest::RunTest(const FString& Parameters)
{
	FTraversalWarpTargetTestWorld TestWorld;
	TestWorld.Character->GetCapsuleComponent()->SetCapsuleHalfHeight(88.f);
	FVector DefaultOffset;
	float RootToLedgeHeight = -1.f;
	TestTrue(TEXT("Mantle 独立偏移属性存在"), TestWorld.GetVectorProperty(TEXT("MantleFrontLedgeWarpOffset"), DefaultOffset));
	TestTrue(TEXT("Mantle 根骨到墙沿高度属性存在"), TestWorld.GetFloatProperty(TEXT("MantleRootToLedgeHeight"), RootToLedgeHeight));
	TestEqual(TEXT("Mantle 根骨到墙沿默认使用中性值"), RootToLedgeHeight, 0.f);
	TestTrue(TEXT("Mantle 独立偏移属性可设置"), TestWorld.SetVectorProperty(TEXT("MantleFrontLedgeWarpOffset"), FVector(-10.f, 0.f, -5.f)));
	FTraversalCheckResult Result = TestWorld.MakeResult(ETraversalActionType::Mantle);
	Result.WallLocation = FVector(0.f, 100.f, 50.f);
	Result.WallNormal = FVector(0.f, -1.f, 0.f);
	Result.TopLocation = FVector(0.f, 100.f, 250.f);
	TestWorld.Component->BuildWarpTargets(Result);
	TestEqual(TEXT("Mantle FrontLedge 不使用胶囊半高"), Result.FrontLedgeWarpTarget.GetLocation().Z, 245.0);
	TestEqual(TEXT("Mantle 旋转墙面后仍保持在墙外"), Result.FrontLedgeWarpTarget.GetLocation().Y, 100.0 - TestWorld.Character->GetCapsuleComponent()->GetScaledCapsuleRadius() - 2.0 - 10.0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTraversalBackLedgeByActionTest,
	"GASTestDemo1.Traversal.WarpTarget.BackLedgeByAction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTraversalBackLedgeByActionTest::RunTest(const FString& Parameters)
{
	FTraversalWarpTargetTestWorld TestWorld;
	TestTrue(
		TEXT("BackLedge 测试偏移可设置"),
		TestWorld.SetVectorProperty(
			TEXT("BackLedgeWarpOffset"),
			FVector(10.f, 0.f, 5.f)));

	FTraversalCheckResult VaultResult = TestWorld.MakeResult(ETraversalActionType::Vault);
	TestWorld.Component->BuildWarpTargets(VaultResult);
	TestFalse(
		TEXT("Vault 第二目标不等于未偏移后沿"),
		VaultResult.BackLedgeWarpTarget.GetLocation().Equals(VaultResult.FarEdgeLocation));
	const FVector ExpectedVaultBackLedge =
		VaultResult.FarEdgeLocation +
		VaultResult.TargetRotation.Quaternion().RotateVector(FVector(10.f, 0.f, 5.f));
	TestTrue(
		TEXT("Vault 第二目标精确保留共享偏移公式"),
		VaultResult.BackLedgeWarpTarget.GetLocation().Equals(ExpectedVaultBackLedge));

	FTraversalCheckResult ClimbResult = TestWorld.MakeResult(ETraversalActionType::Climb);
	TestWorld.Component->BuildWarpTargets(ClimbResult);
	TestTrue(
		TEXT("Climb 第二目标严格等于安全站立点"),
		ClimbResult.BackLedgeWarpTarget.GetLocation().Equals(ClimbResult.LandingLocation));

	FTraversalCheckResult MantleResult = TestWorld.MakeResult(ETraversalActionType::Mantle);
	TestWorld.Component->BuildWarpTargets(MantleResult);
	TestTrue(
		TEXT("Mantle 第二目标严格等于安全站立点"),
		MantleResult.BackLedgeWarpTarget.GetLocation().Equals(MantleResult.LandingLocation));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTraversalStandingEdgeClearanceRemovedTest,
	"GASTestDemo1.Traversal.Landing.StandingEdgeClearanceRemoved",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTraversalStandingEdgeClearanceRemovedTest::RunTest(const FString& Parameters)
{
	TestNull(
		TEXT("StandingEdgeClearance 已从 TraversalComponent 配置中删除"),
		FindFProperty<FProperty>(UT_TraversalComponent::StaticClass(), TEXT("StandingEdgeClearance")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTraversalSafeCompletionTest,
	"GASTestDemo1.Traversal.Landing.SafeCompletionRestoresWalking",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTraversalSafeCompletionTest::RunTest(const FString& Parameters)
{
	FTraversalWarpTargetTestWorld TestWorld;
	AT_PlayerCharacter* PlayerCharacter =
		TestWorld.World->SpawnActor<AT_PlayerCharacter>();
	UT_TraversalComponent* TraversalComponent =
		PlayerCharacter->FindComponentByClass<UT_TraversalComponent>();
	UT_Traversal* TraversalAbility = NewObject<UT_Traversal>();

	TestNotNull(TEXT("测试角色包含 TraversalComponent"), TraversalComponent);
	TestNotNull(TEXT("Traversal Ability 可创建"), TraversalAbility);
	if (!TraversalComponent || !TraversalAbility) return false;

	PlayerCharacter->SetActorLocation(FVector(0.f, 0.f, 500.f));
	FTraversalCheckResult CacheResult;
	TraversalComponent->DetectTraversal(CacheResult);
	PlayerCharacter->SetTraversalCollisionEnabled(false);
	TraversalAbility->PlayerCharacter = PlayerCharacter;
	TraversalAbility->TraversalComponent = TraversalComponent;
	TraversalAbility->CharacterMovementComponent = PlayerCharacter->GetCharacterMovement();
	TraversalAbility->CurrentActionType = ETraversalActionType::Mantle;

	TestTrue(
		TEXT("正常完成且当前位置可容纳完整胶囊时应恢复为落地状态"),
		TraversalAbility->TryRestoreTraversalCollision(false));

	const FVector MontageEndLocation(100.f, 200.f, 500.f);
	PlayerCharacter->SetActorLocation(MontageEndLocation);
	PlayerCharacter->SetTraversalCollisionEnabled(false);
	TraversalAbility->CurrentActionType = ETraversalActionType::Climb;

	const bool bClimbRestoredWithoutGround =
		TraversalAbility->TryRestoreTraversalCollision(false);
	TraversalAbility->RestoreCharacterState(false, bClimbRestoredWithoutGround);
	TestFalse(
		TEXT("Climb 下方没有安全地面时保持 Falling"),
		bClimbRestoredWithoutGround);
	TestEqual(
		TEXT("Climb 下方没有安全地面时移动模式为 Falling"),
		PlayerCharacter->GetCharacterMovement()->MovementMode,
		MOVE_Falling);
	TestTrue(
		TEXT("Climb 保留动画结束位置"),
		PlayerCharacter->GetActorLocation().Equals(MontageEndLocation));
	TestEqual(
		TEXT("Climb 完成时恢复胶囊碰撞"),
		PlayerCharacter->GetCapsuleComponent()->GetCollisionEnabled(),
		ECollisionEnabled::QueryAndPhysics);

	AActor* FloorActor = TestWorld.World->SpawnActor<AActor>();
	UBoxComponent* FloorComponent = NewObject<UBoxComponent>(FloorActor);
	FloorActor->SetRootComponent(FloorComponent);
	FloorActor->AddInstanceComponent(FloorComponent);
	FloorComponent->SetBoxExtent(FVector(500.f, 500.f, 10.f));
	FloorComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	FloorComponent->SetCollisionObjectType(ECC_WorldStatic);
	FloorComponent->SetCollisionResponseToAllChannels(ECR_Block);
	FloorComponent->RegisterComponent();
	FloorActor->SetActorLocation(FVector(300.f, 0.f, -10.f));

	const FVector MinorPenetrationLocation(300.f, 0.f, 80.f);
	PlayerCharacter->SetActorLocation(MinorPenetrationLocation);
	PlayerCharacter->SetTraversalCollisionEnabled(false);
	TraversalAbility->CurrentTraversalResult.LandingLocation = FVector(300.f, 0.f, 88.f);

	const bool bClimbRestoredWithGround =
		TraversalAbility->TryRestoreTraversalCollision(false);
	TraversalAbility->RestoreCharacterState(false, bClimbRestoredWithGround);
	TestTrue(
		TEXT("Climb 下方存在安全地面时恢复 Walking"),
		bClimbRestoredWithGround);
	TestEqual(
		TEXT("Climb 安全落地后的移动模式为 Walking"),
		PlayerCharacter->GetCharacterMovement()->MovementMode,
		MOVE_Walking);
	TestTrue(
		TEXT("Climb 轻微穿透修正时保留动画结束水平位置"),
		FMath::IsNearlyZero(FVector::Dist2D(
			PlayerCharacter->GetActorLocation(),
			MinorPenetrationLocation)));

	PlayerCharacter->SetActorLocation(FVector(300.f, 0.f, -50.f));
	PlayerCharacter->SetTraversalCollisionEnabled(false);
	TraversalAbility->CurrentTraversalResult.LandingLocation = FVector(
		300.f,
		200.f,
		PlayerCharacter->GetCapsuleComponent()->GetScaledCapsuleHalfHeight());

	TestTrue(
		TEXT("Climb 中断且当前位置穿透时使用缓存安全落点"),
		TraversalAbility->TryRestoreTraversalCollision(true));
	TestTrue(
		TEXT("Climb 中断后到达缓存安全落点"),
		PlayerCharacter->GetActorLocation().Equals(FVector(
			300.f,
			200.f,
			PlayerCharacter->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 2.f)));

	const FVector VaultEndLocation(
		300.f,
		0.f,
		PlayerCharacter->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() +
			2.f);
	PlayerCharacter->SetActorLocation(VaultEndLocation);
	PlayerCharacter->SetTraversalCollisionEnabled(false);
	TraversalAbility->PlayerCharacter = PlayerCharacter;
	TraversalAbility->TraversalComponent = TraversalComponent;
	TraversalAbility->CurrentActionType = ETraversalActionType::Vault;

	const bool bVaultRestoredWithGround =
		TraversalAbility->TryRestoreTraversalCollision(false);
	TraversalAbility->RestoreCharacterState(false, bVaultRestoredWithGround);
	TestTrue(
		TEXT("Vault 下方存在安全地面时恢复 Walking"),
		bVaultRestoredWithGround);
	TestEqual(
		TEXT("Vault 安全落地后的移动模式为 Walking"),
		PlayerCharacter->GetCharacterMovement()->MovementMode,
		MOVE_Walking);
	TestTrue(
		TEXT("Vault 保留动画结束位置"),
		PlayerCharacter->GetActorLocation().Equals(VaultEndLocation));
	TestEqual(
		TEXT("Vault 完成时恢复胶囊碰撞"),
		PlayerCharacter->GetCapsuleComponent()->GetCollisionEnabled(),
		ECollisionEnabled::QueryAndPhysics);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTraversalVaultWarpTargetTest,
	"GASTestDemo1.Traversal.WarpTarget.VaultUnchanged",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTraversalVaultWarpTargetTest::RunTest(const FString& Parameters)
{
	FTraversalWarpTargetTestWorld TestWorld;
	TestTrue(TEXT("Vault 共享偏移属性存在"), TestWorld.SetVectorProperty(TEXT("FrontLedgeWarpOffset"), FVector(-3.f, 4.f, 5.f)));
	FTraversalCheckResult Result = TestWorld.MakeResult(ETraversalActionType::Vault);
	TestWorld.Component->BuildWarpTargets(Result);
	TestEqual(TEXT("Vault 继续使用角色当前高度和共享 Z 偏移"), Result.FrontLedgeWarpTarget.GetLocation().Z, 105.0);
	TestEqual(TEXT("Vault 继续使用原有水平位置和共享 X 偏移"), Result.FrontLedgeWarpTarget.GetLocation().X, 100.0 - TestWorld.Character->GetCapsuleComponent()->GetScaledCapsuleRadius() - 2.0 - 3.0);
	TestEqual(TEXT("Vault 继续使用共享 Y 偏移"), Result.FrontLedgeWarpTarget.GetLocation().Y, 4.0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTraversalCurrentStandingLocationTest,
	"GASTestDemo1.Traversal.Landing.PreserveCurrentHorizontalLocation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTraversalCurrentStandingLocationTest::RunTest(const FString& Parameters)
{
	FTraversalWarpTargetTestWorld TestWorld;
	TestWorld.Character->GetCapsuleComponent()->SetCapsuleHalfHeight(88.f);

	AActor* FloorActor = TestWorld.World->SpawnActor<AActor>();
	UBoxComponent* FloorComponent = NewObject<UBoxComponent>(FloorActor);
	FloorActor->SetRootComponent(FloorComponent);
	FloorActor->AddInstanceComponent(FloorComponent);
	FloorComponent->SetBoxExtent(FVector(500.f, 500.f, 10.f));
	FloorComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	FloorComponent->SetCollisionObjectType(ECC_WorldStatic);
	FloorComponent->SetCollisionResponseToAllChannels(ECR_Block);
	FloorComponent->RegisterComponent();
	FloorActor->SetActorLocation(FVector(0.f, 0.f, -10.f));

	FVector StandingLocation;
	TestTrue(
		TEXT("当前水平位置下方存在完整胶囊安全落点"),
		TestWorld.Component->FindSafeStandingLocationBelow(
			FVector(75.f, 25.f, 88.f),
			2.f,
			StandingLocation));
	TestEqual(TEXT("安全落点保留当前 X"), StandingLocation.X, 75.0);
	TestEqual(TEXT("安全落点保留当前 Y"), StandingLocation.Y, 25.0);
	TestEqual(TEXT("安全落点只修正站立 Z"), StandingLocation.Z, 90.0);
	return true;
}

#endif
