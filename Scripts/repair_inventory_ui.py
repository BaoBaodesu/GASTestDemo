import unreal


def main():
    editor_assets = unreal.get_editor_subsystem(unreal.EditorAssetSubsystem)
    ui_directory = "/Game/GASTestDemo/Inventory/UI"
    for asset_path in editor_assets.list_assets(ui_directory, recursive=False, include_folder=False):
        blueprint = editor_assets.load_asset(asset_path)
        if blueprint and blueprint.get_name().startswith("WBP_"):
            unreal.T_InventoryMigrationLibrary.repair_widget_variables(blueprint)
            editor_assets.save_loaded_asset(blueprint, only_if_is_dirty=False)

    controller = editor_assets.load_asset("/Game/GASTestDemo/Player/BP_T_PlayerController")
    inventory_action_path = "/Game/GASTestDemo/Input/AbilitiesActions/IA_Inventory"
    if not editor_assets.does_asset_exist(inventory_action_path):
        inventory_action = editor_assets.duplicate_asset(
            "/Game/GASTestDemo/Input/AbilitiesActions/IA_Aim",
            inventory_action_path,
        )
        if inventory_action:
            editor_assets.save_loaded_asset(inventory_action, only_if_is_dirty=False)
    else:
        inventory_action = editor_assets.load_asset(inventory_action_path)
    screen = editor_assets.load_asset(f"{ui_directory}/WBP_Inventory_Screen")
    if controller and inventory_action and screen:
        unreal.BlueprintEditorLibrary.compile_blueprint(controller)
        controller_default = unreal.get_default_object(controller.generated_class())
        controller_default.set_editor_property("inventory_action", inventory_action)
        controller_default.set_editor_property("inventory_widget_class", screen.generated_class())
        editor_assets.save_loaded_asset(controller, only_if_is_dirty=False)

        input_mapping = editor_assets.load_asset("/Game/GASTestDemo/Input/IMC_Abilities")
        mappings = []
        for existing_mapping in input_mapping.get_editor_property("mappings"):
            action = existing_mapping.get_editor_property("action")
            key_name = str(existing_mapping.get_editor_property("key").get_editor_property("key_name"))
            if key_name == "I" and (action is None or action.get_path_name() == f"{inventory_action_path}.{inventory_action.get_name()}"):
                continue
            mappings.append(existing_mapping)

        mapping = unreal.EnhancedActionKeyMapping()
        mapping.set_editor_property("action", inventory_action)
        key = unreal.Key()
        key.set_editor_property("key_name", "I")
        mapping.set_editor_property("key", key)
        mappings.append(mapping)
        input_mapping.set_editor_property("mappings", mappings)
        editor_assets.save_loaded_asset(input_mapping, only_if_is_dirty=False)

        screen_default = unreal.get_default_object(screen.generated_class())
        screen_default.set_editor_property(
            "storage_panel_class",
            editor_assets.load_asset(f"{ui_directory}/WBP_Inventory_Panel").generated_class(),
        )
        screen_default.set_editor_property(
            "action_menu_widget_class",
            editor_assets.load_asset(f"{ui_directory}/WBP_Inventory_ActionMenu").generated_class(),
        )
        editor_assets.save_loaded_asset(screen, only_if_is_dirty=False)
    else:
        unreal.log_error("TInventoryRepair: controller, input action, or screen is missing")
        return

    unreal.log("TInventoryRepair: success")


main()
