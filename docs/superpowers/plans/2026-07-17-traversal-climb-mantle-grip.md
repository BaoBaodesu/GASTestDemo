# Climb 与 Mantle 墙沿抓取修复 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 修正 Climb 与 Mantle 的 FrontLedge Motion Warp Target，使角色双手跟随真实墙顶高度并保持在墙体外侧，同时不改变 Vault 行为。

**Architecture:** `UT_TraversalComponent` 继续负责检测结果和 Warp Target 构建，`UT_Traversal` 继续只消费结果。Climb/Mantle 使用墙顶高度加胶囊半高作为根骨目标，并使用各自动作局部偏移；Vault 继续使用原有高度和偏移。

**Tech Stack:** Unreal Engine 5.7、C++、Motion Warping、Unreal Automation Tests。

## Global Constraints

- 保持现有 `UT_TraversalComponent` 检测层与 `UT_Traversal` 执行层职责边界。
- 不修改 Montage、AnimBP、IK、GameplayTag、MovementMode 或碰撞恢复逻辑。
- 不改变 Vault 当前行为。
- 使用 UTF-8 中文注释，并保持现有代码结构和编码习惯。
- 只修改与 FrontLedge 计算、调试显示和对应自动化测试直接相关的代码。
- 目标源码已有用户未提交改动；不得重置、覆盖或把这些既有改动混入新的提交。

---

## File Structure

- `Source/GASTestDemo1/Private/Tests/TraversalWarpTargetTests.cpp`：真实调用 `BuildWarpTargets()` 的回归测试。
- `Source/GASTestDemo1/Public/Player/Components/T_TraversalComponent.h`：声明 Climb/Mantle 独立局部偏移。
- `Source/GASTestDemo1/Private/Player/Components/T_TraversalComponent.cpp`：按动作构建 FrontLedge，并绘制最终根骨目标。

### Task 1: 建立 FrontLedge 回归测试

**Files:**
- Create: `Source/GASTestDemo1/Private/Tests/TraversalWarpTargetTests.cpp`
- Test: `Source/GASTestDemo1/Private/Tests/TraversalWarpTargetTests.cpp`

**Interfaces:**
- Consumes: `UT_TraversalComponent::BuildWarpTargets(FTraversalCheckResult&) const`。
- Produces: `GASTestDemo1.Traversal.WarpTarget.ClimbHeightAndOffset`、`MantleHeightAndOffset`、`VaultUnchanged` 三个自动化测试。

- [ ] **Step 1: 写入失败测试**

```cpp
#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Components/CapsuleComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
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
	FTraversalClimbWarpTargetTest,
	"GASTestDemo1.Traversal.WarpTarget.ClimbHeightAndOffset",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTraversalClimbWarpTargetTest::RunTest(const FString& Parameters)
{
	FTraversalWarpTargetTestWorld TestWorld;
	FVector DefaultOffset;
	TestTrue(TEXT("Climb 独立偏移属性存在"), TestWorld.GetVectorProperty(TEXT("ClimbFrontLedgeWarpOffset"), DefaultOffset));
	TestTrue(TEXT("Climb 默认向墙外偏移 10cm"), DefaultOffset.Equals(FVector(-10.f, 0.f, 0.f)));
	TestTrue(TEXT("Climb 独立偏移属性可设置"), TestWorld.SetVectorProperty(TEXT("ClimbFrontLedgeWarpOffset"), FVector(-10.f, 0.f, 5.f)));
	FTraversalCheckResult Result = TestWorld.MakeResult(ETraversalActionType::Climb);
	TestWorld.Component->BuildWarpTargets(Result);
	TestEqual(TEXT("Climb 根骨高度跟随墙顶"), Result.FrontLedgeWarpTarget.GetLocation().Z, 250.0 + TestWorld.Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 5.0);
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
	FVector DefaultOffset;
	TestTrue(TEXT("Mantle 独立偏移属性存在"), TestWorld.GetVectorProperty(TEXT("MantleFrontLedgeWarpOffset"), DefaultOffset));
	TestTrue(TEXT("Mantle 默认向墙外偏移 10cm"), DefaultOffset.Equals(FVector(-10.f, 0.f, 0.f)));
	TestTrue(TEXT("Mantle 独立偏移属性可设置"), TestWorld.SetVectorProperty(TEXT("MantleFrontLedgeWarpOffset"), FVector(-10.f, 0.f, -5.f)));
	FTraversalCheckResult Result = TestWorld.MakeResult(ETraversalActionType::Mantle);
	Result.WallLocation = FVector(0.f, 100.f, 50.f);
	Result.WallNormal = FVector(0.f, -1.f, 0.f);
	Result.TopLocation = FVector(0.f, 100.f, 250.f);
	TestWorld.Component->BuildWarpTargets(Result);
	TestEqual(TEXT("Mantle 根骨高度跟随墙顶"), Result.FrontLedgeWarpTarget.GetLocation().Z, 250.0 + TestWorld.Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() - 5.0);
	TestEqual(TEXT("Mantle 旋转墙面后仍保持在墙外"), Result.FrontLedgeWarpTarget.GetLocation().Y, 100.0 - TestWorld.Character->GetCapsuleComponent()->GetScaledCapsuleRadius() - 2.0 - 10.0);
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

#endif
```

- [ ] **Step 2: 构建并运行测试，确认 RED**

Run:

```powershell
& 'D:\ue5\Epic Games\UE_5.7\Engine\Build\BatchFiles\Build.bat' GASTestDemo1Editor Win64 Development 'D:\Ue5Project\GASTestDemo1\GASTestDemo1.uproject' -WaitMutex -NoHotReloadFromIDE
& 'D:\ue5\Epic Games\UE_5.7\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' 'D:\Ue5Project\GASTestDemo1\GASTestDemo1.uproject' -ExecCmds="Automation RunTests GASTestDemo1.Traversal.WarpTarget;Quit" -unattended -nop4 -NullRHI -nosplash -log
```

Expected: 编译成功；Climb 与 Mantle 测试因独立属性不存在以及目标高度仍使用旧逻辑而失败，Vault 测试通过。

### Task 2: 实现动作独立 FrontLedge 计算与调试显示

**Files:**
- Modify: `Source/GASTestDemo1/Public/Player/Components/T_TraversalComponent.h:354-363`
- Modify: `Source/GASTestDemo1/Private/Player/Components/T_TraversalComponent.cpp:495-583`
- Test: `Source/GASTestDemo1/Private/Tests/TraversalWarpTargetTests.cpp`

**Interfaces:**
- Consumes: `FTraversalCheckResult::ActionType`、`WallLocation`、`WallNormal`、`TopLocation` 和角色胶囊尺寸。
- Produces: `ClimbFrontLedgeWarpOffset`、`MantleFrontLedgeWarpOffset` 与修正后的 `FrontLedgeWarpTarget`。

- [ ] **Step 1: 在组件头文件添加动作独立偏移**

```cpp
	// Climb 根骨相对墙沿目标的局部偏移，X 负方向远离墙体
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal|Motion Warping")
	FVector ClimbFrontLedgeWarpOffset = FVector(-10.f, 0.f, 0.f);

	// Mantle 根骨相对墙沿目标的局部偏移，X 负方向远离墙体
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal|Motion Warping")
	FVector MantleFrontLedgeWarpOffset = FVector(-10.f, 0.f, 0.f);
```

- [ ] **Step 2: 按动作修改 FrontLedge 计算**

用以下分支替换当前 `FrontLedgeLocation.Z` 和统一 `FrontLedgeWarpOffset` 逻辑：

```cpp
	switch (TraversalResult.ActionType)
	{
	case ETraversalActionType::Climb:
		FrontLedgeLocation.Z = TraversalResult.TopLocation.Z + OwnerCapsuleComponent->GetScaledCapsuleHalfHeight();
		FrontLedgeLocation += TargetRotationQuaternion.RotateVector(ClimbFrontLedgeWarpOffset);
		break;
	case ETraversalActionType::Mantle:
		FrontLedgeLocation.Z = TraversalResult.TopLocation.Z + OwnerCapsuleComponent->GetScaledCapsuleHalfHeight();
		FrontLedgeLocation += TargetRotationQuaternion.RotateVector(MantleFrontLedgeWarpOffset);
		break;
	default:
		FrontLedgeLocation.Z = OwnerCharacter->GetActorLocation().Z;
		FrontLedgeLocation += TargetRotationQuaternion.RotateVector(FrontLedgeWarpOffset);
		break;
	}
```

- [ ] **Step 3: 绘制最终 FrontLedge 根骨目标**

在 `DrawTraversalDebug()` 中现有 `TopLocation` 青色球之后增加：

```cpp
	DrawDebugSphere(
		GetWorld(),
		Result.FrontLedgeWarpTarget.GetLocation(),
		12.f,
		12,
		FColor::Purple,
		false,
		GetDebugDuration()
	);

	DrawDebugDirectionalArrow(
		GetWorld(),
		Result.FrontLedgeWarpTarget.GetLocation(),
		Result.FrontLedgeWarpTarget.GetLocation() + Result.FrontLedgeWarpTarget.GetRotation().GetForwardVector() * 35.f,
		10.f,
		FColor::Purple,
		false,
		GetDebugDuration(),
		0,
		2.f
	);
```

- [ ] **Step 4: 重新构建并确认 GREEN**

Run:

```powershell
& 'D:\ue5\Epic Games\UE_5.7\Engine\Build\BatchFiles\Build.bat' GASTestDemo1Editor Win64 Development 'D:\Ue5Project\GASTestDemo1\GASTestDemo1.uproject' -WaitMutex -NoHotReloadFromIDE
& 'D:\ue5\Epic Games\UE_5.7\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' 'D:\Ue5Project\GASTestDemo1\GASTestDemo1.uproject' -ExecCmds="Automation RunTests GASTestDemo1.Traversal.WarpTarget;Quit" -unattended -nop4 -NullRHI -nosplash -log
```

Expected: 构建退出码为 0，三个 `GASTestDemo1.Traversal.WarpTarget` 测试全部通过。

- [ ] **Step 5: 检查最小差异并保留用户既有暂存状态**

Run:

```powershell
git diff --check -- Source/GASTestDemo1/Public/Player/Components/T_TraversalComponent.h Source/GASTestDemo1/Private/Player/Components/T_TraversalComponent.cpp Source/GASTestDemo1/Private/Tests/TraversalWarpTargetTests.cpp
git diff -- Source/GASTestDemo1/Public/Player/Components/T_TraversalComponent.h Source/GASTestDemo1/Private/Player/Components/T_TraversalComponent.cpp Source/GASTestDemo1/Private/Tests/TraversalWarpTargetTests.cpp
git status --short
```

Expected: 只有两个 Traversal 组件文件和新测试文件包含本次修改；不执行源码提交，因为两个组件文件已有用户改动且无法安全拆分提交。

### Task 3: 运行时验收交接

**Files:**
- Inspect: `Saved/Logs/GASTestDemo1.log`
- Inspect: `Content/GASTestDemo/Characters/PlayerCharacters/BP_TestCharacter.uasset`

**Interfaces:**
- Consumes: 紫色 FrontLedge 根骨目标、青色 `TopLocation` 和 Climb/Mantle Montage 运行结果。
- Produces: 对 Climb、Mantle、Vault 运行时表现的验收记录。

- [ ] **Step 1: 在 PIE 中分别触发 Climb、Mantle、Vault**

操作：对三种高度/厚度的障碍物各触发一次动作，观察青色墙顶点与紫色根骨目标，并在手掌接触墙沿的帧暂停。

Expected: Climb/Mantle 双手位于墙顶边缘上方且不穿墙；Vault 与修改前一致。

- [ ] **Step 2: 验证动作结束和打断恢复**

操作：完整播放一次 Climb/Mantle，并在动作中断一次。

Expected: 角色落点有效，胶囊碰撞恢复，MovementMode 不会停留在 Flying。

- [ ] **Step 3: 若 Montage 仍有细小偏差，仅调整动作独立偏移**

在 `BP_TestCharacter` 的 `TraversalComponent` 中只调整以下属性：

```text
ClimbFrontLedgeWarpOffset: X 负值增加会远离墙体，Z 正值增加会抬高手掌
MantleFrontLedgeWarpOffset: X 负值增加会远离墙体，Z 正值增加会抬高手掌
```

Expected: 不修改检测距离、墙顶高度、Montage 或共享 Vault 偏移即可完成动画资源级微调。
