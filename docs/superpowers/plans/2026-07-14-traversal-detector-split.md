# Traversal 检测器拆分 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 将 Traversal 的四种动作检测拆分为独立检测器，并让 Z=200 cm 的窗口下沿能走 Vault 路径。

**Architecture:** `UT_TraversalComponent` 继续持有角色、胶囊、配置和 Debug；它只负责按 Catch、Vault、Mantle、Climb 的优先级调度检测器、保存结果并返回给 GAS。四个无反射 C++ 检测器通过只读组件接口执行几何检测，写入同一个 `FTraversalCheckResult`。

**Tech Stack:** Unreal Engine 5.7、C++、Automation Test、Gameplay Ability System、Motion Warping。

## Global Constraints

- 保持 `UT_TraversalComponent::DetectTraversal(FTraversalCheckResult&)` 和 `UT_Traversal` 的调用接口不变。
- 只修改 Traversal 相关 C++ 文件；不触碰现有未提交的资源与无关源码。
- 保留现有 UPROPERTY 参数和中文 UTF-8 注释；不新增蓝图资产。
- `MaximumVaultHeight` 默认值为 230.f，确保 Z=200 cm 窗口下沿位于 Vault 可检测范围。

---

### Task 1: 建立 Vault 条件的回归测试

**Files:**
- Create: `Source/GASTestDemo1/Private/Tests/T_TraversalVaultTests.cpp`
- Create: `Source/GASTestDemo1/Public/Player/Traversal/T_TraversalVault.h`

**Interfaces:**
- Produces: `T_TraversalVault::CanVault(float ObstacleHeight, float ObstacleDepth, bool bHasFarEdge, bool bHasLandingSpace, float MinimumHeight, float MaximumHeight, float MaximumDepth)`。

- [ ] **Step 1: 写失败测试**

```cpp
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTTraversalVaultWindowHeightTest,
	"GASTestDemo1.Traversal.Vault.WindowHeight",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTTraversalVaultWindowHeightTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("Z=200 的窗口下沿可以 Vault"),
		T_TraversalVault::CanVault(200.f, 80.f, true, true, 30.f, 230.f, 100.f));
	TestFalse(TEXT("没有落点不能 Vault"),
		T_TraversalVault::CanVault(200.f, 80.f, true, false, 30.f, 230.f, 100.f));
	return true;
}
```

- [ ] **Step 2: 运行测试确认失败**

Run: 在编辑器 Automation 面板运行 `GASTestDemo1.Traversal.Vault.WindowHeight`。

Expected: 编译失败，提示 `T_TraversalVault` 或 `CanVault` 尚未定义。

- [ ] **Step 3: 添加最小接口**

```cpp
class GASTESTDEMO1_API T_TraversalVault
{
public:
	static bool CanVault(float ObstacleHeight, float ObstacleDepth,
		bool bHasFarEdge, bool bHasLandingSpace, float MinimumHeight,
		float MaximumHeight, float MaximumDepth);
};
```

- [ ] **Step 4: 运行测试确认通过**

Run: 再次运行 `GASTestDemo1.Traversal.Vault.WindowHeight`。

Expected: 两个断言通过。

### Task 2: 提取 Vault 检测与专用顶面追踪

**Files:**
- Create: `Source/GASTestDemo1/Private/Player/Traversal/T_TraversalVault.cpp`
- Modify: `Source/GASTestDemo1/Public/Player/Traversal/T_TraversalVault.h`
- Modify: `Source/GASTestDemo1/Public/Player/Components/T_TraversalComponent.h`
- Modify: `Source/GASTestDemo1/Private/Player/Components/T_TraversalComponent.cpp`

**Interfaces:**
- Consumes: `UT_TraversalComponent` 的公共只读检测接口与 `FTraversalCheckResult`。
- Produces: `bool T_TraversalVault::Detect(const UT_TraversalComponent&, FTraversalCheckResult&)`。

- [ ] **Step 1: 令 `T_TraversalVault::Detect` 依次完成墙面、`DetectVaultTop`、厚度、窗洞胶囊净空、后方落点和 Warp Target 填充。**

```cpp
if (!Component.DetectWall(WallHit) ||
	!Component.DetectVaultTop(WallHit, TopHit))
{
	return false;
}

return CanVault(Result.ObstacleHeight, Result.ObstacleDepth,
	Result.bHasFarEdge, Result.bHasVaultLandingSpace,
	Component.GetMinimumTraversalHeight(), Component.GetMaximumVaultHeight(),
	Component.GetMaximumVaultDepth());
```

- [ ] **Step 2: 将 `UT_TraversalComponent::DetectVaultTop` 实现为 Vault 检测器的薄转发入口；实际追踪从 `MaximumVaultHeight + VaultTopTraceExtraHeight` 向下到 `MinimumTraversalHeight`，命中法线与高度均合法才返回 true。**

- [ ] **Step 3: 在 Component 中补充 `VaultTopTraceExtraHeight` 的 UPROPERTY，默认 20.f，并把 `MaximumVaultHeight` 改为 230.f。**

- [ ] **Step 4: 运行 Vault Automation 测试。**

Run: `GASTestDemo1.Traversal.Vault.WindowHeight`。

Expected: PASS。

### Task 3: 提取 Mantle、Climb、Catch 检测器

**Files:**
- Create: `Source/GASTestDemo1/Public/Player/Traversal/T_TraversalMantle.h`
- Create: `Source/GASTestDemo1/Private/Player/Traversal/T_TraversalMantle.cpp`
- Create: `Source/GASTestDemo1/Public/Player/Traversal/T_TraversalClimb.h`
- Create: `Source/GASTestDemo1/Private/Player/Traversal/T_TraversalClimb.cpp`
- Create: `Source/GASTestDemo1/Public/Player/Traversal/T_TraversalCatch.h`
- Create: `Source/GASTestDemo1/Private/Player/Traversal/T_TraversalCatch.cpp`

**Interfaces:**
- Produces: 三个同形接口 `static bool Detect(const UT_TraversalComponent&, FTraversalCheckResult&)`。

- [ ] **Step 1: Mantle 检测器使用普通顶面与站立空间，在 `MaximumMantleHeight` 内写入 `ActionType = Mantle`、落点和 Warp Target。**
- [ ] **Step 2: Climb 检测器使用普通顶面与站立空间，在 `MaximumClimbHeight` 内写入 `ActionType = Climb`、落点和 Warp Target。**
- [ ] **Step 3: Catch 检测器仅在角色下落、顶面高度位于 Catch 范围且悬挂胶囊空间可用时写入 `ActionType = Catch` 和 Catch Warp Target。**
- [ ] **Step 4: 在编辑器 Automation 面板运行 Vault 测试，确认新增检测器不影响其通过。**

### Task 4: 收敛 Component 调度与统一 Debug

**Files:**
- Modify: `Source/GASTestDemo1/Public/Player/Components/T_TraversalComponent.h`
- Modify: `Source/GASTestDemo1/Private/Player/Components/T_TraversalComponent.cpp`

**Interfaces:**
- Consumes: 四个检测器的 `Detect` 接口。
- Produces: 原有 `DetectTraversal` 接口和原有 GAS 结果结构。

- [ ] **Step 1: 将 `DetectTraversal` 收敛为缓存检查、依优先级调用 `T_TraversalCatch`、`T_TraversalVault`、`T_TraversalMantle`、`T_TraversalClimb`、统一 `BuildWarpTargets` 和统一 Debug。**

```cpp
if (T_TraversalCatch::Detect(*this, OutTraversalResult) ||
	T_TraversalVault::Detect(*this, OutTraversalResult) ||
	T_TraversalMantle::Detect(*this, OutTraversalResult) ||
	T_TraversalClimb::Detect(*this, OutTraversalResult))
{
	DrawTraversalDebug(OutTraversalResult);
	return true;
}
return false;
```

- [ ] **Step 2: 删除 Component 中已迁移的 Vault/Mantle/Climb/Catch 条件分支，只保留公共 Trace、胶囊净空、Warp、Debug 与参数访问。**
- [ ] **Step 3: 编译 `GASTestDemo1Editor Win64 Development` 并运行 Vault Automation 测试。**

Expected: 编译成功；`GASTestDemo1.Traversal.Vault.WindowHeight` 通过。

### Task 5: 编辑器内验证

**Files:**
- Modify: 无

- [ ] **Step 1: 在 TestMap 对准 Z=200 cm 窗口下沿，开启 Traversal Debug 后按跳跃。**
- [ ] **Step 2: 确认结果为 Vault，且 Front、Back、Land Warp Target 按窗口下沿、后沿与墙后落点绘制。**
- [ ] **Step 3: 对普通矮墙、可站立平台和下落抓边分别确认仍进入 Vault、Mantle/Climb、Catch。**
