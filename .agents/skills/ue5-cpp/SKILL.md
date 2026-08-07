---
name: ue5-cpp
description: 编写、修改、迁移或审查 Unreal Engine 5 C++ 代码时使用。覆盖反射、UObject 指针与 GC、对象创建销毁、Actor/Component 生命周期、网络复制、RPC、模块依赖和 UE 类型选择，并优先保持项目现有架构、命名和代码风格。
---

# UE5 C++ 开发规则

## 工作流程

1. 确认 UE 版本、目标模块、相关基类和项目规范。
2. 阅读相关 `.h`、`.cpp`、`Build.cs`、接口和同类实现。
3. 明确对象所有权、生命周期、运行端和网络权限。
4. 采用最小改动，不无关重构，不自造重复系统。
5. 修改后检查 UHT、编译、运行时生命周期和网络影响。

API 或宏不确定时，查看当前版本引擎源码和项目现有实现，不凭记忆编造。

---

## 1. 反射与头文件

- 只有需要反射、蓝图、序列化、编辑器暴露或网络复制时，才添加 `UCLASS`、`USTRUCT`、`UENUM`、`UPROPERTY`、`UFUNCTION`。
- `UCLASS`、`USTRUCT`、`UINTERFACE` 类体使用 `GENERATED_BODY()`。
- 反射头文件中的 `*.generated.h` 必须是最后一个 `#include`。
- 需要蓝图访问的枚举使用 `UENUM(BlueprintType)`；仅 C++ 使用时按项目规范决定。
- 头文件优先前向声明；调用成员、继承或按值使用时包含完整头文件。
- `.cpp` 先包含自己的头文件，暴露隐式依赖。
- 跨模块公开类型检查正确的 `PROJECT_API`。
- 新使用模块同步更新 `Build.cs`。

---

## 2. UObject 指针与 GC

先判断引用语义，不把所有 UObject 指针机械改成同一种类型。

### 持久强引用

需要 GC 跟踪、序列化、复制或编辑器识别的 `UCLASS` / `USTRUCT` 成员：

```cpp
UPROPERTY()
TObjectPtr<UMyObject> OwnedObject;
```

容器使用：

```cpp
UPROPERTY()
TArray<TObjectPtr<UMyObject>> Objects;
```

`TObjectPtr` 只有作为受支持的 `UPROPERTY` 引用时才构成 GC 可见的强引用。

### 弱引用

不希望延长目标生命周期，且需要跨帧缓存时：

```cpp
TWeakObjectPtr<AActor> LastTarget;
```

使用前调用 `.IsValid()` 或 `.Get()`。

### 软引用

资产或类不应立即加载，或需要避免硬依赖时：

```cpp
UPROPERTY(EditDefaultsOnly)
TSoftObjectPtr<UAnimMontage> MontageAsset;
```

类引用使用 `TSoftClassPtr<T>`。

### 裸指针

可以用于：

- 函数参数和返回值
- 局部变量
- 当前调用内的短期引用
- 项目中有明确生命周期保证和失效清理的非拥有成员

不要用未追踪的裸成员指针保持动态 UObject 存活。

### 非 UObject 类持有 UObject

根据项目结构选择 `FGCObject`、`TStrongObjectPtr`、弱引用，或把引用移到合适的 UObject 所有者。不要默认使用 `TStrongObjectPtr`。

### Outer

`Outer` 表达归属、命名和对象层级，不是持久强引用的替代品。运行时创建的 UObject 需要长期存活时，仍应建立明确的 GC 可见引用。

---

## 3. 创建与销毁

| 场景 | 方式 |
|---|---|
| 构造函数创建默认子对象 | `CreateDefaultSubobject<T>()` |
| 运行时创建 UObject | `NewObject<T>(Outer)` |
| 运行时生成 Actor | `SpawnActor<T>()` |
| 创建 Widget | `CreateWidget<T>()` |
| 延迟加载资产 | 软引用 + 同步/异步加载 |

禁止用 `new` / `delete` 管理 UObject。普通非 UObject 类型可使用值语义、`TUniquePtr`、`TSharedPtr` 或项目既有方式。

销毁：

- Actor：`Destroy()`
- ActorComponent：`DestroyComponent()`
- 普通 UObject：断开强引用，由 GC 回收
- 延时销毁 Actor：`SetLifeSpan()`

不要把 `MarkAsGarbage()`、`ConditionalBeginDestroy()` 或手动 GC 当作普通 Gameplay 销毁流程。

---

## 4. nullptr 与 IsValid

- 只判断是否赋值：`if (!Ptr)`
- 目标可能已 Destroy、标记回收或跨帧失效：`IsValid(Ptr)`
- `TWeakObjectPtr`：`.IsValid()` / `.Get()`

空值属于正常业务状态时可以直接返回，不输出重复 Warning。

空值属于异常状态时，先定位初始化、配置、权限或生命周期根因，再添加包含上下文的日志。不要用大量空判断掩盖问题。

---

## 5. 生命周期

### 构造函数

只做默认值、默认子对象和组件层级设置。不要依赖有效游戏 World、查找关卡 Actor、执行 Gameplay、启动定时器或注册依赖运行时对象的委托。

构造函数会为 CDO 和编辑器对象执行，不能假设当前对象是游戏实例。

### OnConstruction

可能在编辑器中反复执行。逻辑必须可重复，不能累积创建对象或产生不可逆副作用。

### PostInitializeComponents / InitializeComponent

适合建立本 Actor 内部组件关系，但先确认项目使用的初始化路径。

### BeginPlay

适合启动运行时逻辑。不同 Actor 的 `BeginPlay` 顺序没有通用保证。

跨 Actor 依赖优先使用明确初始化入口、委托、Subsystem 或上层协调。不要把固定延迟和 Tick 轮询当作默认修复。

### EndPlay

清理委托、定时器、异步回调、临时状态和缓存。Gameplay 清理不要依赖析构函数或 `BeginDestroy`。

### Super

覆盖生命周期函数时，除非父类文档或项目实现明确要求不调用，否则保留正确位置的 `Super::`。不要机械规定所有函数都必须放开头或结尾。

---

## 6. Actor 与 Component

- 固定组件在构造函数用 `CreateDefaultSubobject`。
- 运行时组件用 `NewObject` 后按项目要求注册、附加和管理。
- 组件复制需要 Owner Actor 可复制，并正确设置组件复制。
- 不在 Tick 中重复查找可缓存组件。
- `FindComponentByClass` 为空时先确认组件是否按架构必然存在。
- 创建组件后检查 Root、Attachment、Collision、Mobility、Activation 和注册状态。

---

## 7. 网络权限与复制

先确认：

1. Actor ownership
2. Authority 在哪一端
3. 输入由哪一端发起
4. 哪些是持久状态，哪些是瞬时表现

规则：

- 共享 Gameplay 状态由服务器裁决。
- 本地输入、UI、摄像机和即时反馈在拥有者客户端处理。
- 复制属性同步状态；RPC 传递有方向的事件。
- 客户端提交的命中、伤害、位置和资源数据必须由服务器校验。

Actor：

```cpp
if (HasAuthority())
{
	// Server
}
```

Component：

```cpp
if (GetOwner() && GetOwner()->HasAuthority())
{
	// Server
}
```

复制属性需要：

- `UPROPERTY(Replicated/ReplicatedUsing)`
- `GetLifetimeReplicatedProps`
- Actor 启用复制
- Component 需要时启用组件复制

RPC 检查：

- 对象是否复制
- Ownership 是否允许该方向
- Server / Client / NetMulticast 是否正确
- 参数是否支持复制
- 是否真的需要 Reliable

Reliable 只保证传输，不代表 Gameplay 结果可信。高频事件不要使用 Reliable RPC 堵塞通道。

---

## 8. UE 类型与标准库

与 UObject、反射、蓝图、序列化、复制或 UE API 交互的数据优先使用 UE 类型：

- `TArray`、`TMap`、`TSet`
- `FString`、`FName`、`FText`
- `TOptional`、`TFunction`
- `FVector`、`FRotator`、`FTransform`

纯 C++ 算法内部、第三方库边界或项目已有标准库代码可以使用 `std::`，但不要无意义混用，也不要把标准智能指针用于 UObject。

文本选择：

- `FName`：标识符和查找键
- `FString`：可变字符串和调试文本
- `FText`：面向玩家并需要本地化的文本

---

## 9. 日志、断言与性能

- 使用项目已有日志分类。
- 正常可选状态不输出 Warning。
- 高频路径避免每帧重复日志。
- `check` 用于无法继续的程序员错误。
- `ensure` 用于记录异常但允许继续的条件。
- 可恢复错误使用明确日志和业务处理。

性能：

- 不默认开启 Tick。
- 不在 Tick 中进行全局 Actor 搜索、资产加载或重复 Cast。
- 异步、定时器和委托回调中的 UObject 使用前重新确认有效性。
- 需要每帧执行时说明原因并缓存稳定引用。

---

## 10. 编译与 Live Coding

修改后检查：

1. 第一条 UHT / 编译错误
2. `.generated.h` 和反射类型
3. 前向声明与 include
4. `Build.cs` 模块依赖
5. 跨模块导出宏
6. 声明、定义和签名一致
7. 新文件所在模块和目录

新增或删除反射成员、修改类布局、继承、默认子对象或 UCLASS/USTRUCT 后，不要只依赖 Live Coding；关闭编辑器并完整编译。

---

## 11. 项目优先

修改前搜索已有：

- 基类
- Component
- Interface
- Subsystem
- 日志分类
- 数据结构
- 工具函数
- 网络实现
- 命名和目录约定

不要新建语义重复系统，不无要求引入框架或依赖，不为了形式上的现代化重写可用代码。

发现现有风险但修复超出任务范围时，明确指出，不悄悄扩大修改。

---

## 12. 交付前检查

- [ ] 修改范围没有无关重构
- [ ] UObject 指针符合所有权语义
- [ ] 没有用 `new` / `delete` 管理 UObject
- [ ] 动态 UObject 有明确 GC 可见引用
- [ ] 没有把 Outer 当作唯一生命周期保证
- [ ] 构造和 OnConstruction 没有运行时副作用
- [ ] 委托、定时器和异步回调有清理
- [ ] 空指针检查没有掩盖根因
- [ ] 网络逻辑明确 Authority、Ownership 和执行端
- [ ] 复制、RPC 和 Component 配置完整
- [ ] UHT、include、导出宏和 Build.cs 正确
- [ ] 未实际编译或运行时，不声称验证通过

输出说明：根因或依据、修改位置、关键改动、已执行验证、未验证项和剩余风险。
