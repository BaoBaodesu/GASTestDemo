# Test2 Shimmy 悬挂移动修复 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 让角色悬挂在 Bar 上时只通过现有 Blueprint `Shimmy()` 链左右移动，并保持高度和朝向稳定。

**Architecture:** `UTest2Component` 在进入 Bar 悬挂状态时建立正确的移动模式和旋转状态；`AT_PlayerController` 在悬挂期间停止提交普通移动输入。现有 Blueprint 输入链、抓取检测和脱离逻辑保持不变。

**Tech Stack:** Unreal Engine 5 C++、Enhanced Input、CharacterMovement、Automation Tests

## Global Constraints

- 不修改 `BP_TestCharacter` 的输入节点。
- 不把 `Shimmy()` 调用迁移到 `AT_PlayerController`。
- 不重构抓取、攀爬或跳离逻辑。
- 使用 UTF-8 中文注释，保持现有代码结构和编码习惯。
- 只修改本任务列出的文件，不触碰当前工作区的其他未提交内容。

---

### Task 1: 锁定 Bar 悬挂状态

**Files:**
- Create: `Source/GASTestDemo1/Private/Tests/Test2ShimmyTests.cpp`
- Modify: `Source/GASTestDemo1/Private/Player/Components/Test2Component.cpp:300-305`

**Interfaces:**
- Consumes: `UTest2Component::AlignGrab()`、`UTest2Component::GrabType`
- Produces: Bar 悬挂状态使用 `MOVE_Walking`，且关闭 `bOrientRotationToMovement` 和 `bUseControllerDesiredRotation`

- [ ] **Step 1: 写失败测试**

新增一个真实 `UWorld`、`ACharacter` 和 `UTest2Component` 的自动化测试。测试在调用 `AlignGrab()` 前主动开启两个旋转选项，调用后断言：

```cpp
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
		TEXT("Bar 悬挂使用 Flying，避免 Walking 贴地修正改变 Z"),
		TestWorld.CharacterMovement->MovementMode,
		MOVE_Walking);
	TestFalse(
		TEXT("进入 Bar 悬挂时立即关闭朝移动方向旋转"),
		TestWorld.CharacterMovement->bOrientRotationToMovement);
	TestFalse(
		TEXT("进入 Bar 悬挂时立即关闭控制器期望旋转"),
		TestWorld.CharacterMovement->bUseControllerDesiredRotation);
	return true;
}
```

测试夹具负责创建和销毁临时世界，并注册组件：

```cpp
struct FTest2ShimmyTestWorld
{
	FTest2ShimmyTestWorld()
	{
		World = UWorld::CreateWorld(EWorldType::Game, false, TEXT("Test2ShimmyTestWorld"));
		FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
		WorldContext.SetCurrentWorld(World);
		Character = World->SpawnActor<ACharacter>();
		Component = NewObject<UTest2Component>(Character);
		Character->AddInstanceComponent(Component);
		Component->RegisterComponent();
		World->BeginPlay();
		CharacterMovement = Character->GetCharacterMovement();
	}

	~FTest2ShimmyTestWorld()
	{
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(false);
	}

	UWorld* World = nullptr;
	ACharacter* Character = nullptr;
	UTest2Component* Component = nullptr;
	UCharacterMovementComponent* CharacterMovement = nullptr;
};
```

- [ ] **Step 2: 运行测试并确认旧代码失败**

先构建 `GASTestDemo1Editor Win64 Development`，然后运行：

```powershell
UnrealEditor-Cmd.exe GASTestDemo1.uproject -ExecCmds="Automation RunTests GASTestDemo1.Test2.Shimmy.BarGrabLocksHeightAndFacing;Quit" -unattended -nop4 -nosplash
```

预期：测试在 `MovementMode` 上失败，实际值为 `MOVE_Walking`；两个旋转断言也失败。

- [ ] **Step 3: 实施最小状态修复**

仅修改 `AlignGrab()` 的 Bar 分支：

```cpp
CharacterMovement->GravityScale = 0.0f;
CharacterMovement->MaxWalkSpeed = 70.0f;
CharacterMovement->bOrientRotationToMovement = false;
CharacterMovement->bUseControllerDesiredRotation = false;
CharacterMovement->SetMovementMode(MOVE_Walking);
bOnBar = true;
```

- [ ] **Step 4: 重跑测试确认通过**

重新构建并运行同一测试。预期：三个断言全部通过。

### Task 2: 阻断悬挂期间的普通移动输入

**Files:**
- Modify: `Source/GASTestDemo1/Private/Player/T_PlayerController.cpp:14-16,70-84`

**Interfaces:**
- Consumes: `UTest2Component::bOnBar`
- Produces: `AT_PlayerController::Move()` 在 Bar 悬挂期间把左右轴直接路由到 `UTest2Component::Shimmy()`

- [ ] **Step 1: 增加组件依赖**

在 Controller 实现文件中加入：

```cpp
#include "Player/Components/Test2Component.h"
```

- [ ] **Step 2: 增加最小输入门控**

在 `Move()` 完成 Pawn 和存活检查后先保留当前输入值：

```cpp
MovementVector = Value.Get<FVector2D>();
```

读取输入后，Bar 悬挂状态直接调用 `Shimmy()` 并返回：

```cpp
if (UTest2Component* Test2Component =
	GetPawn()->FindComponentByClass<UTest2Component>();
	IsValid(Test2Component) && Test2Component->bOnBar)
{
	Test2Component->Shimmy(MovementVector.X);
	return;
}
```

普通状态继续使用摄像机相对的前后和左右输入；悬挂方向由 `BarMoveDirection` 决定，不再受摄像机角度影响。

- [ ] **Step 3: 完整编译**

运行：

```powershell
Build.bat GASTestDemo1Editor Win64 Development -Project=D:\Ue5Project\GASTestDemo1\GASTestDemo1.uproject -WaitMutex -NoLiveCoding
```

预期：`Result: Succeeded`，没有新增编译或链接错误。

### Task 3: PIE 行为回归

**Files:**
- Verify only: `Content/GASTestDemo/Characters/PlayerCharacters/BP_TestCharacter.uasset`
- Verify only: `Content/GASTestDemo/Maps/TestMap.umap`

**Interfaces:**
- Consumes: Task 1 的悬挂状态、Task 2 的普通移动门控
- Produces: 方案 1 的运行时验收结论

- [ ] **Step 1: 验证左右移动**

在 `TestMap` 进入 PIE，抓住带 `Bar` Actor Tag 的横杆，分别持续按左、右方向。记录开始和结束时角色 Actor Location 的 Z。

预期：角色沿 Bar 左右移动，Z 不持续下降。

- [ ] **Step 2: 验证首次点按**

重新抓杆，在没有持续输入的情况下分别点按一次左、右方向。

预期：角色保持面向横杆，不转向移动方向。

- [ ] **Step 3: 验证前后输入**

悬挂时分别持续按前、后方向。

预期：角色 Actor Location 不发生移动。

- [ ] **Step 4: 验证脱离回归**

调用现有跳离或脱离流程，落地后测试前后左右移动。

预期：移动模式恢复 `MOVE_Walking`，普通移动和现有转向行为恢复。
