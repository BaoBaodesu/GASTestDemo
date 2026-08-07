---
name: unreal-gas
description: 编写、修改、迁移、配置或排查 Unreal Engine Gameplay Ability System（GAS）时使用。覆盖 Ability 标签、激活与结束生命周期、Instancing、Cost/Cooldown、GameplayEffect、ASC 初始化、AttributeSet、复制、预测、Montage、GameplayCue 和调试。
---

# Unreal GAS 开发规则

## 工作流程

1. 确认 UE 版本和项目 GAS 基类。
2. 找到 ASC 的 Owner、Avatar、初始化入口和复制模式。
3. 检查 Ability 的标签、Instancing、网络策略、Cost、Cooldown 和结束路径。
4. 检查 GameplayEffect、AttributeSet、GameplayCue、Montage 和 AbilityTask。
5. 分别验证服务器、拥有者客户端和模拟客户端。
6. 采用最小改动，不绕过项目现有 GAS 架构。

API 不确定时读取当前版本引擎源码和项目同类实现，不照搬旧教程。

与 `ue5-cpp`、`ue-debug`、`ue-animation` 配合使用。

---

## 1. 修改 Ability 前确认

- Ability 身份标签
- Activation Owned / Required / Blocked Tags
- Instancing Policy
- Net Execution / Replication Policy
- Cost / Cooldown
- Commit、End、Cancel 路径
- ASC Owner / Avatar / 初始化状态
- GameplayEffect / AttributeSet
- Montage / AbilityTask / GameplayEvent
- 各网络端的预期行为

不要只改 `ActivateAbility` 就假设完整行为已经确定。

---

## 2. Ability 身份标签

UE5.5+ 新代码优先使用 Asset Tags 相关 API，例如 `GetAssetTags()` 和项目已有的 `SetAssetTags()`。不要无条件继续使用已弃用的旧 `AbilityTags` 字段。

改标签前搜索：

- 按标签激活、取消和阻挡
- Gameplay Event
- UI 和输入
- AbilitySet
- Tag Relationship Mapping
- Native Gameplay Tags
- `DefaultGameplayTags.ini`

标签存在父子层级。确认项目使用精确匹配还是层级匹配，不创建语义重复标签。

---

## 3. Activation Tags

### Activation Owned Tags

激活期间表示状态，例如 `State.Attacking`、`State.Aiming`。

检查：

- 是否被其他 Ability 当作 Required、Blocked 或 Cancel 条件
- 是否会意外阻止自己后续输入或重激活
- Ability 是否在所有路径正常结束
- 各网络端能否看到所需标签

不要假设某类标签必然复制或必然不复制。实际可见性受 ASC 配置、Ability 执行策略、标签来源和项目代码影响，调试时分别检查服务器和客户端 ASC。

### Required / Blocked Tags

新增条件时确认：

- 标签由谁添加
- 何时移除
- 是否可能永久残留
- 是否与 Cooldown、状态 GE 或 Owned Tags 冲突
- 父级标签是否造成意外匹配

技能无法激活时先检查标签和 Cooldown，不直接重写输入。

---

## 4. Instancing Policy

### NonInstanced

不能保存每次激活的可变状态，不缓存目标、Task、计数器或委托句柄。

### InstancedPerActor

每个 Actor/ASC 保留一个实例。每次激活前清理临时成员，防止委托重复绑定和旧状态残留。

### InstancedPerExecution

每次激活独立实例，适合并发或独立状态，成本更高。

添加 Ability 成员变量前必须确认 Instancing Policy。

---

## 5. 激活、Commit 与结束

典型流程：

1. `CanActivateAbility`
2. `ActivateAbility`
3. 需要时 `CommitAbility`
4. 启动任务、Montage 或服务器逻辑
5. 完成、打断或取消
6. `EndAbility` / `CancelAbility`

规则：

- Cost / Cooldown 使用项目已有 Commit 流程。
- `CommitAbility` 失败时停止效果并正确结束。
- Commit 前不执行不可撤销副作用。
- 已激活后不能通过提前 `return` 静默离开。
- 每条异步路径最终必须 End 或 Cancel。
- 清理集中到可重复调用的统一函数。
- 不只依赖 Montage 完成、NotifyEnd 或对象析构清理。

只有服务器可以授予和移除 Ability。`GiveAbility`、`ClearAbility` 等放在 Authority 路径。

---

## 6. ASC 的位置、Owner 与 Avatar

ASC 可以属于 Pawn、Character、PlayerState 或其他合适 Actor，位置由状态生命周期决定。

### PlayerState 持有 ASC

适合 Pawn 死亡、更换或重生后仍需保留 Ability、Attribute、Cooldown 和长期状态。

通常：

- Owner：PlayerState
- Avatar：当前 Pawn / Character

### Pawn / Character 持有 ASC

适合 GAS 状态完全属于当前 Pawn，Pawn 销毁后允许重建；AI 常见但不是强制规则。

### InitAbilityActorInfo

Owner 和 Avatar 有效或变化时初始化或重新初始化。

常见但非强制模式：

- PlayerState 持有：服务器 `PossessedBy`，拥有者客户端 `OnRep_PlayerState`
- Pawn 持有：使用项目现有的服务器和客户端初始化入口
- 重生、换 Pawn、重新附身后重新初始化

先确认项目是否已有统一 ASC 初始化函数或委托，防止重复授予 Ability 和重复应用初始 GE。

Ability 的授予和移除只在服务器执行。

---

## 7. AttributeSet

### 创建与注册

固定存在的 AttributeSet 优先在拥有者构造函数使用 `CreateDefaultSubobject`。

也可以在合适时机实例化、注册，或通过 ASC Default Starting Data 配置。动态 AttributeSet 必须遵循项目已有创建、注册、复制和移除流程。

不要规定所有 AttributeSet 都必须：

- 放在 PlayerState
- 在构造函数创建
- 每个新属性单独建类

### 属性复制

复制属性需要：

- `UPROPERTY(ReplicatedUsing=OnRep_Xxx)`
- `GetLifetimeReplicatedProps`
- OnRep 中调用 `GAMEPLAYATTRIBUTE_REPNOTIFY`
- 通常使用 `REPNOTIFY_Always`

示例：

```cpp
void UMyAttributeSet::OnRep_Health(const FGameplayAttributeData& OldHealth)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMyAttributeSet, Health, OldHealth);
}
```

### 属性修改

战斗、Buff、Debuff、Cost、Cooldown 和可预测 Gameplay 优先通过 GameplayEffect 修改。

初始化、存档恢复、调试或服务器完整重置可以使用 ASC 属性 API，但必须明确复制、委托和预测影响。

不要在 Ability 中直接修改 `FGameplayAttributeData` 内部值。Clamp 和结算按项目结构使用 `PreAttributeChange`、`PreGameplayEffectExecute`、`PostGameplayEffectExecute` 或统一计算类。

---

## 8. GameplayEffect

GameplayEffect 是数据资产，运行时通常操作 GameplayEffectSpec。

Duration：

- Instant：立即执行，不进入 Active Gameplay Effects Container
- Has Duration：指定时间内活动
- Infinite：持续到明确移除

是否支持预测和回滚不只由 Duration 决定，还受调用路径、Prediction Key 和计算方式影响。

计算选择：

- 简单修改：Modifier
- 动态幅值：Magnitude Calculation
- 多属性复杂结算：Execution Calculation
- 运行时参数：SetByCaller

复杂伤害通常由服务器裁决，不假设自定义 Execution Calculation 能在客户端完整预测。

修改 GE 前检查：

- Cost / Cooldown
- Duration / Period
- Target / Granted Tags
- Application Requirements
- Modifier Operation
- Snapshot / Non-Snapshot
- Stacking、刷新、溢出
- Immunity、Removal、Additional Effects
- GameplayCue
- SetByCaller Tag 和赋值路径

缺少 SetByCaller 值时不要静默使用不明确默认值。

---

## 9. Cost 与 Cooldown

优先复用项目已有 GE 和 Ability 基类。

检查：

- Cost 属性是否足够
- Cost 是否重复应用
- Cooldown Tag 是否正确授予和移除
- 是否与其他 Ability 共享 Cooldown
- SetByCaller / 等级缩放是否正确
- 预测失败后能否校正
- 取消是否需要退还 Cost

不要在输入代码手动扣属性，同时又让 `CommitAbility` 应用 Cost GE。

---

## 10. ASC Replication Mode

### Full

复制完整 Active Gameplay Effects，信息完整但网络成本较高。

### Mixed

通常只向拥有者复制完整 GE，其他客户端主要接收 Tags 和 GameplayCues。常用于玩家。

Mixed 要求 Owner Actor 的 ownership 链正确解析到拥有连接。ASC 不强制必须放在 PlayerState。

### Minimal

不向客户端复制完整 GE，主要同步 Tags 和 GameplayCues，常用于大量 AI 或非玩家单位。

不要只按玩家/AI机械选择。确认 UI、旁观者、效果查询和调试是否依赖完整 GE 数据。

---

## 11. Prediction

Net Execution Policy：

| 策略 | 用途 |
|---|---|
| `LocalPredicted` | 拥有者客户端先执行，服务器裁决 |
| `LocalOnly` | 纯本地，不改变共享 Gameplay 状态 |
| `ServerOnly` | 服务器执行 |
| `ServerInitiated` | 服务器发起并同步 |

规则：

- 预测结果必须允许服务器拒绝和校正。
- 只使用明确支持预测的 GAS API 和 AbilityTask。
- Prediction Key 必须来自有效预测窗口。
- 客户端目标数据必须由服务器验证。
- 命中可以预测表现，但最终目标和伤害由服务器决定。
- 不把客户端 Trace 直接当权威结果。
- 不在预测路径执行不可回滚副作用：持久 Actor、背包扣除、存档、任务结算、外部请求。
- GE 移除、周期、到期和自定义执行是否可预测，按当前引擎实现验证，不写绝对规则。

---

## 12. AbilityTask、Montage 与 GameplayEvent

优先使用项目已有 AbilityTask，不用 Tick 轮询替代异步任务。

`PlayMontageAndWait` 至少处理：

- Completed
- BlendOut
- Interrupted
- Cancelled

统一清理并防止重复 `EndAbility`。Montage 播放失败时立即结束，不让 Ability 永久激活。

GameplayEvent 检查：

- Event Tag 是否匹配
- Payload 的 Instigator、Target、OptionalObject、Magnitude
- 由哪一端发送
- 服务器是否会执行产生事件的 Notify
- Ability 是否可能提前取消

`WaitGameplayEvent` 如果由 Anim Notify 发送，仍依赖 Notify 实际执行。

---

## 13. GameplayCue

与 GAS 状态或 GE 关联的粒子、音效和短期表现优先使用 GameplayCue。

检查：

- Cue Tag 层级
- Executed / OnActive / WhileActive / Removed 语义
- Dedicated Server 是否跳过无意义表现
- 本地预测是否造成重复播放
- Mixed / Minimal 模式下旁观者能否收到表现

GameplayCue 是推荐通道，但不是绝对禁止自定义 RPC。仅在 Cue 语义不适合且项目已有明确设计时使用其他方案。

---

## 14. 项目优先

修改前搜索：

- Ability 基类
- 自定义 ASC
- AttributeSet
- AbilitySet / Startup Ability
- Native Tags
- GE 模板
- GameplayCue
- AbilityTask
- Execution Calculation
- Tag Relationship Mapping
- 输入绑定
- ASC 初始化委托
- 联机测试

不要绕过 ASC、创建重复标签、复制现有 Cost/Cooldown GE、无依据移动 ASC 所属 Actor，或同时在 GE 和 C++ 维护两套数值。

---

## 15. 调试顺序

### Ability 无法激活

1. ASC / ActorInfo
2. Ability 是否授予
3. Spec Handle
4. Required / Blocked / Owned Tags
5. Cooldown
6. Cost
7. NetExecutionPolicy 和当前端
8. Ability 是否仍激活
9. Instancing 状态

### Attribute 不同步

1. GE 是否在服务器应用
2. AttributeSet 是否注册到正确 ASC
3. ReplicatedUsing
4. GetLifetimeReplicatedProps
5. GAMEPLAYATTRIBUTE_REPNOTIFY
6. Owner ownership 链
7. Replication Mode
8. UI 是否引用旧 ASC / Avatar

### 技能只能触发一次

1. EndAbility
2. AbilityTask 是否结束
3. Interrupted / Cancelled 是否漏处理
4. Owned Tag 是否残留
5. Cooldown GE 是否残留
6. InstancedPerActor 成员是否重置
7. 激活中输入是否正确转发

工具：

- `showdebug abilitysystem`
- 项目 GAS 日志
- 分端记录 Authority、LocallyControlled、Owner、Avatar、Prediction Key
- 查看 Active GE、Owned Tags 和 Ability Specs

没有服务器与客户端证据时，不把网络推测写成事实。

---

## 16. 交付前检查

- [ ] 使用当前 UE 版本的 Ability / Asset Tags API
- [ ] 标签关系未被破坏
- [ ] Instancing 与成员状态匹配
- [ ] NetExecutionPolicy 正确
- [ ] Cost / Cooldown 没有重复应用
- [ ] Commit 失败会结束
- [ ] 四种 Montage 结束路径都有收尾
- [ ] 每条 Ability 路径会 End 或 Cancel
- [ ] ASC Owner / Avatar 与生命周期一致
- [ ] 服务器和拥有者客户端正确初始化 ActorInfo
- [ ] Give / Clear Ability 只在服务器
- [ ] AttributeSet 创建、注册、复制完整
- [ ] Attribute OnRep 使用 GAS 宏
- [ ] GE Duration、Stacking、Tags、SetByCaller 正确
- [ ] 预测路径没有不可回滚副作用
- [ ] 客户端目标数据由服务器验证
- [ ] 表现优先复用 GameplayCue
- [ ] 未联机验证时，不声称网络问题已修复

输出说明：根因或依据、GAS 调用链、修改文件和资产、标签/GE/ASC/网络影响、已执行验证、未验证项与风险。
