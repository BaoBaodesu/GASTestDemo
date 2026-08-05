import unreal


ROOT = "/Game/GASTestDemo/UI/Inventory/UI"
NATIVE_PARENTS = {
    "WBP_Inventory_Screen1": "/Script/GASTestDemo1.T_InventoryWidget",
    "WBP_Inventory_Panel1": "/Script/GASTestDemo1.T_InventoryPanelWidget",
    "WBP_Inventory_Slot_Grid1": "/Script/GASTestDemo1.T_InventoryGridWidget",
    "WBP_Inventory_Slot1": "/Script/GASTestDemo1.T_InventorySlotWidget",
    "WBP_Inventory_ActionMenu1": "/Script/GASTestDemo1.T_InventoryActionMenuWidget",
    "WBP_Inventory_Discard_Zone1": "/Script/GASTestDemo1.T_InventoryDiscardZoneWidget",
    "WBP_Inventory_Dragging1": "/Script/GASTestDemo1.T_InventoryDraggingWidget",
    "WBP_QuickSlots_Panel1": "/Script/GASTestDemo1.T_WeaponBarWidget",
    "WBP_QuickBar_Slot1": "/Script/GASTestDemo1.T_WeaponSlotWidget",
}


def main():
    assets = unreal.get_editor_subsystem(unreal.EditorAssetSubsystem)
    targets = {}

    for name, parent_path in NATIVE_PARENTS.items():
        blueprint = assets.load_asset(f"{ROOT}/{name}")
        parent_class = unreal.load_class(None, parent_path)
        if not blueprint or not parent_class:
            unreal.log_error(f"TInventoryUI1: missing {name} or {parent_path}")
            return
        if not unreal.T_InventoryMigrationLibrary.clean_and_reparent_blueprint(blueprint, parent_class):
            unreal.log_error(f"TInventoryUI1: cannot clean and reparent {name}")
            return
        targets[name] = blueprint

    source_paths = [
        "/Game/VisualSandbox/Blueprints/UMG/Inventory/WBP_Inventory_Panel",
        "/Game/VisualSandbox/Blueprints/UMG/Inventory/WBP_Inventory_Slot_Grid",
        "/Game/VisualSandbox/Blueprints/UMG/Inventory/WBP_Inventory_Slot",
        "/Game/VisualSandbox/Blueprints/UMG/Inventory/WBP_Inventory_ActionMenu",
        "/Game/VisualSandbox/Blueprints/UMG/Inventory/WBP_Inventory_Discard_Zone",
        "/Game/VisualSandbox/Blueprints/UMG/Inventory/WBP_Inventory_Dragging",
        "/Game/VisualSandbox/Blueprints/UMG/Common/WBP_Window_Panel",
        "/Game/VisualSandbox/Blueprints/GameModes/ThirdPerson/UMG/QuickSlots/WBP_QuickSlots_Panel",
        "/Game/GASTestDemo/UI/Inventory/WBP_WeaponBar",
        "/Game/VisualSandbox/Blueprints/GameModes/ThirdPerson/UMG/QuickSlots/WBP_QuickBar_Slot",
    ]
    target_paths = [
        f"{ROOT}/WBP_Inventory_Panel1",
        f"{ROOT}/WBP_Inventory_Slot_Grid1",
        f"{ROOT}/WBP_Inventory_Slot1",
        f"{ROOT}/WBP_Inventory_ActionMenu1",
        f"{ROOT}/WBP_Inventory_Discard_Zone1",
        f"{ROOT}/WBP_Inventory_Dragging1",
        f"{ROOT}/WBP_Window_Panel",
        f"{ROOT}/WBP_QuickSlots_Panel1",
        f"{ROOT}/WBP_QuickSlots_Panel1",
        f"{ROOT}/WBP_QuickBar_Slot1",
    ]
    source_blueprints = [assets.load_asset(path) for path in source_paths]
    target_blueprints = [assets.load_asset(path) for path in target_paths]
    source_classes = [blueprint.generated_class() for blueprint in source_blueprints]
    target_classes = [blueprint.generated_class() for blueprint in target_blueprints]

    unreal.T_InventoryMigrationLibrary.remove_widgets(targets["WBP_Inventory_Screen1"], ["Crafting_Panel"])
    for blueprint in targets.values():
        if not unreal.T_InventoryMigrationLibrary.remap_blueprint_references(
            blueprint, source_blueprints, target_blueprints
        ):
            unreal.log_error(f"TInventoryUI1: cannot remap references in {blueprint.get_name()}")
            return
        if not unreal.T_InventoryMigrationLibrary.replace_widget_classes(
            blueprint, source_classes, target_classes
        ):
            unreal.log_error(f"TInventoryUI1: cannot replace nested widgets in {blueprint.get_name()}")
            return

    screen_default = unreal.get_default_object(targets["WBP_Inventory_Screen1"].generated_class())
    screen_default.set_editor_property(
        "storage_panel_class", targets["WBP_Inventory_Panel1"].generated_class()
    )
    screen_default.set_editor_property(
        "action_menu_widget_class", targets["WBP_Inventory_ActionMenu1"].generated_class()
    )

    grid_default = unreal.get_default_object(targets["WBP_Inventory_Slot_Grid1"].generated_class())
    grid_default.set_editor_property("slot_widget_class", targets["WBP_Inventory_Slot1"].generated_class())

    slot_default = unreal.get_default_object(targets["WBP_Inventory_Slot1"].generated_class())
    slot_default.set_editor_property(
        "dragging_widget_class", targets["WBP_Inventory_Dragging1"].generated_class()
    )

    weapon_bar = targets["WBP_QuickSlots_Panel1"]
    weapon_slot = targets["WBP_QuickBar_Slot1"]
    weapon_bar_default = unreal.get_default_object(weapon_bar.generated_class())
    weapon_bar_default.set_editor_property("quick_slot_widget_class", weapon_slot.generated_class())

    controller = assets.load_asset("/Game/GASTestDemo/Player/BP_T_PlayerController")
    controller_default = unreal.get_default_object(controller.generated_class())
    controller_default.set_editor_property("inventory_widget_class", targets["WBP_Inventory_Screen1"].generated_class())

    tick_blueprints = list(targets.values())
    for blueprint in tick_blueprints:
        if not unreal.T_InventoryMigrationLibrary.set_widget_tick_enabled(blueprint, True):
            unreal.log_error(f"TInventoryUI1: cannot enable tick for {blueprint.get_name()}")
            return
        unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
        assets.save_loaded_asset(blueprint, only_if_is_dirty=False)

    unreal.BlueprintEditorLibrary.compile_blueprint(controller)
    assets.save_loaded_asset(controller, only_if_is_dirty=False)
    unreal.log("TInventoryUI1: migration success")


main()
