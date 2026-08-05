import unreal


TARGET_DIRECTORY = "/Game/GASTestDemo/Inventory/UI"


def main():
    editor_assets = unreal.get_editor_subsystem(unreal.EditorAssetSubsystem)
    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    options = unreal.AssetRegistryDependencyOptions(
        include_soft_package_references=True,
        include_hard_package_references=True,
        include_searchable_names=True,
        include_soft_management_references=True,
        include_hard_management_references=True,
    )

    logical_visual_sandbox_dependencies = []
    for asset_path in editor_assets.list_assets(TARGET_DIRECTORY, recursive=True, include_folder=False):
        package_name = asset_path.split(".")[0]
        dependencies = registry.get_dependencies(package_name, options) or []
        for dependency in dependencies:
            dependency_name = str(dependency)
            if dependency_name.startswith("/Game/VisualSandbox/Blueprints/"):
                logical_visual_sandbox_dependencies.append(f"{package_name} -> {dependency_name}")

    if logical_visual_sandbox_dependencies:
        for dependency in logical_visual_sandbox_dependencies:
            unreal.log_error(f"TInventoryVerify: forbidden dependency: {dependency}")
    else:
        unreal.log("TInventoryVerify: no VisualSandbox Blueprint dependency")

    controller_blueprint = editor_assets.load_asset("/Game/GASTestDemo/Player/BP_T_PlayerController")
    controller_default = unreal.get_default_object(controller_blueprint.generated_class())
    unreal.log(f"TInventoryVerify: controller inventory action={controller_default.get_editor_property('inventory_action')}")
    unreal.log(f"TInventoryVerify: controller inventory widget={controller_default.get_editor_property('inventory_widget_class')}")
    unreal.log(f"TInventoryVerify: controller pickup action={controller_default.get_editor_property('pick_up_action')}")

    screen_blueprint = editor_assets.load_asset(f"{TARGET_DIRECTORY}/WBP_Inventory_Screen")
    screen_default = unreal.get_default_object(screen_blueprint.generated_class())
    unreal.log(f"TInventoryVerify: storage panel class={screen_default.get_editor_property('storage_panel_class')}")
    unreal.log(f"TInventoryVerify: action menu class={screen_default.get_editor_property('action_menu_widget_class')}")

    for item_name in ["DA_Item_Pistol", "DA_Item_HealthPotion", "DA_Item_Grenade", "DA_Item_Other"]:
        item = editor_assets.load_asset(f"/Game/GASTestDemo/GameObjects/{item_name}")
        unreal.log(f"TInventoryVerify: {item_name}={item}")

    input_mapping = editor_assets.load_asset("/Game/GASTestDemo/Input/IMC_Abilities")
    inventory_action = editor_assets.load_asset("/Game/GASTestDemo/Input/AbilitiesActions/IA_Inventory")
    inventory_keys = [
        str(mapping.get_editor_property("key").get_editor_property("key_name"))
        for mapping in input_mapping.get_editor_property("mappings")
        if mapping.get_editor_property("action") == inventory_action
    ]
    unreal.log(f"TInventoryVerify: inventory keys={inventory_keys}")
    for mapping in input_mapping.get_editor_property("mappings"):
        action = mapping.get_editor_property("action")
        if action:
            unreal.log(
                f"TInventoryVerify: mapping {action.get_path_name()} -> "
                f"{mapping.get_editor_property('key').get_editor_property('key_name')}"
            )


main()
