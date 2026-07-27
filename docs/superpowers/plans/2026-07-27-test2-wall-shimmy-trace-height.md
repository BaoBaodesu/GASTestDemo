# Test2 Wall Shimmy Trace Height Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Wall 抓取状态使用匹配墙沿的侧方检测高度，同时保持 Bar 固定使用角色位置上方 `145.0`。

**Architecture:** 继续复用 `UTest2Component::Shimmy(double Direction)` 的现有检测与移动链路，只根据 `bOnBar` 选择检测高度。Wall 高度由 `TopImpactPoint.Z - Character->GetActorLocation().Z` 得到，不增加新的生产状态。

**Tech Stack:** Unreal Engine 5.7 C++、CharacterMovement、Automation Tests

## Global Constraints

- 仅修改 `UTest2Component::Shimmy()` 的侧方检测高度。
- Bar 分支继续固定使用 `145.0`。
- 不修改 MovementMode、输入路由、AnimBP 或抓取状态结构。
- 使用 UTF-8 中文注释并保持现有编码习惯。

---

### Task 1: 区分 Wall 与 Bar 的侧方检测高度

**Files:**
- Modify: `Source/GASTestDemo1/Private/Tests/Test2ShimmyTests.cpp`
- Modify: `Source/GASTestDemo1/Private/Player/Components/Test2Component.cpp:378`

**Interfaces:**
- Consumes: `UTest2Component::bOnBar`、`UTest2Component::TopImpactPoint`
- Produces: `UTest2Component::Shimmy(double Direction)` 的 Wall/Bar 检测高度分支

- [ ] **Step 1: Write the failing test**

新增 Wall 测试，将可抓取墙面放在 `TopImpactPoint.Z - ActorLocation.Z` 高度，并断言 `Shimmy(1.0)` 能产生沿墙移动输入。

- [ ] **Step 2: Run test to verify it fails**

运行 `GASTestDemo1.Test2.Shimmy.WallUsesGrabHeight`，确认旧实现因固定 `+145` 检测不到墙面而失败。

- [ ] **Step 3: Write minimal implementation**

```cpp
const FVector TraceStart =
	Character->GetActorLocation() +
	SideDirection * 40.0f +
	FVector(
		0.0f,
		0.0f,
		bOnBar
			? 145.0f
			: TopImpactPoint.Z - Character->GetActorLocation().Z);
```

- [ ] **Step 4: Run test to verify it passes**

重新构建并运行 Wall 目标测试以及现有 Bar Shimmy 测试，确认两种检测高度均生效。

- [ ] **Step 5: Review scope**

检查最终差异只包含测试覆盖和 `TraceStart` 高度分支，不修改其他行为。
