# GASTestDemo1 GAS C++ 文件知识汇总

本文档汇总当前项目中和 GAS（Gameplay Ability System）直接相关，或承担 GAS 初始化、事件发送、属性显示、伤害入口职责的 C++ 文件。

## 1. 模块依赖

### `Source/GASTestDemo1/GASTestDemo1.Build.cs`

项目启用了 GAS 相关模块：

- `GameplayAbilities`：Ability、AbilitySystemComponent、AttributeSet、GameplayEffect。
- `GameplayTasks`：AbilityTask、AI Task 等异步任务能力。
- `GameplayTags`：Native GameplayTag 定义和匹配。
- `UMG`：属性 UI 显示。
- `AIModule`：敌人搜索、移动、攻击链路。
- `Niagara`：伤害数字等表现效果。

这说明项目的战斗逻辑主要围绕 GAS 的 Ability、Attribute、GameplayEvent、GameplayTag 运行。

## 2. GAS 核心类型

### `Source/GASTestDemo1/Public/AbilitySystem/T_AbilitySystemComponent.h`
### `Source/GASTestDemo1/Private/AbilitySystem/T_AbilitySystemComponent.cpp`

`UT_AbilitySystemComponent` 是项目自定义 ASC。

主要作用：

- 继承 `UAbilitySystemComponent`。
- 重写 `OnGiveAbility()`，在 Ability 被授予时检查是否需要自动激活。
- 重写 `OnRep_ActivateAbilities()`，客户端同步 Ability 列表后再次检查自动激活。
- 提供 `SetAbilityLevel()` 和 `AddToAbilityLevel()`，用于修改已授予 Ability 的等级。
- 通过 `TTags::TAbilities::ActivateOnGiven` 判断某个 Ability 是否“授予后立即激活”。

关键点：

- `ActivateOnGiven` 标签是自动启动监听型 Ability 的核心，例如长期等待 GameplayEvent 的 Ability。
- 修改 Ability 等级只允许权威端执行，代码里通过 `GetAvatarActor()->HasAuthority()` 做了保护。

### `Source/GASTestDemo1/Public/AbilitySystem/T_AttributeSet.h`
### `Source/GASTestDemo1/Private/AbilitySystem/T_AttributeSet.cpp`

`UT_AttributeSet` 是项目当前的属性容器。

保存的属性：

- `Health`
- `MaxHealth`
- `Mana`
- `MaxMana`

主要作用：

- 定义 GAS Attribute，并通过 `ATTRIBUTE_ACCESSORS` 生成 Getter、Setter、Attribute Getter。
- 复制属性到客户端，并使用 `GAMEPLAYATTRIBUTE_REPNOTIFY` 通知属性变化。
- 在 `PostGameplayEffectExecute()` 里处理 GameplayEffect 生效后的逻辑。
- 第一次属性被 GE 修改时设置 `bAttributesInitialized = true`，并广播 `OnAttributesInitialized`。
- Mana 变化后限制到 `0 ~ MaxMana`。
- Health 小于等于 0 时，给伤害来源发送 `TTags::Events::KillScored` 事件。

关键点：

- `PostGameplayEffectExecute()` 是属性被 GameplayEffect 修改后的统一入口。
- UI 的属性绑定依赖 `bAttributesInitialized` 和 `OnAttributesInitialized`。
- 当前 `OnRep_AttributesInitialized()` 中使用 `if (!bAttributesInitialized)`，客户端收到复制后的值通常已经是 `true`，这个判断可能导致客户端不广播初始化完成事件。若客户端 UI 等不到属性初始化，优先查这里。

## 3. Ability 基类和具体 Ability

### `Source/GASTestDemo1/Public/AbilitySystem/Abilities/T_GameplayAbility.h`
### `Source/GASTestDemo1/Private/AbilitySystem/Abilities/T_GameplayAbility.cpp`

`UT_GameplayAbility` 是项目所有自定义 Ability 的基础类。

主要作用：

- 继承 `UGameplayAbility`。
- 重写 `ActivateAbility()`。
- 提供 `bDrawDebugs` 调试开关。
- Ability 激活时可在屏幕上显示当前 Ability 名称。

关键点：

- 后续所有项目 Ability 继承它，可以统一保留调试能力。

### `Source/GASTestDemo1/Public/AbilitySystem/Abilities/T_Primary.h`
### `Source/GASTestDemo1/Private/AbilitySystem/Abilities/T_Primary.cpp`

`UT_Primary` 是玩家主要攻击 Ability 的 C++ 基类或辅助类。

主要作用：

- 提供 `SendHitReactEventToActors()`。
- 对命中的 Actor 发送 `TTags::Events::Enemy::HitReact` GameplayEvent。
- Payload 里把 `Instigator` 设置为当前 Ability 的 Avatar Actor。
- 提供 HitBox 相关参数：`HitBoxRadius`、`HitBoxForwardOffset`、`HitBoxElevationOffset`。

关键点：

- 它只发送敌人受击反应事件，不直接处理伤害数值。
- 敌人是否真的播放受击反应，取决于目标身上是否有监听 `TTags::Events::Enemy::HitReact` 的 Ability 或蓝图逻辑。

### `Source/GASTestDemo1/Public/AbilitySystem/Abilities/Enemy/T_SearchForTarget.h`
### `Source/GASTestDemo1/Private/AbilitySystem/Abilities/Enemy/T_SearchForTarget.cpp`

`UT_SearchForTarget` 是敌人的搜索和攻击循环 Ability。

主要作用：

- 构造函数中设置：
  - `InstancingPolicy = InstancedPerActor`
  - `NetExecutionPolicy = ServerOnly`
- 激活时缓存敌人和 AIController。
- 调用 `StartSearch()` 开始循环搜索。
- 使用 `UT_WaitGameplayEvent` 监听 `TTags::Events::Enemy::EndAttack`。
- 搜索最近的玩家目标。
- 移动到目标附近。
- 到达后延迟一段时间，再通过 `TTags::TAbilities::Enemy::Attack` 激活敌人攻击 Ability。

运行链路：

1. Ability 激活。
2. `StartSearch()` 随机等待一段攻击间隔。
3. `Search()` 查找最近的存活玩家。
4. `MoveToTargetAndAttack()` 移动到目标。
5. `AttackTarget()` 等待攻击时间线长度。
6. `Attack()` 激活带 `TTags::TAbilities::Enemy::Attack` 的 Ability。
7. 等收到 `TTags::Events::Enemy::EndAttack` 后重新 `StartSearch()`。

关键点：

- 敌人攻击循环能否继续，核心看 `EndAttack` 事件有没有发回。
- 如果敌人只攻击一次，优先查 `TTags::Events::Enemy::EndAttack` 是否发送、标签是否一致、`WaitGameplayEventTask` 是否启动。
- 如果敌人复活后只移动不攻击，优先查 `bAlive`、死亡标签/效果是否清理、攻击 Ability 是否能被 `TTags::TAbilities::Enemy::Attack` 激活。

### `Source/GASTestDemo1/Public/AbilitySystem/Abilities/Enemy/T_HitReact.h`
### `Source/GASTestDemo1/Private/AbilitySystem/Abilities/Enemy/T_HitReact.cpp`

`UT_HitReact` 是受击反应 Ability 的辅助类。

主要作用：

- 提供 `CacheHitDirectionVectors()`。
- 缓存目标自己的前向向量 `AvatarForward`。
- 计算攻击者到目标的方向 `ToInstigator`。
- 蓝图可读取这两个向量，用于判断前后左右受击方向。

关键点：

- 它不负责监听事件，通常由 Ability 蓝图通过 GameplayEvent 触发。
- 方向判断需要结合 `UT_BlueprintLibrary::GetHitDirection()` 使用。

## 4. AbilityTask / 异步监听

### `Source/GASTestDemo1/Public/AbilitySystem/AbilityTasks/T_WaitGameplayEvent.h`
### `Source/GASTestDemo1/Private/AbilitySystem/AbilityTasks/T_WaitGameplayEvent.cpp`

`UT_WaitGameplayEvent` 是对 `UAbilityAsync_WaitGameplayEvent` 的简单封装。

主要作用：

- `WaitGameplayEventToActorProxy()` 创建异步监听对象。
- 设置目标 Actor、监听标签、是否只触发一次、是否精确匹配。
- `StartActivation()` 手动调用 `Activate()`。

关键点：

- `UT_SearchForTarget` 用它监听敌人自己的 `TTags::Events::Enemy::EndAttack`。
- 它是事件监听器，不会主动发送事件。
- 发送端必须使用同一个 Actor 和同一个 GameplayTag，监听才会回来。

### `Source/GASTestDemo1/Public/Tasks/T_AttributeChangeTask.h`
### `Source/GASTestDemo1/Private/Tasks/T_AttributeChangeTask.cpp`

`UT_AttributeChangeTask` 是蓝图异步任务，用于监听 Attribute 变化。

主要作用：

- `ListenForAttributeChange()` 绑定 ASC 的 `GetGameplayAttributeValueChangeDelegate()`。
- 属性变化时广播 `OnAttributeChange` 给蓝图。
- `EndTask()` 解绑并标记对象可销毁。

关键点：

- 它适合蓝图 UI 或蓝图逻辑监听血量、蓝量等变化。
- 如果不再监听，应调用 `EndTask()`，避免保留无效绑定。

## 5. GameplayTag 定义

### `Source/GASTestDemo1/Public/GameplayTags/TTags.h`
### `Source/GASTestDemo1/Private/GameplayTags/TTags.cpp`

`TTags` 是项目 Native GameplayTag 集中定义处。

主要分类：

- `TTags::SetByCaller`
  - `Projectile`
  - `Melee`
  - `Player::Secondary`
- `TTags::TAbilities`
  - `ActivateOnGiven`
  - `Death`
  - `BlockHitReact`
  - `Primary`
  - `Secondary`
  - `Tertiary`
  - `Enemy::Attack`
- `TTags::Events`
  - `KillScored`
  - `Player::HitReact`
  - `Player::Death`
  - `Player::Primary`
  - `Player::Secondary`
  - `Enemy::HitReact`
  - `Enemy::EndAttack`
  - `Enemy::MeleeTraceHit`
- `TTags::Cooldown`
  - `Secondary`

关键点：

- `TAbilities` 通常用于激活 Ability。
- `Events` 通常用于 `SendGameplayEventToActor()` 和 GameplayEvent 触发。
- `SetByCaller` 通常用于给 GameplayEffect 传入动态数值。
- `TTags::Events::Player::HitReact` 和 `GameplayCue.HitReact` 不是同一类标签，GameplayEvent 发出不等于 GameplayCue 自动执行。

## 6. 角色侧 GAS 初始化

### `Source/GASTestDemo1/Public/Characters/T_BaseCharacter.h`
### `Source/GASTestDemo1/Private/Characters/T_BaseCharacter.cpp`

`AT_BaseCharacter` 是玩家和敌人的共同基类，并实现 `IAbilitySystemInterface`。

主要作用：

- 提供 `GetAbilitySystemComponent()` 虚函数。
- 提供 `GetAttributeSet()` 虚函数。
- 保存初始 Ability 数组 `StartupAbilities`。
- 保存属性初始化 GE：`InitializeAttributesEffect`。
- 保存属性重置 GE：`ResetAttributesEffect`。
- `GiveStartupAbilities()` 给 ASC 授予初始 Ability。
- `InitializeAttributes()` 应用初始化属性 GE。
- `ResetAttributes()` 应用重置属性 GE。
- 监听 Health 变化，Health 小于等于 0 时调用 `HandleDeath()`。
- 维护复制的 `bAlive`。
- 广播 `OnASCInitialized`，供 UI 等组件在 ASC 可用后绑定。

关键点：

- 它本身不创建 ASC 和 AttributeSet，具体由 PlayerState 或 EnemyCharacter 创建。
- `InitializeAttributesEffect` 没配置会触发 `checkf`，直接中断运行。

### `Source/GASTestDemo1/Public/Player/T_PlayerState.h`
### `Source/GASTestDemo1/Private/Player/T_PlayerState.cpp`

`AT_PlayerState` 是玩家 GAS 数据的持有者。

主要作用：

- 创建 `UT_AbilitySystemComponent`。
- 创建 `UT_AttributeSet`。
- 设置 ASC 复制。
- 设置 ASC 复制模式为 `Mixed`。
- 提供 `GetAbilitySystemComponent()` 和 `GetAttributeSet()`。
- 设置 `NetUpdateFrequency = 100.0f`，提高 PlayerState 网络同步频率。

关键点：

- 玩家 ASC 和 AttributeSet 放在 PlayerState 上，而不是 PlayerCharacter 上。
- 这样玩家死亡、重生、Pawn 变化时，能力和属性更容易保留和同步。

### `Source/GASTestDemo1/Public/Characters/T_PlayerCharacter.h`
### `Source/GASTestDemo1/Private/Characters/T_PlayerCharacter.cpp`

`AT_PlayerCharacter` 是玩家 Pawn。

主要 GAS 作用：

- 从 `AT_PlayerState` 获取 ASC 和 AttributeSet。
- `PossessedBy()` 中在服务器初始化 ASC ActorInfo：
  - OwnerActor = PlayerState
  - AvatarActor = PlayerCharacter
- 服务器授予初始 Ability。
- 服务器初始化属性。
- 广播 `OnASCInitialized`。
- 绑定 Health 变化，Health 小于等于 0 时走死亡逻辑。
- `OnRep_PlayerState()` 中在客户端初始化 ASC ActorInfo，并广播 `OnASCInitialized`。

关键点：

- 服务器路径在 `PossessedBy()`。
- 客户端路径在 `OnRep_PlayerState()`。
- 如果客户端 UI 获取不到 ASC，优先查 `OnRep_PlayerState()` 有没有执行、`OnASCInitialized` 有没有广播。

### `Source/GASTestDemo1/Public/Characters/T_EnemyCharacter.h`
### `Source/GASTestDemo1/Private/Characters/T_EnemyCharacter.cpp`

`AT_EnemyCharacter` 是敌人 Pawn，同时直接持有 ASC 和 AttributeSet。

主要 GAS 作用：

- 构造函数创建 `UT_AbilitySystemComponent`。
- 构造函数创建 `UT_AttributeSet`。
- 设置 ASC 复制和 `Mixed` 复制模式。
- `BeginPlay()` 中初始化 ASC ActorInfo：
  - OwnerActor = EnemyCharacter
  - AvatarActor = EnemyCharacter
- 服务器授予初始 Ability。
- 服务器初始化属性。
- 广播 `OnASCInitialized`。
- 绑定 Health 变化，死亡时停止 AI 移动。
- 被击飞时停止移动，落地后发送 `TTags::Events::Enemy::EndAttack`，让搜索/攻击循环继续。

关键点：

- 敌人 ASC 和 AttributeSet 放在敌人自己身上。
- `StopMovementUntilLanded()` 和 `EnableMovementOnLanded()` 会影响 `UT_SearchForTarget` 是否重新开始搜索。
- 如果敌人被击飞后攻击循环不恢复，优先查落地回调和 `EndAttack` 事件。

## 7. 输入与 Ability 激活

### `Source/GASTestDemo1/Public/Player/T_PlayerController.h`
### `Source/GASTestDemo1/Private/Player/T_PlayerController.cpp`

`AT_PlayerController` 把输入映射到 Ability 激活。

主要作用：

- 绑定 Enhanced Input。
- `Primary()` 激活带 `TTags::TAbilities::Primary` 的 Ability。
- `Secondary()` 激活带 `TTags::TAbilities::Secondary` 的 Ability。
- `Tertiary()` 激活带 `TTags::TAbilities::Tertiary` 的 Ability。
- `ActivateAbility()` 通过 `UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetPawn())` 获取 ASC，然后调用 `TryActivateAbilitiesByTag()`。
- 移动、跳跃、Ability 激活前都检查角色是否存活。

关键点：

- 玩家 Ability 能否被按键触发，取决于 Ability 资产是否带对应 Ability Tag。
- 这里使用的是 `TAbilities` 标签，不是 `Events` 标签。

## 8. 伤害、GameplayEvent 和 SetByCaller

### `Source/GASTestDemo1/Public/Utils/T_BlueprintLibrary.h`
### `Source/GASTestDemo1/Private/Utilis/T_BlueprintLibrary.cpp`

`UT_BlueprintLibrary` 是战斗蓝图和 C++ 共用工具库。

与 GAS 相关的主要函数：

- `SendDamageEventToPlayer()`
- `SendDamageEventToPlayers()`
- `HitBoxOverlapTest()`
- `ApplyKnockback()`
- `GetHitDirection()`
- `GetHitDirectionName()`

`SendDamageEventToPlayer()` 的作用：

1. 确认目标是存活角色。
2. 如果没有强制事件标签，就读取目标 `UT_AttributeSet::Health`，判断这次伤害是否致死。
3. 致死时发送 `TTags::Events::Player::Death`。
4. 非致死时发送 `TTags::Events::Player::HitReact`。
5. 给 Payload 附加可选粒子对象。
6. 创建伤害 GameplayEffect Spec。
7. 通过 `AssignTagSetByCallerMagnitude()` 写入动态伤害数值。
8. 把 GameplayEffect 应用到目标自身。

关键点：

- GameplayEvent 负责通知反应，如受击、死亡。
- GameplayEffect 负责真正改属性。
- SetByCaller 负责把本次伤害数值传给 GE。
- 如果事件播放了但血量没变，查 GE 和 SetByCaller。
- 如果血量变了但受击表现没播放，查 GameplayEvent 标签和监听 Ability。

### `Source/GASTestDemo1/Public/Notifies/UT_MeleeAttack.h`
### `Source/GASTestDemo1/Private/Notifies/UT_MeleeAttack.cpp`

`UT_MeleeAttack` 是敌人近战动画通知。

主要作用：

- 在动画通知 Tick 中进行球形 Sweep。
- 只检测 Pawn。
- 找到玩家后构造 `FGameplayEventData`。
- Payload 包含：
  - `Target`
  - `ContextHandle`
  - `Instigator`
- 给攻击者发送 `TTags::Events::Enemy::MeleeTraceHit`。

关键点：

- 它不是直接给玩家扣血，而是通知攻击者“近战命中了”。
- 后续通常由敌人攻击 Ability 蓝图接收 `MeleeTraceHit`，再决定是否调用伤害逻辑。

### `Source/GASTestDemo1/Public/GameObjects/T_Projectile.h`
### `Source/GASTestDemo1/Private/GameObjects/T_Projectile.cpp`

`AT_Projectile` 是投射物。

主要 GAS 作用：

- 重叠到玩家后检查目标是否存活。
- 只在权威端处理伤害。
- 构造 Payload：
  - `Instigator = GetOwner()`
  - `Target = PlayerCharacter`
- 调用 `UT_BlueprintLibrary::SendDamageEventToPlayer()`。
- 使用 `TTags::SetByCaller::Projectile` 作为伤害数值标签。
- 生成命中特效后销毁自身。

关键点：

- 投射物伤害入口最终仍回到 `SendDamageEventToPlayer()`。
- 如果投射物命中但不扣血，优先查 `DamageEffect`、`Damage`、`SetByCaller.Projectile` 和是否在服务器执行。

## 9. UI 与 Attribute 显示

### `Source/GASTestDemo1/Public/UI/T_WidgetComponent.h`
### `Source/GASTestDemo1/Private/UI/T_WidgetComponent.cpp`

`UT_WidgetComponent` 用于把 GAS Attribute 绑定到 UI Widget。

主要作用：

- 从 Owner 角色获取 ASC 和 AttributeSet。
- 如果 ASC / AttributeSet 还不可用，就监听 `AT_BaseCharacter::OnASCInitialized`。
- 如果 AttributeSet 还没初始化，就监听 `UT_AttributeSet::OnAttributesInitialized`。
- 遍历 `AttributeMap`。
- 查找当前 UserWidget 和所有子 Widget 中的 `UT_AttributeWidget`。
- 匹配 Widget 绑定的 Attribute / MaxAttribute。
- 初次主动刷新 UI。
- 后续通过 `GetGameplayAttributeValueChangeDelegate()` 监听属性变化并刷新 UI。

关键点：

- `AttributeMap` 决定哪些属性需要绑定。
- UI 是否刷新，取决于 ASC 初始化、AttributeSet 初始化、Widget 属性匹配三件事。

### `Source/GASTestDemo1/Public/UI/T_AttributeWidget.h`
### `Source/GASTestDemo1/Private/UI/T_AttributeWidget.cpp`

`UT_AttributeWidget` 是显示属性的 Widget 基类。

主要作用：

- 保存当前属性 `Attribute`。
- 保存最大属性 `MaxAttribute`。
- `OnAttributeChange()` 读取当前属性值和最大属性值。
- 通过蓝图事件 `BP_OnAttributeChange()` 把新值传给蓝图 UI。
- `MatchesAttributes()` 判断自己是否匹配某组属性。
- `SpawnDamageNumbers()` 根据属性变化生成伤害数字 Niagara。

关键点：

- C++ 负责读取 Attribute 值和触发蓝图事件。
- 蓝图负责真正显示血条、蓝条、伤害数字等 UI 表现。

## 10. 当前项目 GAS 运行主链路

### 玩家初始化链路

1. `AT_PlayerState` 创建 ASC 和 AttributeSet。
2. `AT_PlayerCharacter::PossessedBy()` 服务器初始化 ASC ActorInfo。
3. 服务器 `GiveStartupAbilities()`。
4. 服务器 `InitializeAttributes()`。
5. 广播 `OnASCInitialized`。
6. 客户端 `OnRep_PlayerState()` 初始化 ASC ActorInfo。
7. UI 通过 `UT_WidgetComponent` 等待 ASC 和 AttributeSet 可用。
8. UI 绑定 Attribute 变化。

### 敌人初始化链路

1. `AT_EnemyCharacter` 构造函数创建 ASC 和 AttributeSet。
2. `BeginPlay()` 初始化 ASC ActorInfo。
3. 服务器授予初始 Ability。
4. 服务器初始化属性。
5. 广播 `OnASCInitialized`。
6. 如果有 `ActivateOnGiven` Ability，会在授予后自动激活。
7. 敌人搜索 Ability 开始等待、找目标、移动、攻击。

### 玩家输入激活 Ability

1. `AT_PlayerController` 接收输入。
2. 根据输入选择 `TTags::TAbilities::Primary / Secondary / Tertiary`。
3. 通过 ASC 调用 `TryActivateAbilitiesByTag()`。
4. 带对应 Ability Tag 的 Ability 被激活。

### 伤害链路

1. 攻击检测命中目标。
2. 构造 `FGameplayEventData`。
3. 发送 GameplayEvent，让目标或攻击者播放受击、死亡、命中等反应。
4. 创建 GameplayEffect Spec。
5. 用 SetByCaller 写入本次伤害。
6. 应用 GameplayEffect 修改 Attribute。
7. `UT_AttributeSet::PostGameplayEffectExecute()` 处理属性后逻辑。
8. Health 变化委托触发死亡逻辑和 UI 刷新。

## 11. 排查时的高价值锚点

### Ability 没激活

优先查：

- Ability 是否已通过 `GiveStartupAbilities()` 授予。
- Ability 资产是否带对应 `TTags::TAbilities.*` 标签。
- 输入是否调用到了 `AT_PlayerController::ActivateAbility()`。
- ASC 是否有效。
- 是否受 Authority 限制。
- 如果是自动激活 Ability，是否带 `TTags::TAbilities::ActivateOnGiven`。

### GameplayEvent 没收到

优先查：

- 发送端 Actor 和监听端 Actor 是否是同一个预期对象。
- 发送标签和监听标签是否完全一致。
- `OnlyMatchExact` 是否要求精确匹配。
- 监听任务是否真的 `StartActivation()`。
- 监听型 Ability 是否已激活。

### 属性不变

优先查：

- GameplayEffect 是否配置正确。
- SetByCaller Tag 是否和 GE 里读取的 Tag 一致。
- `ApplyGameplayEffectSpecToSelf()` 是否执行。
- 目标 ASC 是否有效。
- 是否在服务器执行。

### UI 不刷新

优先查：

- `OnASCInitialized` 是否广播。
- `UT_WidgetComponent::InitAbilitySystemData()` 是否拿到 ASC 和 AttributeSet。
- `UT_AttributeSet::bAttributesInitialized` 是否变成 true。
- `OnAttributesInitialized` 是否广播。
- `AttributeMap` 是否配置了正确的 Attribute / MaxAttribute。
- `UT_AttributeWidget::Attribute` 和 `MaxAttribute` 是否匹配。

### 敌人只攻击一次

优先查：

- `UT_SearchForTarget` 是否激活。
- `TTags::Events::Enemy::EndAttack` 是否发送。
- `UT_WaitGameplayEvent` 是否监听了正确 Actor。
- `EndAttackEventReceived()` 是否执行。
- 敌人是否还处于 `bIsBeingLaunched`。
- 攻击 Ability 是否发送或触发了攻击结束事件。

## 12. 文件职责速查表

| 文件 | 主要职责 |
| --- | --- |
| `GASTestDemo1.Build.cs` | GAS、GameplayTags、GameplayTasks 等模块依赖 |
| `T_AbilitySystemComponent.*` | 自定义 ASC，自动激活 Ability，修改 Ability 等级 |
| `T_AttributeSet.*` | Health/Mana 属性、复制、GE 后处理、击杀事件 |
| `T_GameplayAbility.*` | 项目 Ability 基类，统一调试激活输出 |
| `T_Primary.*` | 玩家主要攻击辅助，给敌人发送 HitReact 事件 |
| `T_SearchForTarget.*` | 敌人搜索、移动、攻击循环 |
| `T_HitReact.*` | 缓存受击方向向量，供蓝图判断方向 |
| `T_WaitGameplayEvent.*` | 封装 GameplayEvent 异步监听 |
| `T_AttributeChangeTask.*` | 蓝图异步监听 Attribute 变化 |
| `TTags.*` | 项目 Native GameplayTag 定义 |
| `T_BaseCharacter.*` | GAS 角色基类，授予 Ability，初始化/重置属性，死亡基础逻辑 |
| `T_PlayerState.*` | 玩家 ASC 和 AttributeSet 持有者 |
| `T_PlayerCharacter.*` | 玩家 Pawn，初始化 ASC ActorInfo，绑定 Health 变化 |
| `T_EnemyCharacter.*` | 敌人 Pawn，持有 ASC/AttributeSet，初始化能力和属性 |
| `T_PlayerController.*` | 输入到 Ability Tag 激活 |
| `T_BlueprintLibrary.*` | 伤害事件、SetByCaller、HitBox、击退、方向判断 |
| `UT_MeleeAttack.*` | 敌人近战动画通知，发送 MeleeTraceHit 事件 |
| `T_Projectile.*` | 投射物命中玩家后走伤害 GameplayEffect 链路 |
| `T_WidgetComponent.*` | 绑定 Attribute 到 UI Widget |
| `T_AttributeWidget.*` | 读取 Attribute 数值并通知蓝图 UI |

