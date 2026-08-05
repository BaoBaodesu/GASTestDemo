import unreal


def get_key_name(mapping):
    return str(mapping.get_editor_property("key").get_editor_property("key_name"))


def main():
    editor_assets = unreal.get_editor_subsystem(unreal.EditorAssetSubsystem)
    controller = editor_assets.load_asset("/Game/GASTestDemo/Player/BP_T_PlayerController")
    input_mapping = editor_assets.load_asset("/Game/GASTestDemo/Input/IMC_Abilities")
    interactive_action = editor_assets.load_asset("/Game/GASTestDemo/Input/AbilitiesActions/IA_Interactive")
    backpack_action = editor_assets.load_asset("/Game/GASTestDemo/Input/AbilitiesActions/IA_Backpack")
    old_inventory_action = editor_assets.load_asset("/Game/GASTestDemo/Input/AbilitiesActions/IA_Inventory")
    if not controller or not input_mapping or not interactive_action or not backpack_action:
        unreal.log_error("TInventoryInput: required controller or input asset is missing")
        return

    configured_paths = {interactive_action.get_path_name(), backpack_action.get_path_name()}
    if old_inventory_action:
        configured_paths.add(old_inventory_action.get_path_name())
    mappings = []
    for existing_mapping in input_mapping.get_editor_property("mappings"):
        action = existing_mapping.get_editor_property("action")
        action_path = action.get_path_name() if action else ""
        key_name = get_key_name(existing_mapping)
        if action_path in configured_paths:
            continue
        if key_name in {"B", "F"} and action is None:
            continue
        mappings.append(existing_mapping)

    for action, key_name in [(interactive_action, "F"), (backpack_action, "B")]:
        mapping = unreal.EnhancedActionKeyMapping()
        mapping.set_editor_property("action", action)
        key = unreal.Key()
        key.set_editor_property("key_name", key_name)
        mapping.set_editor_property("key", key)
        mappings.append(mapping)

    input_mapping.set_editor_property("mappings", mappings)
    editor_assets.save_loaded_asset(input_mapping, only_if_is_dirty=False)

    unreal.BlueprintEditorLibrary.compile_blueprint(controller)
    controller_default = unreal.get_default_object(controller.generated_class())
    controller_default.set_editor_property("pick_up_action", interactive_action)
    controller_default.set_editor_property("inventory_action", backpack_action)
    editor_assets.save_loaded_asset(controller, only_if_is_dirty=False)
    unreal.log("TInventoryInput: IA_Interactive=F, IA_Backpack=B")


main()
