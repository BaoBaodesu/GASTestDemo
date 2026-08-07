# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

# GASTestDemo1 Project Instructions

## Project Overview

Project Name:
GASTestDemo1

Engine:
Unreal Engine 5.7

Language:
C++

Architecture:
Gameplay Ability System (GAS)

Project Type:
Third Person Action TPS


---

# Build & Test

## Build

引擎:
Unreal Engine 5.7

推荐在 IDE（JetBrains Rider / Visual Studio）中通过 `GASTestDemo1.sln` 的 `GASTestDemo1Editor` Target 编译。

命令行编译（需按本机引擎安装路径调整）:

```
"<UE5.7安装路径>\Engine\Build\BatchFiles\Build.bat" GASTestDemo1Editor Win64 Development -Project="D:\Ue5Project\GASTestDemo1\GASTestDemo1.uproject" -WaitMutex
```

编译前检查:
- Include
- Module依赖（见 `Source/GASTestDemo1/GASTestDemo1.Build.cs`）
- Generated.h
- Unreal Header Tool错误

## Tests

测试使用 UE Automation 框架（`IMPLEMENT_SIMPLE_AUTOMATION_TEST`），位于 `Source/GASTestDemo1/Private/Tests/`，用 `#if WITH_DEV_AUTOMATION_TESTS` 包裹。

测试命名前缀:

- `GASTestDemo1.Inventory.*`
- `GASTestDemo1.Grab.*`
- `GASTestDemo1.Traversal.*`
- `GASTestDemo1.Test2.*`

在编辑器命令行运行全部项目测试:

```
UnrealEditor.exe "D:\Ue5Project\GASTestDemo1\GASTestDemo1.uproject" -ExecCmds="Automation RunTests GASTestDemo1" -unattended -nopause -nullrhi -nosplash -log
```

运行单组测试（如 Inventory）:

```
UnrealEditor.exe "D:\Ue5Project\GASTestDemo1\GASTestDemo1.uproject" -ExecCmds="Automation RunTests GASTestDemo1.Inventory" -unattended -nopause -nullrhi -nosplash -log
```

也可在编辑器的 Window → Developer Tools → Automation 面板中运行。


---

# General Rules

## Language

回答使用中文。

代码注释保持简洁。

修改代码前先分析现有架构。

不要未经确认进行大规模重构。


## Modification Rules

优先采用最小修改方案。

保持现有代码结构和命名习惯。

不要为了优化随意新增系统。

不要替换已有架构。


---

# Unreal Engine C++ Rules


## UObject System

必须遵守 Unreal Engine Object System。

禁止:

- new UObject
- 手动管理 UObject 生命周期

必须正确使用:

- UCLASS
- USTRUCT
- UENUM
- UPROPERTY
- UFUNCTION


## Pointer Rules

优先使用:

TObjectPtr

TWeakObjectPtr


注意:

- UObject生命周期
- GC
- nullptr检查


---

# Naming Convention

文件名统一使用 `T_` 前缀（如 `T_BaseCharacter.h`）。

类名使用 UE 标准前缀:

- Actor: `AT_`（如 AT_BaseCharacter、AT_PlayerCharacter、AT_EnemyCharacter）
- Component: `UT_`（如 UT_TraversalComponent、UT_GrabComponent、UT_InventoryComponent）
- Ability: `UT_`（如 UT_PrimaryComboAbility、UT_Traversal，文件名为 `T_*.h`）
- Animation: `UT_`
- AttributeSet / ASC / BlueprintFunctionLibrary: `UT_`


---

# Gameplay Ability System Rules


项目使用 GAS。


修改 Ability 前必须检查:


1. Ability Tags

2. Activation Owned Tags

3. Activation Required Tags

4. Activation Blocked Tags

5. Gameplay Effects

6. Ability System Component 初始化


禁止绕过 GAS 创建独立技能系统。


---

# GAS Architecture


## Character


Base:

AT_BaseCharacter


Player:

AT_PlayerCharacter


Enemy:

AT_EnemyCharacter


---

## Ability System


ASC:

UT_AbilitySystemComponent


Attribute:

AttributeSet


GameplayTag:

TTags


GameplayTag 全部在 `Public/GameplayTags/TTags.h` 中声明（`namespace TTags`），在 `Private/GameplayTags/TTags.cpp` 中定义。结构分：

- State（LockOn / Aiming / Action.* 等状态）
- TAbilities（能力标签）
- Events（事件标签）
- SetByCaller / Cooldown


修改 GameplayTag 前必须确认现有结构。

Native GameplayTag 无需手动注册，`NativeGameplayTags.h` + 配置即可。

---

# Current Systems


## Combat

Primary Combo:

UT_PrimaryComboAbility


Attack状态:

GameplayTag控制


---

## Traversal


Component:

UT_TraversalComponent


相关系统:

- Motion Warping
- Root Motion
- AnimNotify


修改Traversal时检查:

- Ability状态
- CharacterMovement
- Root Motion
- Collision


---

## Animation


主要:

AnimBlueprint

AnimMontage

AnimNotify

Motion Warping


动画问题优先检查:


1. Skeleton

2. Montage Slot

3. Anim Layer

4. Root Motion

5. Notify时间


---

## Inventory

Component:

UT_InventoryComponent


相关:

- UT_ItemDefinition（物品定义）
- UT_InventoryStorage
- UT_InventoryMigrationLibrary
- IT_InventoryItemHandler（玩家角色实现，用于使用/装备物品）

主要 API:

- AddItem / RemoveItem / MoveItem / SplitStack / TransferItem
- AssignQuickSlot / GetQuickSlotItem
- ResizeInventory / DropItem


拾取:

UT_PickUpComponent（玩家侧）+ AT_PickUpItems（可拾取物）


---

# Debug Rules


遇到Bug不要直接修改。


按照流程:


1. 分析Log

2. 定位调用链

3. 检查生命周期

4. 检查nullptr来源

5. 最小修改


禁止:

仅添加大量return隐藏问题。


---

# Build Rules


修改C++后:

检查:

- Include
- Module依赖
- Generated.h
- Unreal Header Tool错误


---

# Code Review Checklist


修改完成后检查:


□ 是否符合UE架构

□ 是否影响GAS

□ 是否存在空指针

□ 是否影响Replication

□ 是否产生Tick性能问题

□ 是否破坏现有命名规则


---

# Important Project Constraints


不要:

- 删除已有系统
- 改变GameplayTag命名
- 替换GAS架构
- 大规模修改文件结构


除非明确要求。


---

# Preferred Workflow


修改流程:

1. 阅读相关.h/.cpp

2. 分析调用关系

3. 提出修改方案

4. 等待确认

5. 修改代码

6. 检查编译问题