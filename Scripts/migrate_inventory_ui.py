import unreal


SOURCE_DIRECTORY = "/Game/VisualSandbox/Blueprints/UMG/Inventory"
TARGET_DIRECTORY = "/Game/GASTestDemo/Inventory/UI"

PARENT_CLASSES = {
    "BP_Inventory_DragDropOperation": "/Script/GASTestDemo1.T_InventoryDragDropOperation",
    "WBP_Inventory_Dragging": "/Script/GASTestDemo1.T_InventoryDraggingWidget",
    "WBP_Inventory_ActionMenu": "/Script/GASTestDemo1.T_InventoryActionMenuWidget",
    "WBP_Inventory_Discard_Zone": "/Script/GASTestDemo1.T_InventoryDiscardZoneWidget",
    "WBP_Inventory_Slot": "/Script/GASTestDemo1.T_InventorySlotWidget",
    "WBP_Inventory_Slot_Grid": "/Script/GASTestDemo1.T_InventoryGridWidget",
    "WBP_Inventory_Panel": "/Script/GASTestDemo1.T_InventoryPanelWidget",
    "WBP_Inventory_Screen": "/Script/GASTestDemo1.T_InventoryWidget",
}


def main():
    editor_assets = unreal.get_editor_subsystem(unreal.EditorAssetSubsystem)
    if not editor_assets.does_directory_exist(TARGET_DIRECTORY):
        unreal.log_error(f"TInventory: target directory does not exist: {TARGET_DIRECTORY}")
        return

    source_blueprints = [editor_assets.load_asset(f"{SOURCE_DIRECTORY}/{asset_name}") for asset_name in PARENT_CLASSES]
    target_blueprints = [editor_assets.load_asset(f"{TARGET_DIRECTORY}/{asset_name}") for asset_name in PARENT_CLASSES]

    processed_blueprints = []
    for asset_name, parent_path in PARENT_CLASSES.items():
        asset_path = f"{TARGET_DIRECTORY}/{asset_name}"
        blueprint = editor_assets.load_asset(asset_path)
        source_blueprint = editor_assets.load_asset(f"{SOURCE_DIRECTORY}/{asset_name}")
        parent_class = unreal.load_class(None, parent_path)
        if blueprint is None or source_blueprint is None or parent_class is None:
            unreal.log_error(f"TInventory: cannot load {asset_path} or {parent_path}")
            continue

        unreal.T_InventoryMigrationLibrary.remap_blueprint_references(blueprint, source_blueprints, target_blueprints)
        if not unreal.T_InventoryMigrationLibrary.clean_and_reparent_blueprint(blueprint, parent_class):
            unreal.log_error(f"TInventory: cannot clean/reparent {asset_path}")
            continue

        processed_blueprints.append(blueprint)
        editor_assets.save_asset(asset_path, only_if_is_dirty=False)
        unreal.log(f"TInventory: migrated {asset_path} -> {parent_path}")

    for blueprint in processed_blueprints:
        if not unreal.T_InventoryMigrationLibrary.remap_blueprint_references(blueprint, source_blueprints, target_blueprints):
            unreal.log_error(f"TInventory: cannot remap {blueprint.get_path_name()}")
            continue
        editor_assets.save_loaded_asset(blueprint, only_if_is_dirty=False)

    target_by_name = {blueprint.get_name(): blueprint for blueprint in processed_blueprints}
    grid_blueprint = target_by_name.get("WBP_Inventory_Slot_Grid")
    slot_blueprint = target_by_name.get("WBP_Inventory_Slot")
    dragging_blueprint = target_by_name.get("WBP_Inventory_Dragging")
    screen_blueprint = target_by_name.get("WBP_Inventory_Screen")

    if screen_blueprint:
        unreal.T_InventoryMigrationLibrary.remove_widgets(screen_blueprint, ["Crafting_Panel", "QuickBar"])
        editor_assets.save_loaded_asset(screen_blueprint, only_if_is_dirty=False)

    if grid_blueprint and slot_blueprint:
        unreal.get_default_object(grid_blueprint.generated_class()).set_editor_property("slot_widget_class", slot_blueprint.generated_class())
    if slot_blueprint and dragging_blueprint:
        unreal.get_default_object(slot_blueprint.generated_class()).set_editor_property("dragging_widget_class", dragging_blueprint.generated_class())

    controller_blueprint = editor_assets.load_asset("/Game/GASTestDemo/Player/BP_T_PlayerController")
    if controller_blueprint and screen_blueprint:
        unreal.get_default_object(controller_blueprint.generated_class()).set_editor_property("inventory_widget_class", screen_blueprint.generated_class())
        editor_assets.save_loaded_asset(controller_blueprint, only_if_is_dirty=False)

    inventory_action_path = "/Game/GASTestDemo/Input/AbilitiesActions/IA_Inventory"
    if not editor_assets.does_asset_exist(inventory_action_path):
        editor_assets.duplicate_asset(
            "/Game/GASTestDemo/Input/AbilitiesActions/IA_Aim",
            inventory_action_path,
        )

    inventory_action = editor_assets.load_asset(inventory_action_path)
    input_mapping = editor_assets.load_asset("/Game/GASTestDemo/Input/IMC_Abilities")
    if inventory_action and input_mapping:
        mappings = list(input_mapping.get_editor_property("mappings"))
        if not any(mapping.get_editor_property("action") == inventory_action for mapping in mappings):
            mapping = unreal.EnhancedActionKeyMapping()
            mapping.set_editor_property("action", inventory_action)
            key = unreal.Key()
            key.set_editor_property("key_name", "I")
            mapping.set_editor_property("key", key)
            mappings.append(mapping)
            input_mapping.set_editor_property("mappings", mappings)
            editor_assets.save_loaded_asset(input_mapping, only_if_is_dirty=False)

    if controller_blueprint and inventory_action:
        unreal.get_default_object(controller_blueprint.generated_class()).set_editor_property("inventory_action", inventory_action)
        editor_assets.save_loaded_asset(controller_blueprint, only_if_is_dirty=False)

    unreal.log("TInventory: migration finished")


main()
