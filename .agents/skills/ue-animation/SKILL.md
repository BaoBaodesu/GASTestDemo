---
name: ue-animation
description: 处理 Unreal Engine 5 的 Animation Blueprint、AnimInstance、Montage、Notify、Root Motion、Motion Warping、动画分层、IK、Control Rig 和网络动画同步时使用。用于新增或修改动画逻辑，以及排查动画不播放、Slot 不生效、Notify 异常、Root Motion 原地不动、Motion Warping 对位错误等问题。本技能与 ue5-cpp、ue-debug 配合使用：动画相关的 C++ 同样遵守那两个技能的规则。
---

# UE Animation Rules

## 核心原则：权威逻辑不能仅依赖动画回调

动画系统主要负责姿势、混合、Root Motion 和表现事件。伤害、资源消耗、物品生成等权威结果必须由服务器游戏逻辑裁决。

Anim Notify 可以作为触发入口，但关键逻辑不能假设 Notify 在所有端、所有优化配置和所有中断路径下必然执行。使用服务器 Notify 时，必须确认：

- Skeletal Mesh 在服务器上会更新动画
- Notify 启用了 Trigger on Dedicated Server
- 没有被 LOD、Sync Group、Trigger Chance 或运行时过滤
- Montage 中断和 Ability 取消路径有独立收尾

GAS 的 `WaitGameplayEvent` 如果由 Anim Notify 发送 Gameplay Event，也仍然依赖 Notify 是否执行，不能把它当作绕开 Notify 的方案。

贯穿始终的分工：

- **游戏逻辑驱动动画**，而不是动画驱动游戏逻辑
- 动画层向外**发事件**，由游戏侧决定做什么、在哪一端做
- 需要与动画时间同步的权威逻辑，确保服务器端动画确实更新并正确配置 Notify，或者由服务器可控的方式（定时器、Ability Task）独立计时

---

## 1. AnimInstance 与 Animation Blueprint

### 线程模型

- `NativeUpdateAnimation` 和 Event Blueprint Update Animation 在游戏线程运行
- 启用了多线程动画更新并满足并行条件时，AnimGraph 和线程安全更新逻辑可以在工作线程执行
- 使用 Root Motion From Everything 或 Root Motion From Montages Only 时，动画图可能在游戏线程更新，不要假设它始终在工作线程
- C++ 需要线程安全动画更新时，使用 `NativeThreadSafeUpdateAnimation`，不要把它与 `NativeUpdateAnimation` 混为一谈

线程安全函数中的数据访问规则：

- 优先使用 Property Access 从相关对象安全复制属性
- 不直接调用非线程安全的 Actor、Controller、World 或组件函数
- 不执行 Trace、SpawnActor、修改组件或游戏状态

**在 `NativeUpdateAnimation` 里把外部数据拉进 AnimInstance 成员变量**，AnimGraph 只读这些成员：

```cpp
void UMyAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
    Super::NativeUpdateAnimation(DeltaSeconds);

    // 编辑器预览窗口里 Owner 为空，必须判断
    ACharacter* Character = Cast<ACharacter>(TryGetPawnOwner());
    if (!IsValid(Character)) { return; }

    // 只在这里读取游戏状态，写入自己的成员；AnimGraph 后续只读成员
    Speed = Character->GetVelocity().Size2D();
    bIsFalling = Character->GetCharacterMovement()->IsFalling();
}
```

### 结构约定

- **变量在 C++ 基类声明，图表在蓝图里连**。C++ 侧用 `UPROPERTY(BlueprintReadOnly, Category = "...")`，蓝图子类只做状态机和混合
- 一个 `USkeletalMeshComponent` 对应一个 AnimInstance 实例。Linked Anim Instance（子动画实例）用于按功能拆分，避免单个 AnimBP 膨胀
- `TryGetPawnOwner()` 在编辑器预览、初始化早期、Owner 销毁后都可能为空——所有访问点都要判
- 不要在 AnimBP 里写游戏逻辑。需要通知游戏侧时用委托或 Gameplay Event

### 性能

- 远处角色开 Update Rate Optimization（URO），降低更新频率
- `VisibilityBasedAnimTickOption` 决定不可见时是否更新姿势——它同时影响 Notify 是否触发（见第 3 节）
- 复杂计算尽量放线程安全路径，让它能并行

---

## 2. Montage 与 Slot

### 播了看不见，先查 Slot

这是出现频率最高的问题，且不会报错。Montage 播放在某个 **Slot** 上，只有当 AnimGraph 里存在对应名字的 **Slot 节点**、且该节点处在最终输出的路径上时，动画才会出现。

排查顺序：

1. Montage 资产里配的 Slot 名（在 Montage 编辑器的 Slot 轨道上）
2. AnimGraph 里有没有同名 Slot 节点
3. 那个 Slot 节点是否真的连到了最终 Pose 输出（有没有被某个分支绕过去）
4. 是否被后续节点覆盖（比如放在 Layered Blend 的下半身分支，却播的是上半身动画）

### Slot Group 与打断

Slot 归属 Slot Group（默认 `DefaultGroup`）。**同一 Group 内的 Montage 互相打断**，不同 Group 可以同时播。所以上半身攻击 + 下半身移动这种需求，要把上半身 Slot 放到独立的 Group 里，否则会互相顶掉。

### 播放与回调

```cpp
// 直接播，返回时长；0 表示播放失败
const float Duration = AnimInstance->Montage_Play(MontageToPlay, PlayRate);
```

拿结束回调时区分情况：

- **Completed**：正常播放并完全混出
- **BlendOut**：开始正常混出（早于 Completed）
- **Interrupted**：被同 Group 的另一个 Montage 或流程打断

不要只绑 Completed 就假设逻辑一定会走到——被打断时它不会触发，配套的状态清理会永久卡住。

GAS 环境下用 `PlayMontageAndWait` Ability Task，至少检查四条结束路径：OnCompleted、OnBlendOut、OnInterrupted、**OnCancelled**（Ability 被取消时触发，不等同于普通的 Interrupted）。遗漏 OnCancelled 可能导致 Busy Tag 未清理、碰撞窗口未关闭、EndAbility 未调用。

需要"必定清理"的状态不能只放在其中一个回调，应集中到统一的结束函数，并保证 EndAbility / CancelAbility 路径都会调用，同时避免重复 EndAbility。

分段控制用 `Montage_JumpToSection` / `Montage_SetNextSection`（连招常用），而不是重新 Play。

### 网络

**Montage 播放不会自动复制。** 处理方式按环境选：

- **GAS**：用 `PlayMontageAndWait`，在 `LocalPredicted` 技能里它会处理本地预测 + 其他端同步
- **非 GAS**：Montage 不会自动把完整播放状态同步到其他端，需要手动处理：
  - 短暂、可丢失的纯表现可以通过 Unreliable NetMulticast 触发
  - 对晚加入、重新相关、动画中途同步有要求时，应复制明确的动画状态（Montage 引用、Section、起始时间）或 Gameplay 状态，而不是只依赖一次 Multicast
  - 拥有者客户端上避免重复播（本地已经先播过了，用 `IsLocallyControlled()` 判断跳过）
  - 命中、伤害、资源消耗等判定始终由服务器游戏逻辑执行，不通过 Reliable Montage RPC 保证
  - Root Motion Montage 需要结合 ACharacter / CharacterMovement 的网络 Root Motion 支持验证

---

## 3. Notify 与 NotifyState

### Notify 不执行时的排查项

Notify 不触发时不要先归因于帧率，依次检查：

1. Skeletal Mesh 当前是否 Tick Pose
2. `VisibilityBasedAnimTickOption` 的设置（专用服务器上没有渲染，若未正确配置，骨骼可能不更新）
3. Dedicated Server 上是否启用了 Trigger on Dedicated Server
4. Sync Group 中当前动画是 Leader 还是 Follower
5. Notify Trigger Chance、Trigger Weight Threshold 和 LOD Filter
6. Montage 是否在 Notify 前被打断、跳 Section 或开始混出
7. URO / Animation Budget Allocator 是否停止了该 Mesh 的更新
8. Motion Matching 或 AnimGraph 是否启用了 Notify Filtering

Notify 可以作为触发入口（见核心原则），但关键逻辑不能假设 Notify 在所有配置下必然执行。伤害判定、生成投射物、消耗资源等权威逻辑，要么确保服务器端满足上述所有条件，要么用服务器可控的方式独立计时（GAS Ability Task、服务器定时器）。

### 多端重复执行

Notify 在**每个执行动画更新的端上都会触发**。所以 Notify 里写 `SpawnActor` 或改属性，会在服务器和客户端各跑一遍。规则：

- 特效、音效、镜头抖动 → 直接在 Notify 里做，本来就该每端各播各的
- 任何改游戏状态的事 → 要么不放在这里，要么加 `HasAuthority()` 守卫（但要接受服务器可能不触发的风险，见上）

### 写法要点

```cpp
void UMyAnimNotify::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
    const FAnimNotifyEventReference& EventReference)
{
    Super::Notify(MeshComp, Animation, EventReference);

    // 在 Persona 预览窗口里 Owner 为空，必须判断，否则一打开动画编辑器就崩
    if (!IsValid(MeshComp) || !IsValid(MeshComp->GetOwner())) { return; }

    // ...
}
```

两条硬要求：

- **Notify 对象是资产上的共享实例，不是每次播放新建的。** 绝不要在 Notify 类里存播放状态（计数、目标缓存），多个角色同时播同一动画会互相踩踏
- 预览窗口没有 Owner、没有真实 World，所有访问都要判空

### AnimNotifyState

有 `NotifyBegin` / `NotifyTick` / `NotifyEnd` 三个回调，用于持续窗口（无敌帧、武器碰撞开关、位移窗口）。

`NotifyEnd` 通常会在 Notify State 离开活动范围时执行，但不能把它当作所有异常路径上的绝对保证。Montage 中断、Ability 取消、Mesh/Owner 销毁、AnimInstance 重建和关卡切换都可能导致 End 未按预期调用。

"开启后必须关闭"的状态应在统一的清理函数中处理，并从以下路径调用：

- NotifyEnd
- Montage Interrupted / Cancelled
- Ability EndAbility / CancelAbility
- Actor 或 Component EndPlay

清理函数必须允许重复调用（幂等），避免多个结束路径造成二次操作。

`NotifyTick` 每帧调用，开销敏感，不要在里面做重活。

---

## 4. Layered Blend 与分层

上下半身分离的标准做法是 `Layered blend per bone`，从某根骨骼（通常 `spine_01`）开始用上半身姿势。

要点：

- 简单的单骨骼分层可以使用 Branch Filter；需要复用、精细权重或多骨骼控制时使用 Blend Mask。两者按项目现有结构选择，不为了形式统一强制迁移
- 局部空间混合出现肩膀、脊柱或瞄准旋转扭曲时，尝试启用 **Mesh Space Rotation Blend**；不要无条件开启，修改后要比较姿势结果和性能
- **叠加动画（Additive）** 要在资产里正确设置 Additive Anim Type 和 Base Pose，用 `Apply Additive` / `Apply Mesh Space Additive` 节点。Base Pose 选错会导致姿势整体偏移
- **Slot 节点的位置决定它受不受影响**：上半身 Montage 的 Slot 要放在上半身分支里，放错位置会被分层节点整个覆盖掉
- 分层节点可能覆盖根骨骼，导致 Root Motion 失效（见下节）

---

## 5. Root Motion

### 原地不动时按这个顺序查

1. 动画资产里勾了 **Enable Root Motion** 吗
2. **先读取 AnimInstance / AnimBP 当前的 Root Motion Mode**，不要假设项目用的是哪种。多人 Character 通常使用 `Root Motion from Montages Only`（AnimGraph 里的动画在此模式下不提取 Root Motion）；`Root Motion from Everything` 更适合单机或无需网络同步的动画图逻辑
3. Montage 的 **Slot 是否在 Root Motion 的提取路径上**，有没有被分层节点截断
4. 用的是 **ACharacter + CharacterMovementComponent** 吗——普通 Pawn 没有内建的 Root Motion 位移处理
5. 动画本身的根骨骼有位移吗（有些资产的位移烘在了 pelvis 上）

注意：启用 Root Motion From Everything 或 Root Motion From Montages Only 后，动画图可能转为游戏线程更新（见第 1 节线程模型），修改 Root Motion Mode 时同时评估动画性能。

### 网络

`ACharacter` 对 **Montage 上的 Root Motion** 有内建的网络支持（服务器发送 Root Motion 快照给模拟端），这也是官方推荐把 Root Motion 放在 Montage 而非状态机里的原因之一。

- 模拟端不是逐帧还原，而是按快照插值，所以精确对位不要依赖模拟端表现
- Root Motion 期间玩家输入通常应被限制，避免和位移打架
- 需要位移可被服务器校验的场景，优先用 Root Motion Source 而不是直接 `SetActorLocation`

---

## 6. Motion Warping

用于让 Root Motion 动画对准实际目标（翻越、处决、爬墙对位）。依赖 `MotionWarping` 插件和 `UMotionWarpingComponent`。

**必须配合 Root Motion 使用**，非 Root Motion 动画上它不起作用。

### 没效果时先查名字

Montage 上用 `AnimNotifyState_MotionWarping` 标出窗口，窗口里配置的 **Warp Target Name** 必须和代码里设置目标时用的名字完全一致：

```cpp
// 名字必须与 Montage 上 Notify State 里配置的一致，对不上就是静默不生效
MotionWarpingComponent->AddOrUpdateWarpTargetFromLocationAndRotation(
    FName("AttackTarget"), TargetLocation, TargetRotation);
```

其他要点：

- **静态目标**：优先在 Montage 播放前设置 Warp Target。最迟必须在对应 Motion Warping Window 开始求值前设置
- **移动目标**：可以在窗口期间持续 `AddOrUpdateWarpTarget` 更新，但服务器和预测客户端必须使用一致或可校正的数据来源。不要让客户端和服务器各自用不确定的 Trace 独立得出关键落点
- 目标生命周期结束后删除（`RemoveWarpTarget`），或在下一次使用前明确覆盖，残留目标会影响后续播放
- **网络下服务器和客户端都要设置同样的 Warp Target**，只在一端设会导致位置不一致。在 `LocalPredicted` 技能里两端各自计算，或者由服务器把目标复制下去
- 目标点要做合法性校验（可达、不穿墙），Motion Warping 只负责对位，不做碰撞判断

---

## 7. IK

### 分工

- **计算在游戏线程做**：脚部落点的射线检测放 `NativeUpdateAnimation`，结果写进 AnimInstance 成员
- **应用在 AnimGraph 做**：Two Bone IK / FABRIK / Full Body IK 节点读那些成员

绝不要在 AnimGraph 或线程安全函数里发射线。

### 脚部 IK 的常规做法

1. 从脚骨位置向下射线，拿到落点高度和地面法线
2. 取两脚中较低的一侧，把 Pelvis 整体下沉相应距离
3. 各脚用 Two Bone IK 拉到落点，并按法线旋转脚掌
4. 所有偏移做插值，避免跳变

注意**坐标空间**：Two Bone IK 的 Effector / Joint Target 有 Component Space、Bone Space、World Space 之分，选错是最常见的"IK 乱飞"原因。

### UE5 的工具

- **IK Rig**：定义骨架的 IK 解算结构，也是 IK Retargeter 的基础
- **IK Retargeter**：跨骨架重定向动画，替代旧的 Retarget Manager 流程
- **Full Body IK**：整体解算，适合复杂交互，但开销明显高于 Two Bone IK，不要无脑套在所有角色上

---

## 8. Control Rig

用途分三类：

- **制作期**：在 Sequencer 里做动画、做程序化的姿势调整

### AnimGraph Control Rig 节点

- 把游戏状态在游戏线程读取后，通过暴露变量传入 Control Rig
- 不在 RigVM 求值过程中执行 World 查询、射线、SpawnActor 或游戏状态修改
- 适合运行时姿势修正、接触点对齐和复杂 IK
- 也可以挂成 **Post Process Anim BP**，对所有使用该骨架的角色统一生效（常用于道具/装备对齐）
- 开销明显，人群角色上慎用；能用普通 AnimGraph 节点解决的不要上 Control Rig

### Control Rig Component

- 可以由 Actor Blueprint 和游戏逻辑驱动
- 用于将场景组件映射到 Control Rig、设置控制值或驱动非 Skeletal Mesh 对象
- 即使由 Gameplay 驱动，也要明确更新时机，避免与 AnimBP 同时写同一骨骼或控制产生冲突

---

## 常见问题速查

| 现象 | 优先怀疑 |
|---|---|
| Montage 播了没反应 | Slot 名不匹配 / AnimGraph 里没有该 Slot 节点 / Slot 不在输出路径上 |
| 动画在其他玩家身上看不到 | Montage 没有复制，缺 Multicast 或没走 GAS 的 Task |
| 上半身动画顶掉了下半身 | Slot 节点位置放错 / Slot Group 相同导致互相打断 |
| Notify 在服务器不触发 | 按第 3 节排查项逐条检查：Tick Pose、VisibilityBasedAnimTickOption、Trigger on Dedicated Server、Sync Group、URO |
| Notify 效果播了两遍 | 多端各触发一次，或本地已播又收到 Multicast |
| Root Motion 原地不动 | Root Motion Mode 不对 / 未勾 Enable Root Motion / Slot 不在提取路径 / 不是 Character |
| Motion Warping 没效果 | Warp Target 名字对不上 / 未在 Warping Window 求值前设置 / 动画没有 Root Motion |
| 打开动画编辑器就崩 | Notify 里没判 Owner 为空 |
| 偶发随机崩溃在动画更新 | 在 AnimGraph / 线程安全函数里访问了游戏对象 |
| 连招第二段没反应 | Montage 的 Section 跳转逻辑 / 被同 Group Montage 打断后回调没走 |

调试工具：`ShowDebug Animation` 看运行时状态；Rewind Debugger / Animation Insights 看逐帧的姿势和 Notify 触发；Pose Watch 在 AnimGraph 上看中间姿势。

---

## 优先适配项目已有结构

动手前先看工程里现有的做法，不要自造：

- 用项目已有的 **AnimInstance 基类**和已有的成员变量，不要为一个新状态另开一套
- 用项目已有的 **Slot 命名和 Slot Group 划分**，随意新增 Slot 会破坏已有的打断关系
- 用项目已有的 **Notify 类和事件分发方式**（很多项目统一走 Gameplay Event 或某个自定义委托）
- 用项目已有的**骨骼名常量集合**，不要在代码里硬编码 `TEXT("hand_r")`
- 用项目已有的 **Montage 组织约定**（目录、命名、Section 命名）

找不到现成的就明说："工程里没找到统一的上半身 Slot 命名，我先用了 `UpperBody`，如果已有请告诉我。"

---

## 交付前自检

- [ ] 线程安全路径里不直接调用非线程安全的 Actor、Controller、World 接口，不执行 Trace
- [ ] 所有 `TryGetPawnOwner()`、`MeshComp->GetOwner()` 的访问都判了空（考虑编辑器预览）
- [ ] Montage 的 Slot 名与 AnimGraph 中的 Slot 节点一致，且在输出路径上
- [ ] Montage 的结束处理覆盖了 Completed / BlendOut / Interrupted / Cancelled，统一清理函数避免重复 EndAbility
- [ ] 多人环境下 Montage 有明确的同步方式，拥有者端不会重复播放
- [ ] Notify 作为触发入口时已确认服务器端 Tick Pose、Trigger on Dedicated Server 等配置；权威逻辑不假设 Notify 必然执行
- [ ] Notify / NotifyState 类里没有存播放状态
- [ ] NotifyState 开启的东西（碰撞、无敌）有统一的幂等清理函数，从 NotifyEnd、Montage 回调、Ability 结束、EndPlay 多路径调用
- [ ] Root Motion 的 Mode 已从 AnimInstance/AnimBP 实际读取确认，Slot 路径和 Character 前提已确认
- [ ] Motion Warping 的 Target 名与 Notify State 配置一致，静态目标在 Warping Window 求值前设置、移动目标持续更新、用后清理
- [ ] 骨骼名、Slot 名、Section 名沿用项目已有约定，没有硬编码散落各处
- [ ] 同时满足 ue5-cpp 的基础规则：`UPROPERTY` + `TObjectPtr`、`IsValid()`、`Super::` 调用

不确定引擎版本的具体节点名或 API 签名时，去工程内的引擎源码或已有实现里确认，不要凭记忆写。
