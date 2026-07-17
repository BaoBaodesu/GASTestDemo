# Climb 与 Mantle 墙沿抓取修复设计

## 目标

修复 `UT_TraversalComponent` 生成 Climb、Mantle Motion Warping Target 时的高度和水平位置错误，使角色双手在动作接触阶段落到障碍物顶部边缘，并保持 Vault 当前行为不变。

## 已确认根因

- 调试截图中的青色 `TopLocation` 已落在障碍物顶面，墙顶检测不是主要故障点。
- `BuildWarpTargets()` 对 Climb 使用 `TopLocation.Z`，但该目标代表动画根骨位置，缺少胶囊中心高度补偿，因此角色整体被拉得过低。
- Mantle 的 `FrontLedgeWarpTarget` 高度使用动作开始前的 `ActorLocation.Z`，不会随障碍物顶面高度变化。
- Climb、Mantle 共用 `FrontLedgeWarpOffset`，无法补偿两个 Montage 不同的根骨到手掌距离。
- 水平目标只按胶囊半径放在墙外，当前 Montage 下手掌仍会穿入墙体，需要按动作增加离墙校正。

## 设计

### 墙沿几何基准

Climb 和 Mantle 的 FrontLedge 基准统一由检测结果构建：

- XY 使用 `WallLocation` 所在墙面，并沿水平 `WallNormal` 向墙外偏移胶囊半径和现有 2 cm 安全距离。
- Z 使用 `TopLocation.Z + CapsuleHalfHeight`，把检测到的墙顶表面点转换为角色胶囊体中心目标。
- 旋转继续使用 `GetFacingWallRotation()`，保证角色面向墙壁。

Vault 继续使用动作开始时的角色中心高度，避免改变图 3 中当前可接受的 Vault 表现。

### 动作独立校正

保留 `FrontLedgeWarpOffset` 作为 Vault 的现有偏移，并新增两个可编辑偏移：

- `ClimbFrontLedgeWarpOffset`，默认 `(-10, 0, 0)` cm。
- `MantleFrontLedgeWarpOffset`，默认 `(-10, 0, 0)` cm。

偏移使用目标旋转的局部空间：X 正方向朝向墙内，因此默认 X 为负值会把角色根骨再向墙外移动 10 cm，减少手掌穿墙。Y 用于左右校正，Z 用于不同 Montage 的最终高度微调。

只对 FrontLedge 做本次修复；`BackLedgeWarpTarget`、落点、碰撞恢复和 Ability 执行结构保持不变。

### 调试显示

在现有 Traversal 调试绘制中增加最终 FrontLedge Warp Target：

- 青色球继续表示实际检测到的 `TopLocation`。
- 紫色球和朝向箭头表示最终动画根骨目标。

这样可直接判断后续偏差来自墙沿检测，还是来自特定 Montage 的根骨/手掌距离。

## 数据流

1. `DetectWall()` 获得墙面位置和法线。
2. `DetectTop()` 获得真实顶面高度。
3. `T_TraversalClimb::Detect()` 或 `T_TraversalMantle::Detect()` 确定动作类型和安全落点。
4. `BuildWarpTargets()` 根据动作类型选择墙沿基准和对应局部偏移。
5. `UT_Traversal::UpdateMotionWarpTargets()` 把结果写入 Montage 使用的 `FrontLedge`、`BackLedge` Target。

## 边界与失败处理

- `OwnerCharacter` 或胶囊组件无效时维持当前提前返回行为。
- `TopLocation` 只在 Climb/Mantle 检测成功后使用，不改变检测失败条件。
- 不修改 Montage、AnimBP、IK、GameplayTag、MovementMode 或碰撞恢复逻辑。
- 不覆盖工作区现有未提交修改。

## 测试与验收

### 自动化测试

- Climb 的 FrontLedge 高度等于墙顶高度、胶囊半高和 Climb 局部 Z 偏移之和。
- Mantle 的 FrontLedge 高度随 `TopLocation.Z` 变化，不再依赖当前 Actor 高度。
- Climb 与 Mantle 分别使用自己的局部偏移。
- Vault 仍使用动作开始时的角色中心高度和原有 `FrontLedgeWarpOffset`。
- 墙面旋转后，局部负 X 偏移仍始终指向墙外。

### 运行时验收

- Climb 接触阶段双手落在墙顶边缘上方，不再低于边缘或伸入墙体。
- Mantle 接触阶段双手落在墙顶边缘上方。
- 不同高度的障碍物上抓取高度都跟随顶面变化。
- Vault 表现与修复前一致。
- 动作完成或打断后，落点、胶囊碰撞和移动模式恢复正常。
