import unreal


ROOT = "/Game/GASTestDemo/UI/Inventory/UI"
NAMES = [
    "WBP_Inventory_Screen1",
    "WBP_Inventory_Panel1",
    "WBP_Inventory_Slot_Grid1",
    "WBP_Inventory_Slot1",
    "WBP_Inventory_ActionMenu1",
    "WBP_Inventory_Discard_Zone1",
    "WBP_Inventory_Dragging1",
    "WBP_QuickSlots_Panel1",
    "WBP_QuickBar_Slot1",
]


def main():
    assets = unreal.get_editor_subsystem(unreal.EditorAssetSubsystem)
    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    options = unreal.AssetRegistryDependencyOptions(
        include_soft_package_references=True,
        include_hard_package_references=True,
        include_searchable_names=True,
        include_soft_management_references=True,
        include_hard_management_references=True,
    )
    loaded = {name: assets.load_asset(f"{ROOT}/{name}") for name in NAMES}

    forbidden = []
    for name in NAMES:
        for dependency in registry.get_dependencies(f"{ROOT}/{name}", options) or []:
            dependency_name = str(dependency)
            if dependency_name.startswith("/Game/VisualSandbox/Blueprints"):
                forbidden.append(f"{name} -> {dependency_name}")
    if forbidden:
        for dependency in forbidden:
            unreal.log_error(f"TInventoryUI1Verify: forbidden dependency {dependency}")
        raise RuntimeError("VisualSandbox Blueprint dependencies remain")

    controller = assets.load_asset("/Game/GASTestDemo/Player/BP_T_PlayerController")
    controller_default = unreal.get_default_object(controller.generated_class())
    screen_class = controller_default.get_editor_property("inventory_widget_class")
    expected_screen_class = loaded["WBP_Inventory_Screen1"].generated_class()
    if screen_class != expected_screen_class:
        raise RuntimeError(f"controller uses {screen_class}, expected {expected_screen_class}")

    screen_default = unreal.get_default_object(expected_screen_class)
    if screen_default.get_editor_property("storage_panel_class") != loaded["WBP_Inventory_Panel1"].generated_class():
        raise RuntimeError("wrong storage panel class")
    if screen_default.get_editor_property("action_menu_widget_class") != loaded["WBP_Inventory_ActionMenu1"].generated_class():
        raise RuntimeError("wrong action menu class")

    grid_default = unreal.get_default_object(loaded["WBP_Inventory_Slot_Grid1"].generated_class())
    if grid_default.get_editor_property("slot_widget_class") != loaded["WBP_Inventory_Slot1"].generated_class():
        raise RuntimeError("wrong inventory slot class")

    slot_default = unreal.get_default_object(loaded["WBP_Inventory_Slot1"].generated_class())
    if slot_default.get_editor_property("dragging_widget_class") != loaded["WBP_Inventory_Dragging1"].generated_class():
        raise RuntimeError("wrong dragging widget class")

    weapon_bar = loaded["WBP_QuickSlots_Panel1"]
    weapon_slot = loaded["WBP_QuickBar_Slot1"]
    weapon_bar_default = unreal.get_default_object(weapon_bar.generated_class())
    if weapon_bar_default.get_editor_property("quick_slot_widget_class") != weapon_slot.generated_class():
        raise RuntimeError("wrong quick slot widget class")

    screen_widgets = {}
    for widget in unreal.ObjectIterator(unreal.Widget):
        path = widget.get_path_name()
        if f"{ROOT}/WBP_Inventory_Screen1.WBP_Inventory_Screen1:WidgetTree." in path:
            screen_widgets[widget.get_name()] = widget.get_class().get_name()
    expected_widgets = {
        "Inventory_Panel": "WBP_Inventory_Panel1_C",
        "ItemDiscardZone": "WBP_Inventory_Discard_Zone1_C",
        "QuickBar": "WBP_QuickSlots_Panel1_C",
    }
    for widget_name, widget_class in expected_widgets.items():
        if screen_widgets.get(widget_name) != widget_class:
            raise RuntimeError(f"{widget_name}={screen_widgets.get(widget_name)}, expected {widget_class}")
    if "Crafting_Panel" in screen_widgets:
        raise RuntimeError("Crafting_Panel was not removed")

    unreal.log(f"TInventoryUI1Verify: controller={screen_class.get_path_name()}")
    unreal.log(f"TInventoryUI1Verify: screen_widgets={screen_widgets}")
    unreal.log("TInventoryUI1Verify: no VisualSandbox Blueprint dependency")
    unreal.log("TInventoryUI1Verify: success")


main()
