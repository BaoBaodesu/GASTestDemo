# Climb/Mantle 顶部站立目标与碰撞恢复修复设计

## 状态与范围

本设计替代 `2026-07-17-traversal-climb-mantle-grip-design.md` 中把 Climb/Mantle FrontLedge 统一视为站立胶囊中心的旧结论。本次只修改现有两段 Motion Warping 和结束恢复相关代码，不重构 Traversal，不新增第三个 Warp Target，不修改 Montage 资产，不提交 Git commit。

## 目标

- Vault 的两段 Motion Warping 行为保持不变。
- Climb、Mantle 的 `BackLedge` 直接表示 `TraversalResult.LandingLocation`，不叠加通用 `BackLedgeWarpOffset`。
- 正常完成时完整胶囊不与平台正面或顶面产生 Blocking Overlap。
- 缓存落点传送只作为真实穿透时的异常兜底。
- 正常完成不清零速度，不产生额外停顿。

## 已确认现状

- `BuildWarpTargets()` 当前给所有动作的第二目标叠加 `BackLedgeWarpOffset`。
- Climb FrontLedge 已使用 `TopLocation.Z - ClimbRootToLedgeHeight`。
- Mantle FrontLedge 仍使用 `TopLocation.Z + CapsuleHalfHeight`，把抓边根骨目标误作站立胶囊中心。
- `AM_GettingUp` 引用启用 Root Motion 的 `Anim_GettingUp`，并包含 `FrontLedge`、`BackLedge` 窗口；资产没有墙沿参考或手部接触标记，无法可靠推导精确的 Mantle Root-to-Ledge 数值。
- `RestoreCharacterState()` 已只在取消时清零速度。
- `OnBlendOut` 当前仍绑定正常完成回调，会在 Montage 真正完成前收尾。

## 设计

### 第二 Warp Target

`BuildWarpTargets()` 按动作类型计算 `BackLedgeLocation`：

- Vault：`FarEdgeLocation + rotated BackLedgeWarpOffset`。
- Climb：`LandingLocation`。
- Mantle：`LandingLocation`。
- 默认分支：`LandingLocation`。

保留 `FrontLedge`、`BackLedge` 的结构和名称。

### Mantle FrontLedge

新增可编辑属性：

```cpp
float MantleRootToLedgeHeight = 0.f;
```

默认值 `0` 是中性值，不复用 Climb 的 `132cm`，也不伪造无法从资产确定的精确值。计算为：

```cpp
FrontLedgeLocation.Z =
    TraversalResult.TopLocation.Z - MantleRootToLedgeHeight;
FrontLedgeLocation +=
    TargetRotationQuaternion.RotateVector(MantleFrontLedgeWarpOffset);
```

用户暂时不需要按接触帧手工调整；现有 `MantleFrontLedgeWarpOffset` 仍保留未来微调能力。

### 结束与碰撞恢复

- 正常完成分支开始时记录 Action、Current、Landing、Dist2D、DeltaZ。
- 当前完整胶囊安全时直接恢复碰撞。
- 当前水平位置下方有安全地面时，只允许现有最大距离内的垂直修正。
- 当前实际位置穿透时，才回退缓存 `LandingLocation`，并保留既有兜底警告。
- 中断且当前位置不安全时回退 `TraversalStartLocation`。
- 不扩大校正距离，不缩小最终验证胶囊，不把传送变成正常路径。
- 正常完成只响应 `OnCompleted`；`OnBlendOut` 不结束 Ability。取消和中断继续走取消路径。
- Traversal 开始时保留 `StopMovementImmediately()`；正常完成不清零速度；取消时可以清零。

### 调试颜色

- `IsCapsuleLocationClear()` 的检测胶囊保持 Red/Green 表示阻挡/安全。
- `DrawTraversalDebug()` 中只表示 Landing 目标的完整胶囊改为 Cyan，尺寸和持续时间不变。

## 测试策略

先修改自动化测试并确认旧实现失败，再修改生产代码：

1. 使用非零 `BackLedgeWarpOffset`，验证 Vault 第二目标仍叠加偏移。
2. 使用同一非零偏移，验证 Climb 和 Mantle 第二目标严格等于 `LandingLocation`。
3. 验证 `MantleRootToLedgeHeight` 存在且默认 `0`。
4. 验证 Mantle FrontLedge 不再包含胶囊半高，并继续应用 Mantle 局部偏移。
5. 保留当前水平位置安全落点测试，确认仅修正 Z。
6. 运行 Traversal 自动化测试、UHT 和项目编译；不处理无关警告。

## UE 编辑器人工验收

- Climb、Mantle 保持 `FrontLedge` 和 `BackLedge` 两个 Notify State。
- `FrontLedge` 覆盖接近并抓住墙沿阶段。
- `BackLedge` 实际表示 TopStanding，目标为 `LandingLocation`，关闭 Ignore Z Axis。
- `BackLedge` 覆盖到最后一段有效水平/垂直 Root Motion 结束；窗口后不能残留明显 Root Motion。
- Vault 两个窗口和时间范围保持原样。
- 对多种障碍高度、平台厚度和进入速度测试 Climb/Mantle；观察新增终点日志，并确认正常路径不出现缓存落点恢复警告。

## 验收标准

- Vault 行为不变。
- Climb/Mantle 的动画终点与安全落点 Delta2D 明显缩小。
- 正常完成不依赖最终 `SetActorLocation` 回退缓存落点。
- 完整胶囊恢复后不与平台边缘 Blocking Overlap。
- 正常完成保持移动连续性，取消行为保持安全。
