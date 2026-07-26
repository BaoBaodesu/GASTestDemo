# Test2 Wall 横移与攀爬输入门控设计

## 目标

- Wall 抓握状态下，左右输入可以沿墙面水平移动。
- 执行 `LedgeJump()` 攀爬后，持续按住左右键也不会再次触发横移动作。
- 保留 Wall 当前的 `MOVE_None`，不修改任何 `SetMovementMode`。
- 不改变 Bar 当前通过 `AddMovementInput()` 横移的逻辑。

## 根因

1. `UTest2Component::Shimmy()` 当前要求 `bOnBar == true`，因此 Wall 抓握状态会直接返回。
2. Wall 使用 `MOVE_None`，不能通过 `AddMovementInput()` 驱动 `CharacterMovement`。
3. `LedgeJump()` 开始后到 Montage 结束前仍保持抓握状态，持续触发的 Move 输入会再次进入 `Shimmy()`；单次调用 `StopMovementImmediately()` 不能阻止后续输入。

## 设计

### Wall 横移

- `Shimmy()` 同时接受 Wall 和 Bar 抓握状态。
- Bar 保留现有 `AddMovementInput()`。
- Wall 使用沿墙切线方向的带 Sweep 水平位移，不改变 MovementMode。
- Wall 检测使用与墙面抓握高度匹配的检测偏移；目标方向没有墙面时立即停止。
- Wall 位移时同步设置水平速度，供现有动画状态读取；输入归零或检测失败时立即清零速度。

### 横移门控

- 复用现有 `bCanMove`，不新增状态变量。
- `AlignGrab()` 开始对齐时维持 `bCanMove = false`。
- `Transition()` 到达抓握目标并停止对齐 Timer 时，将 `bCanMove` 设置为 `true`。
- `Shimmy()` 在 `bCanMove == false` 时立即停止并返回。
- `LedgeJump()` 成功进入攀爬流程时立即设置 `bCanMove = false`，同时清除当前移动速度。
- `Detach()` 保留当前状态恢复逻辑。

## 测试

1. Wall、`MOVE_None`、对齐完成且侧方存在墙面时，`Shimmy(1.0)` 应改变角色水平位置。
2. `bCanMove == false` 时调用 `Shimmy()`，角色位置和速度都不应继续变化。
3. 现有 Bar 横移、松键停止和边缘立即停止测试必须继续通过。
4. UE 5.7 Development Editor 编译成功。

## 不在本次范围

- 不调整 Wall 或 Bar 的抓握距离、高度。
- 不修改 Montage、Motion Warping、AnimBP 状态机。
- 不修改输入资产和 Controller 绑定。
- 不修改任何 `SetMovementMode`。
