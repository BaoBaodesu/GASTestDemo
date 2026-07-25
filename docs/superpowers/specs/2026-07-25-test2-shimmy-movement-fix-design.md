# Test2 Shimmy 悬挂移动修复设计

## 目标

修复角色悬挂在 Bar 上时的三个问题：

- 左右 Shimmy 时角色 Z 轴下降。
- 首次点按移动键时角色发生转向。
- 悬挂状态仍能使用前后方向移动。

本轮不迁移现有 Blueprint 的 `IA_Move -> Shimmy` 输入链。

## 根因

- `UTest2Component::AlignGrab()` 在角色悬空时使用 `MOVE_Walking`。Walking 物理会执行贴地及台阶修正，不适合保持悬挂高度。
- 自动朝移动方向旋转只在 `Shimmy()` 收到输入后才关闭，首次普通移动输入可能已经进入 `CharacterMovement`。
- `AT_PlayerController::Move()` 未判断 `UTest2Component::bOnBar`，始终提交相机方向的前后和左右普通移动输入。

## 修改方案

### `UTest2Component`

- 角色进入 Bar 悬挂状态时使用 `MOVE_Flying`，保持现有 `GravityScale = 0` 和移动速度设置。
- 在进入悬挂状态时立即关闭 `bOrientRotationToMovement` 和 `bUseControllerDesiredRotation`。
- `Shimmy()` 继续使用现有 `BarMoveDirection` 和 Blueprint 调用方式，不增加新的输入入口。
- `Detach()` 继续恢复 `MOVE_Walking` 以及现有转向设置。

### `AT_PlayerController`

- `Move()` 检测角色身上的 `UTest2Component`。
- 当组件存在且 `bOnBar` 为 true 时直接返回，不再提交普通前后或左右移动输入。
- 非悬挂状态完全保留当前移动逻辑。

## 验证

增加自动化测试验证状态和输入门控：

- 进入 Bar 悬挂后移动模式为 `MOVE_Flying`。
- 进入 Bar 悬挂后自动转向关闭。
- `bOnBar` 为 true 时 Controller 普通移动不会产生 Pending Movement Input。
- 非悬挂状态普通移动仍正常提交。

编译通过后，在 PIE 中手动验证：

- 持续左右移动时角色 Z 高度保持不变。
- 首次点按左右键不会使角色转向。
- 前后键不产生位移。
- `Detach()` 后角色恢复正常行走和转向。

## 范围限制

- 不修改 `BP_TestCharacter` 的输入节点。
- 不把 Shimmy 调用迁移到 `AT_PlayerController`。
- 不重构 `UTest2Component` 的抓取、攀爬或跳离逻辑。
