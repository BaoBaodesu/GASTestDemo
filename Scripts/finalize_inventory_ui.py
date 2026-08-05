import unreal


SOURCE_DIRECTORY = "/Game/GASTestDemo/Inventory/UI"
FINAL_DIRECTORY = "/Game/GASTestDemo/Inventory/UI_Final"
VISUALSANDBOX_DIRECTORY = "/Game/VisualSandbox/Blueprints/UMG/Inventory"
WINDOW_PANEL_SOURCE = "/Game/VisualSandbox/Blueprints/UMG/Common/WBP_Window_Panel"
WINDOW_PANEL_NAME = "WBP_Window_Panel"

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


def has_forbidden_dependencies(directory):
    editor_assets = unreal.get_editor_subsystem(unreal.EditorAssetSubsystem)
    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    options = unreal.AssetRegistryDependencyOptions(
        include_soft_package_references=True,
        include_hard_package_references=True,
        include_searchable_names=True,
        include_soft_management_references=True,
        include_hard_management_references=True,
    )
    forbidden = []
    for asset_path in editor_assets.list_assets(directory, recursive=True, include_folder=False):
        package_name = asset_path.split(".")[0]
        for dependency in registry.get_dependencies(package_name, options) or []:
            dependency_name = str(dependency)
            if dependency_name.startswith("/Game/VisualSandbox/Blueprints/"):
                forbidden.append(f"{package_name} -> {dependency_name}")
    for dependency in sorted(set(forbidden)):
        unreal.log_error(f"TInventoryFinalize: forbidden dependency: {dependency}")
    return bool(forbidden)


def main():
    editor_assets = unreal.get_editor_subsystem(unreal.EditorAssetSubsystem)
    editor_assets.delete_directory(FINAL_DIRECTORY)

    for asset_name in PARENT_CLASSES:
        if not editor_assets.duplicate_asset(
            f"{SOURCE_DIRECTORY}/{asset_name}",
            f"{FINAL_DIRECTORY}/{asset_name}",
        ):
            unreal.log_error(f"TInventoryFinalize: cannot duplicate {asset_name}")
            return
    if not editor_assets.duplicate_asset(WINDOW_PANEL_SOURCE, f"{FINAL_DIRECTORY}/{WINDOW_PANEL_NAME}"):
        unreal.log_error("TInventoryFinalize: cannot duplicate window panel")
        return

    processed = {}
    for asset_name, parent_path in PARENT_CLASSES.items():
        blueprint = editor_assets.load_asset(f"{FINAL_DIRECTORY}/{asset_name}")
        parent_class = unreal.load_class(None, parent_path)
        if not blueprint or not parent_class:
            unreal.log_error(f"TInventoryFinalize: cannot load {asset_name} or {parent_path}")
            return
        if not unreal.T_InventoryMigrationLibrary.clean_and_reparent_blueprint(blueprint, parent_class):
            unreal.log_error(f"TInventoryFinalize: cannot clean/reparent {asset_name}")
            return
        processed[asset_name] = blueprint
        editor_assets.save_loaded_asset(blueprint, only_if_is_dirty=False)

    window_panel = editor_assets.load_asset(f"{FINAL_DIRECTORY}/{WINDOW_PANEL_NAME}")
    user_widget_class = unreal.load_class(None, "/Script/UMG.UserWidget")
    if not unreal.T_InventoryMigrationLibrary.clean_and_reparent_blueprint(window_panel, user_widget_class):
        unreal.log_error("TInventoryFinalize: cannot clean window panel")
        return
    editor_assets.save_loaded_asset(window_panel, only_if_is_dirty=False)

    original_blueprints = [editor_assets.load_asset(f"{VISUALSANDBOX_DIRECTORY}/{name}") for name in PARENT_CLASSES]
    original_blueprints.append(editor_assets.load_asset(WINDOW_PANEL_SOURCE))
    replacement_blueprints = list(processed.values()) + [window_panel]
    source_classes = [blueprint.generated_class() for blueprint in original_blueprints]
    target_classes = [blueprint.generated_class() for blueprint in replacement_blueprints]
    for asset_name, blueprint in processed.items():
        if not asset_name.startswith("WBP_"):
            continue
        if not unreal.T_InventoryMigrationLibrary.replace_widget_classes(blueprint, source_classes, target_classes):
            unreal.log_error(f"TInventoryFinalize: cannot replace nested widgets in {blueprint.get_name()}")
            return
        editor_assets.save_loaded_asset(blueprint, only_if_is_dirty=False)

    screen = processed["WBP_Inventory_Screen"]
    unreal.T_InventoryMigrationLibrary.remove_widgets(screen, ["Crafting_Panel", "QuickBar"])
    editor_assets.save_loaded_asset(screen, only_if_is_dirty=False)

    grid_default = unreal.get_default_object(processed["WBP_Inventory_Slot_Grid"].generated_class())
    grid_default.set_editor_property("slot_widget_class", processed["WBP_Inventory_Slot"].generated_class())
    slot_default = unreal.get_default_object(processed["WBP_Inventory_Slot"].generated_class())
    slot_default.set_editor_property("dragging_widget_class", processed["WBP_Inventory_Dragging"].generated_class())
    editor_assets.save_loaded_asset(processed["WBP_Inventory_Slot_Grid"], only_if_is_dirty=False)
    editor_assets.save_loaded_asset(processed["WBP_Inventory_Slot"], only_if_is_dirty=False)

    if has_forbidden_dependencies(FINAL_DIRECTORY):
        unreal.log_error("TInventoryFinalize: advanced copy retained forbidden dependencies")
        return

    for asset_name in reversed(list(PARENT_CLASSES)):
        if editor_assets.does_asset_exist(f"{SOURCE_DIRECTORY}/{asset_name}"):
            editor_assets.delete_asset(f"{SOURCE_DIRECTORY}/{asset_name}")
    if editor_assets.does_asset_exist(f"{SOURCE_DIRECTORY}/{WINDOW_PANEL_NAME}"):
        editor_assets.delete_asset(f"{SOURCE_DIRECTORY}/{WINDOW_PANEL_NAME}")

    for asset_name in list(PARENT_CLASSES) + [WINDOW_PANEL_NAME]:
        if not editor_assets.rename_asset(f"{FINAL_DIRECTORY}/{asset_name}", f"{SOURCE_DIRECTORY}/{asset_name}"):
            unreal.log_error(f"TInventoryFinalize: cannot rename {asset_name}")
            return

    controller = editor_assets.load_asset("/Game/GASTestDemo/Player/BP_T_PlayerController")
    final_screen = editor_assets.load_asset(f"{SOURCE_DIRECTORY}/WBP_Inventory_Screen")
    if controller and final_screen:
        controller_default = unreal.get_default_object(controller.generated_class())
        controller_default.set_editor_property("inventory_widget_class", final_screen.generated_class())
        editor_assets.save_loaded_asset(controller, only_if_is_dirty=False)

    unreal.log("TInventoryFinalize: success")


main()
