# Traversal 检测器拆分设计

## 目标

将 Vault、Mantle、Climb、Catch 的几何检测从 `UT_TraversalComponent` 中拆出。组件保留统一入口、动作优先级、最终结果、Debug 绘制和 GAS 返回职责。

## 架构

`UT_TraversalComponent::DetectTraversal` 缓存角色与胶囊信息后，按 Catch、Vault、Mantle、Climb 的优先级调用检测器。首个成功检测器填充 `FTraversalCheckResult` 并返回；组件随后统一绘制该结果的 Debug 信息。

检测器使用无反射的 C++ 类：`T_TraversalVault`、`T_TraversalMantle`、`T_TraversalClimb`、`T_TraversalCatch`。它们接收组件提供的检测上下文和现有参数，不创建 UObject，不引入新的蓝图资产或生命周期管理。

## Vault

`T_TraversalVault` 负责墙面、Vault 顶面、前沿/后沿、厚度、窗洞通行空间、墙后落点、Vault 条件和 Front/Back/Land Warp Transform。Vault 最大高度的默认值调整到 230 cm，使下沿 Z=200 cm 的窗口可以使用现有 Vault 蒙太奇路径。

Vault 顶面追踪从 `MaximumVaultHeight + VaultTopTraceExtraHeight` 上方开始，向 `MinimumTraversalHeight` 向下检测，仅接受范围内且法线满足 `MinimumTopNormalZ` 的顶面。`UT_TraversalComponent::DetectVaultTop` 保留为兼容入口并转发给 `T_TraversalVault`，不保留 Vault 几何逻辑。

## 其他动作

`T_TraversalMantle` 负责中等高度墙顶和站立空间；`T_TraversalClimb` 负责更高平台和站立空间；`T_TraversalCatch` 负责下落状态的边缘与悬挂胶囊空间。它们继续使用现有 `FTraversalCheckResult`，不改变 GAS Ability 的调用接口或 Motion Warping Target 名称。

## 验证

为每个检测器的动作条件建立可自动执行的测试，至少覆盖 Vault 的 200 cm 窗口下沿。完成后编译 `GASTestDemo1Editor`，并在 TestMap 中打开 Debug，确认结果动作、Warp Target 与现有 Ability 路径一致。
