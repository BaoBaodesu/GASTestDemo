---
name: ue-debug
description: 排查 Unreal Engine 项目的运行时错误、崩溃与编译错误时使用本技能。核心要求是先定位根因再动手：看日志 → 找调用链 → 查生命周期 → 判断 nullptr 来源 → 最小修改，禁止用空指针兜底掩盖时序或初始化问题。只要出现 Accessed None、nullptr、空指针、崩溃、Crash、Callstack、堆栈、Assertion failed、check 失败、ensure、UE_LOG、日志、编译报错、link error、LNK、UHT 报错、"点了没反应"、"客户端不生效"、"偶尔崩溃"等字样，或用户贴出 UE 的日志、崩溃堆栈、编译输出，都要使用本技能，即使用户只说"帮我看看这个报错"。本技能与 ue5-cpp、unreal-gas 配合使用：修复代码时同样遵守那两个技能的规则。
---

# UE Debug Rules

## 第一原则：不要立即修改

看到报错就加一行 `if (!Ptr) return;`，是最常见也最有害的处理方式。它让报错消失了，但：

- 根因还在，只是从"报错"变成了"功能静默失效"
- 下次同样的时序问题会在别的地方以别的形式冒出来，且更难关联到原因
- 兜底代码会永久留在工程里，后来的人无法判断这个空判断是"业务上允许为空"还是"当年掩盖了一个 bug"

**空指针检查表达的是"这里业务上允许为空"，不是"我不知道为什么会空"。** 这两种情况写出来的代码长得一样，但只有前者是对的。

所以：先走完下面的排查流程，能说清楚"指针为什么是空的"，再决定改什么。

---

## 排查流程

### 1. 查看 Log

不要凭报错的一句话猜，先拿到完整上下文。

- 编辑器：Output Log 窗口，把过滤器关掉看全量
- 打包版本：`项目目录/Saved/Logs/项目名.log`，崩溃则看 `Saved/Crashes/` 下对应时间戳的目录
- 启动参数加 `-log` 可以开独立日志窗口

看日志时关注：

- **报错前面几十行**，而不是只看报错那一行。真正的线索通常是更早的一条 Warning（比如"XX 加载失败"、"Component 未注册"）
- **报错发生的时机**：在 BeginPlay 之前？关卡切换时？第一次触发还是第 N 次？
- **是哪一端报的**：日志里区分 `[Server]`/`[Client]` 前缀，或自己加 `HasAuthority()` 分端日志。很多问题只在一端出现，这条信息能直接砍掉一半排查范围
- 需要更多信息时用控制台提高级别：`Log LogXXX Verbose`

蓝图的 Accessed None 报错本身就带定位信息——属性名、节点名、图表名、蓝图名都在里面，先把这四项读出来再往下走。

### 2. 找调用链

从报错点往上追，回答"这段代码是被谁、在什么时机调起来的"。

- 崩溃有 Callstack 时，**从上往下找第一个属于项目模块的帧**。引擎帧只是崩溃现场，不是原因所在
- 没有 Callstack 时（比如 Accessed None），手工往上追：这个函数被谁调用？绑定在哪个委托上？是 Tick、输入事件、重叠回调，还是网络回调？
- 特别留意**异步入口**：定时器回调、异步资产加载完成回调、动画通知、网络 RPC。这些路径上的对象状态和主流程完全不同

追调用链的产出应该是一条能写下来的链路，比如：

```
玩家按键 → EnhancedInput 绑定 → ACharacter::Attack()
       → ASC->TryActivateAbility()  ← 这里 ASC 为空
```

### 3. 找生命周期

拿着上一步的调用链，逐个对象问："它这时候创建了吗？还活着吗？在这一端存在吗？"

UE 的初始化顺序是固定的，先确认代码所在阶段能做什么：

```
构造函数 / CDO → PostInitProperties → 属性序列化
→ OnConstruction → PostInitializeComponents → BeginPlay → Tick
→ EndPlay → BeginDestroy → GC 回收
```

几条常查的：

- 构造函数阶段没有 World、没有其他 Actor、没有关卡
- **不同 Actor 的 BeginPlay 顺序不保证**，A 的 BeginPlay 里不能假设 B 已经初始化完
- 网络端：客户端上 `PlayerState`、`Controller`、`Pawn` 的到达顺序不保证，`OnRep_` 可能早于 `BeginPlay`
- 对象可能已经 `Destroy()` 但还没被 GC 回收，此时指针非空但已失效——所以判断用 `IsValid()` 而不是 `!= nullptr`

### 4. 判断 nullptr 来源

把空指针归到下面某一类，不同类别的修法完全不同。**归不了类说明前三步没走完，回去补。**

| 类别 | 典型表现 | 正确修法 |
|---|---|---|
| **时序问题** | 在构造函数或过早的阶段访问；BeginPlay 顺序依赖；OnRep 早于 BeginPlay | 把访问挪到正确阶段；用委托 / 下一帧 / 上层统一驱动来建立依赖 |
| **权限问题** | 只在客户端为空（如 GameMode、AuthorityGameMode）；只在模拟端为空（如其他玩家的 Controller） | 加 `HasAuthority()` / `IsLocallyControlled()` 分支，让该端本来就不执行这段逻辑 |
| **生命周期** | 缓存的指针在对象销毁后失效；跨帧回调里访问已死对象 | 改用 `TWeakObjectPtr`，或在 `EndPlay` 里解绑委托 / 清缓存 |
| **配置缺失** | 蓝图里 `TSubclassOf`、资产引用、组件引用没填 | 改数据而不是改代码；同时在初始化处加一条明确的 `UE_LOG` 报告哪个字段没配 |
| **查找失败** | `FindComponentByClass` 找不到、`Cast` 失败、`TMap::Find` 返回空 | 确认目标是否真的应该存在；是则修复创建逻辑，否则这里才是业务上合法的空判断 |
| **GC 回收** | 指针成员没有 `UPROPERTY`，运行一段时间后突然为空 | 补 `UPROPERTY()` + `TObjectPtr`，见 ue5-cpp |

### 5. 最小修改

确认根因后再动手，并且：

- **改根因，不改症状**。时序问题就调时序，配置问题就补配置，别在使用处加兜底
- 只改与本次问题直接相关的地方，不顺手重构
- 确实需要空判断时，按情况选择是否加日志：
  - **为空属于异常状态**（不该发生但要防御）：输出一条包含对象名和上下文的日志，方便事后排查
    ```cpp
    if (!IsValid(ASC)) { UE_LOG(LogMyGame, Warning, TEXT("%s: ASC 无效，跳过技能授予"), *GetName()); return; }
    ```
  - **为空属于正常业务状态**（设计上允许）：直接返回即可，不要产生多余日志
    ```cpp
    // 可选目标组件，没有就跳过，属于正常情况
    if (!OptionalComp) { return; }
    ```
  - **Tick 或高频回调**：即使是异常状态，也不要每帧输出日志。用 `UE_LOG` 前加一次性标记，或改用 `UE_CLOG` / `ensureMsgf`（只触发一次）
    ```cpp
    if (!IsValid(TargetActor)) { UE_LOG(LogMyGame, Warning, TEXT("%s: TargetActor 无效"), *GetName()); TargetActor = nullptr; return; }  // 置空后下帧不再进这个分支
    ```

- 修完要说清楚：**根因是什么、改了哪里、怎么验证的、还剩什么风险**

---

## 示例：Invalid ability system component retrieved from Actor

**错误做法**——在拿到 ASC 的地方直接加兜底：

```cpp
UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
if (!ASC) { return; }   // 报错没了，技能也永远授予不上了
```

**正确做法**——先把 ASC 这条链路摊开，逐环节确认在哪断的：

```
EnemyCharacter
   |
ASC 创建            → 是否在构造函数里 CreateDefaultSubobject？是否有 UPROPERTY？
   |
InitAbilityActorInfo → 服务器 PossessedBy 调了吗？客户端 OnRep_PlayerState 调了吗？
   |                   Owner / Avatar 传对了吗？重生后重新调了吗？
GiveAbility          → 是否在 HasAuthority() 内？调用时机是否早于 ASC 初始化？
```

对着这条链看日志和调用栈，答案通常落在其中一环：ASC 忘了加 `UPROPERTY` 被 GC 了、客户端漏调 `InitAbilityActorInfo`、或者 `GiveAbility` 跑在了 ASC 初始化之前。这三种情况的修法互不相同，加 `if (!ASC) return;` 一种都解决不了。

GAS 相关的排查细节见 `unreal-gas` 技能。

---

## 编译错误

编译错误不走上面的运行时流程，但同样先读完整输出再动手。**从第一条错误开始看**，后面的多半是它的连锁反应。

常见类别与对应检查：

| 现象 | 常见原因 |
|---|---|
| `xxx.generated.h` 相关报错、`GENERATED_BODY` 报错 | `.generated.h` 不是最后一个 include；类没加 `UCLASS`；缺 `#pragma once` |
| 未定义标识符、不完整类型 | 缺 include；只有前向声明就调用了成员（前向声明只够声明指针，用成员必须包含头文件） |
| 链接错误 `LNK2019` / `unresolved external` | `Build.cs` 里没加模块依赖；跨模块使用的类缺 `MYGAME_API` 宏；函数只声明没实现 |
| UHT 报错（Unreal Header Tool） | `UPROPERTY` 用在非 `UCLASS`/`USTRUCT` 里；`UFUNCTION` 参数用了反射不支持的类型；接口写法不规范 |
| 循环包含 | 两个头文件互相 include，改用前向声明 + 在 .cpp 里 include |
| "以前能编，改完就不行"且错误位置很奇怪 | Unity Build 合并导致的隐式包含变化；试试全量重编 |
| 改了头文件热重载不生效 / 崩溃 | 修改反射声明（`UPROPERTY`/`UFUNCTION`/`UCLASS`）、类布局或默认子对象结构后，不要依赖 Live Coding 验证结果；关闭编辑器并执行完整编译 |
| 引擎 API 找不到 | 版本差异，去引擎源码里搜实际签名，不要凭记忆写 |

改完编译错误后，确认改的是根因而不是把类型强转过去了。

---

## 崩溃

- 崩溃报告在 `Saved/Crashes/`，里面有 Callstack 和当时的日志
- **`Assertion failed: xxx`**：是 `check()` 命中，断言表达式本身就说明了违反了什么前提，先读它
- **`Ensure condition failed`**：`ensure()` 命中，进程没死但状态已经不对，同样要查
- **`EXCEPTION_ACCESS_VIOLATION`** 且地址接近 0（如 `0x00000010`）：典型的空指针 + 成员偏移，走上面的 nullptr 流程
- 崩在引擎内部的 GC、渲染线程等位置时，优先检查项目代码和第三方插件造成的无效状态（`UPROPERTY` 漏了、跨线程访问 UObject、销毁后仍在使用是最常见的三类），但不能仅凭调用栈位置排除引擎缺陷、驱动问题或插件自身的 bug——如果项目侧排查干净了仍然复现，要往这些方向查

---

## 反模式清单

看到这些写法要停下来问"根因查清了吗"：

- 在使用处加 `if (!Ptr) return;` 让报错消失，但说不出为什么会空
- 大量空判断堆在函数开头，用来兜住初始化顺序问题
- 用 `Cast` 失败就静默返回，掩盖了类型设计问题
- 用延迟 / `SetTimer(0.1f)` 来"绕过"时序问题，而不是建立明确的依赖关系
- 把逻辑塞进 `Tick` 里反复检查，代替一次性的正确初始化
- 注释掉报错的代码
- 改引擎源码来绕过项目代码的问题

---

## 优先适配项目已有结构

修复时先看工程里现有的做法，不要自造：

- 用项目已有的**日志分类**（`LogMyGame` 之类）而不是一律 `LogTemp`
- 用项目已有的**校验 / 断言宏和错误处理约定**
- 用项目已有的**基类、组件、接口**来修，而不是新加一层
- 同类问题项目里已经修过的，照那个模式修，保持一致

找不到现成的就明说："工程里没找到统一的日志分类，我先用了 `LogTemp`，如果有请告诉我。"

---

## 输出格式

排查结论按这个结构给：

```
根因：<为什么会出问题，落到具体的时序 / 权限 / 生命周期 / 配置 / GC 某一类>
依据：<日志的哪几行、调用链的哪一环支持这个结论>
修改：<改了哪个文件的哪个位置，为什么这样改>
验证：<实际做了什么验证；没能验证的如实说>
风险：<还可能受影响的地方、未覆盖的场景>
```

**不确定时说不确定。** 如果日志信息不足以定位，直接说明还需要什么——加哪几条日志、复现时注意看什么、是否需要联机复现——而不是给一个看起来合理的猜测当结论。同样，没有实际编译和运行过，就不要说"已修复"，只说"预期能解决，需要你验证"。
