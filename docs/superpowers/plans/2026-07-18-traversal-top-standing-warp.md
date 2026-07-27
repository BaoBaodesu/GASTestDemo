# Climb/Mantle Top Standing Warp Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make Climb and Mantle finish at their validated top-standing location without routinely teleporting out of platform-edge overlap, while preserving Vault behavior.

**Architecture:** Keep the existing two named Motion Warping targets. Split `BackLedgeLocation` calculation by action type, give Mantle an explicit root-to-ledge height, preserve the existing collision-recovery fallback, and correct only diagnostics, callback timing, and normal-completion velocity handling around that strategy.

**Tech Stack:** Unreal Engine 5.7, C++20/Unreal Build Tool, Gameplay Ability System, Motion Warping, Unreal Automation Tests.

## Global Constraints

- Do not add a third Motion Warping target or rename `FrontLedge`/`BackLedge`.
- Do not modify Montage assets from C++.
- Keep Vault's two-window behavior and `BackLedgeWarpOffset` behavior unchanged.
- Do not enlarge `MaximumLandingCorrectionDistance`, shrink final collision capsules, or make cached-landing teleport the normal path.
- Do not refactor unrelated Traversal code or fix unrelated warnings.
- Do not create a Git commit.

---

### Task 1: Lock action-specific warp target behavior with failing tests

**Files:**
- Modify: `Source/GASTestDemo1/Private/Tests/TraversalWarpTargetTests.cpp:105-142`

**Interfaces:**
- Consumes: `UT_TraversalComponent::BuildWarpTargets(FTraversalCheckResult&) const` and reflected properties on `UT_TraversalComponent`.
- Produces: Regression coverage for Vault, Climb, and Mantle second targets plus Mantle FrontLedge height semantics.

- [ ] **Step 1: Extend the Mantle test before production changes**

Add assertions that the new property must exist with default zero and that capsule half height is absent from the result:

```cpp
float RootToLedgeHeight = -1.f;
TestTrue(
    TEXT("Mantle 根骨到墙沿高度属性存在"),
    TestWorld.GetFloatProperty(
        TEXT("MantleRootToLedgeHeight"),
        RootToLedgeHeight));
TestEqual(TEXT("Mantle 根骨到墙沿默认使用中性值"), RootToLedgeHeight, 0.f);

TestWorld.Character->GetCapsuleComponent()->SetCapsuleHalfHeight(88.f);
TestTrue(
    TEXT("Mantle 独立偏移属性可设置"),
    TestWorld.SetVectorProperty(
        TEXT("MantleFrontLedgeWarpOffset"),
        FVector(-10.f, 0.f, -5.f)));
```

Change the Z expectation to:

```cpp
TestEqual(
    TEXT("Mantle FrontLedge 不使用胶囊半高"),
    Result.FrontLedgeWarpTarget.GetLocation().Z,
    245.0);
```

- [ ] **Step 2: Add an action-specific BackLedge test before production changes**

```cpp
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

    FTraversalCheckResult VaultResult =
        TestWorld.MakeResult(ETraversalActionType::Vault);
    TestWorld.Component->BuildWarpTargets(VaultResult);
    TestFalse(
        TEXT("Vault 第二目标继续应用共享偏移"),
        VaultResult.BackLedgeWarpTarget.GetLocation().Equals(
            VaultResult.FarEdgeLocation));

    FTraversalCheckResult ClimbResult =
        TestWorld.MakeResult(ETraversalActionType::Climb);
    TestWorld.Component->BuildWarpTargets(ClimbResult);
    TestTrue(
        TEXT("Climb 第二目标严格等于安全站立点"),
        ClimbResult.BackLedgeWarpTarget.GetLocation().Equals(
            ClimbResult.LandingLocation));

    FTraversalCheckResult MantleResult =
        TestWorld.MakeResult(ETraversalActionType::Mantle);
    TestWorld.Component->BuildWarpTargets(MantleResult);
    TestTrue(
        TEXT("Mantle 第二目标严格等于安全站立点"),
        MantleResult.BackLedgeWarpTarget.GetLocation().Equals(
            MantleResult.LandingLocation));
    return true;
}
```

- [ ] **Step 3: Build and run the tests to verify RED**

Build command:

```powershell
& 'D:\ue5\Epic Games\UE_5.7\Engine\Build\BatchFiles\Build.bat' `
  GASTestDemo1Editor Win64 Development `
  '-Project=D:\Ue5Project\GASTestDemo1\GASTestDemo1.uproject' `
  -WaitMutex -NoHotReloadFromIDE
```

Test command:

```powershell
& 'D:\ue5\Epic Games\UE_5.7\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' `
  'D:\Ue5Project\GASTestDemo1\GASTestDemo1.uproject' `
  -unattended -nop4 -nosplash -nullrhi `
  '-ExecCmds=Automation RunTests GASTestDemo1.Traversal.WarpTarget;Quit'
```

Expected RED: the Mantle property assertion fails because `MantleRootToLedgeHeight` does not exist; Climb/Mantle BackLedge assertions fail because the shared offset is still applied.

---

### Task 2: Implement action-specific warp targets and debug semantics

**Files:**
- Modify: `Source/GASTestDemo1/Public/Player/Components/T_TraversalComponent.h:357-370`
- Modify: `Source/GASTestDemo1/Private/Player/Components/T_TraversalComponent.cpp:610-671`
- Test: `Source/GASTestDemo1/Private/Tests/TraversalWarpTargetTests.cpp`

**Interfaces:**
- Consumes: `FTraversalCheckResult::{ActionType,TopLocation,FarEdgeLocation,LandingLocation}`.
- Produces: `float MantleRootToLedgeHeight`, unchanged target names, and action-specific `BackLedgeWarpTarget` transforms.

- [ ] **Step 1: Add the minimal Mantle height property**

Place beside `ClimbRootToLedgeHeight`:

```cpp
// Mantle 动画中，接触墙沿时根骨低于墙沿的高度；默认使用中性值
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal|Motion Warping",
    meta = (ClampMin = "0.0", Units = "cm"))
float MantleRootToLedgeHeight = 0.f;
```

- [ ] **Step 2: Replace Mantle's capsule-centre FrontLedge calculation**

```cpp
case ETraversalActionType::Mantle:
    FrontLedgeLocation.Z =
        TraversalResult.TopLocation.Z - MantleRootToLedgeHeight;
    FrontLedgeLocation +=
        TargetRotationQuaternion.RotateVector(MantleFrontLedgeWarpOffset);
    break;
```

- [ ] **Step 3: Split the second target by action type**

Replace the common base-plus-offset block with:

```cpp
FVector BackLedgeLocation;
switch (TraversalResult.ActionType)
{
case ETraversalActionType::Vault:
    BackLedgeLocation =
        TraversalResult.FarEdgeLocation +
        TargetRotationQuaternion.RotateVector(BackLedgeWarpOffset);
    break;
case ETraversalActionType::Climb:
case ETraversalActionType::Mantle:
default:
    BackLedgeLocation = TraversalResult.LandingLocation;
    break;
}
```

- [ ] **Step 4: Change only the target-display capsule color**

In `DrawTraversalDebug()`, change the `Result.LandingLocation` capsule color from:

```cpp
FColor::Green
```

to:

```cpp
FColor::Cyan
```

Do not change the Red/Green overlap capsule in `IsCapsuleLocationClear()`.

- [ ] **Step 5: Rebuild and verify GREEN for WarpTarget tests**

Run the Task 1 build and test commands.

Expected GREEN: all `GASTestDemo1.Traversal.WarpTarget` tests pass, including Vault unchanged and Climb/Mantle exact landing targets.

---

### Task 3: Preserve recovery policy while fixing completion diagnostics and timing

**Files:**
- Modify: `Source/GASTestDemo1/Private/AbilitySystem/Abilities/T_Traversal.cpp:104-220`
- Verify: `Source/GASTestDemo1/Public/AbilitySystem/Abilities/T_Traversal.h`

**Interfaces:**
- Consumes: `CurrentActionType`, `CurrentTraversalResult.LandingLocation`, `MaximumLandingCorrectionDistance`, `LandingCollisionClearance`.
- Produces: One normal-completion diagnostic log and collision restoration that preserves current safe position before fallback.

- [ ] **Step 1: Remove premature normal completion**

Delete only:

```cpp
PlayMontageTask->OnBlendOut.AddDynamic(
    this,
    &ThisClass::OnTraversalFinished);
```

Keep `OnCompleted`, `OnInterrupted`, and `OnCancelled` bindings.

- [ ] **Step 2: Add the endpoint diagnostic at the start of normal completion**

Immediately after `const FVector CurrentLocation = ...` add:

```cpp
UE_LOG(
    LogTemp,
    Warning,
    TEXT("Traversal End: Action=%d Current=%s Landing=%s Delta2D=%.2f DeltaZ=%.2f"),
    static_cast<int32>(CurrentActionType),
    *CurrentLocation.ToCompactString(),
    *CurrentTraversalResult.LandingLocation.ToCompactString(),
    FVector::Dist2D(CurrentLocation, CurrentTraversalResult.LandingLocation),
    FMath::Abs(CurrentLocation.Z - CurrentTraversalResult.LandingLocation.Z));
```

- [ ] **Step 3: Make a fully clear current capsule the first normal path**

Before calling `FindSafeStandingLocationBelow`, add:

```cpp
if (TraversalComponent->IsCapsuleLocationClear(CurrentLocation, 0.f))
{
    PlayerCharacter->SetTraversalCollisionEnabled(true);
    return true;
}
```

Remove the later duplicate current-clear branch. Retain the limited vertical correction, cached landing fallback, warning string, interrupted-current check, and start-location fallback unchanged.

- [ ] **Step 4: Verify normal completion does not clear velocity**

Keep this structure in `RestoreCharacterState()`:

```cpp
if (bWasCancelled)
{
    CharacterMovementComponent->StopMovementImmediately();
}

CharacterMovementComponent->SetMovementMode(
    !bWasCancelled && bRestoredToLanding
        ? MOVE_Walking
        : MOVE_Falling);
```

Do not add a normal-completion `StopMovementImmediately()`.

- [ ] **Step 5: Run all Traversal automation tests**

```powershell
& 'D:\ue5\Epic Games\UE_5.7\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' `
  'D:\Ue5Project\GASTestDemo1\GASTestDemo1.uproject' `
  -unattended -nop4 -nosplash -nullrhi `
  '-ExecCmds=Automation RunTests GASTestDemo1.Traversal;Quit'
```

Expected: all Traversal tests pass with no test errors.

- [ ] **Step 6: Perform final build and dependency checks**

Run the Task 1 build command, then:

```powershell
rg -n "OnBlendOut.*OnTraversalFinished|MantleRootToLedgeHeight|BackLedgeWarpOffset" `
  Source/GASTestDemo1
```

Expected: no `OnBlendOut` completion binding; the Mantle property has one declaration and one production use; the shared BackLedge offset remains used by Vault only in `BuildWarpTargets()`.

- [ ] **Step 7: Report manual UE acceptance checks**

Report exactly these editor checks without claiming the assets were edited:

- Climb/Mantle retain two Motion Warping Notify States.
- FrontLedge covers approach-to-hand-contact.
- BackLedge targets top standing, has Ignore Z Axis disabled, and extends through the last meaningful Root Motion.
- Vault Notify windows remain unchanged.
- Test multiple obstacle heights, platform depths, and entry speeds; verify endpoint Delta2D drops and the cached-landing recovery warning is absent during normal traversal.
