# GAS Tag Ownership Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Eliminate invalid loose-tag removals by assigning each action tag to exactly one lifecycle owner.

**Architecture:** The combo animation notify exclusively owns `ComboWindow`; its begin/end callbacks add/remove the loose tag. The roll gameplay ability exclusively owns `Busy` and `Rolling` through `ActivationOwnedTags`, which GAS releases when the ability ends.

**Tech Stack:** Unreal Engine Gameplay Ability System, C++.

## Global Constraints

- Preserve the existing ability and notify structure.
- Do not add tag state variables or duplicate cleanup paths.

---

### Task 1: Correct tag ownership

**Files:**
- Modify: `Source/GASTestDemo1/Private/AbilitySystem/Abilities/T_PrimaryComboAbility.cpp`
- Modify: `Source/GASTestDemo1/Private/AbilitySystem/Abilities/T_RollAbility.cpp`

- [ ] **Step 1: Verify the pre-change ownership violation**

Run: `Select-String` checks confirming `ComboWindow` is removed both by the notify and `EndAbility`, and that `T_RollAbility` removes loose tags without adding them.

- [ ] **Step 2: Apply the minimal ownership correction**

Remove the combo ability's extra `RemoveLooseGameplayTag(ComboWindow)`. Restore roll `ActivationOwnedTags` and remove its manual loose-tag removals.

- [ ] **Step 3: Verify source ownership**

Run source checks confirming one `ComboWindow` remover (the notify), roll activation-owned tags, and no roll loose-tag remover.

- [ ] **Step 4: Build the editor target**

Run the project Unreal Build Tool command and require exit code 0.
