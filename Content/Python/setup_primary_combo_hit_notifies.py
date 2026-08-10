"""
在编辑器中执行（Output Log / Python 控制台）：
  py "Content/Python/setup_primary_combo_hit_notifies.py"

为 AM_Attack_1/2/3 添加 AnimNotifyState_ComboHit，并移除旧的 AN_SendEventToActor 命中 Notify。
保留 ANF_AttackWindow（连招输入窗口）。
"""

import unreal

COMBO_HIT_CLASS_PATH = "/Script/GASTestDemo1.AnimNotifyState_ComboHit"
NOTIFY_TRACK = "Notifies"
SEND_EVENT_NOTIFY_NAME = "AN_SendEventToActor_C"

# (montage_path, socket_name, start_time, duration)
MONTAGE_SETUPS = [
    (
        "/Game/GASTestDemo/Characters/PlayerCharacters/Animations/Test/AM_Attack_1",
        "hand_r",
        0.25,
        0.35,
    ),
    (
        "/Game/GASTestDemo/Characters/PlayerCharacters/Animations/Test/AM_Attack_2",
        "hand_l",
        0.25,
        0.35,
    ),
    (
        "/Game/GASTestDemo/Characters/PlayerCharacters/Animations/Test/AM_Attack_3",
        "foot_r",
        0.25,
        0.35,
    ),
]


def _get_combo_hit_class():
    return unreal.load_class(None, COMBO_HIT_CLASS_PATH)


def _class_name(obj):
    if obj is None:
        return ""
    try:
        return obj.get_class().get_name()
    except Exception:
        return str(obj)


def _is_combo_hit_notify(event):
    ns = getattr(event, "notify_state_class", None)
    return ns is not None and "ComboHit" in _class_name(ns)


def _has_combo_hit(montage):
    try:
        notifies = unreal.AnimationLibrary.get_animation_notify_events(montage)
    except Exception:
        return False
    return any(_is_combo_hit_notify(event) for event in notifies)


def _remove_send_event_notifies(montage):
    try:
        return unreal.AnimationLibrary.remove_animation_notify_events_by_name(
            montage, unreal.Name(SEND_EVENT_NOTIFY_NAME)
        )
    except Exception as exc:
        unreal.log_warning(f"未能自动删除 SendEvent Notify: {exc}")
        return 0


def _ensure_notify_track(montage):
    try:
        tracks = unreal.AnimationLibrary.get_animation_notify_track_names(montage)
        if NOTIFY_TRACK not in [str(t) for t in tracks]:
            unreal.AnimationLibrary.add_animation_notify_track(montage, NOTIFY_TRACK)
    except Exception as exc:
        unreal.log_warning(f"Notify Track 检查失败（可忽略）: {exc}")


def _set_prop(obj, names, value):
    for name in names:
        try:
            obj.set_editor_property(name, value)
            return True
        except Exception:
            continue
    return False


def _add_combo_hit(montage, socket_name, start_time, duration, notify_class):
    if _has_combo_hit(montage):
        unreal.log(f"已存在 ComboHit，跳过添加: {montage.get_path_name()}")
        return True

    notify_state = unreal.AnimationLibrary.add_animation_notify_state_event(
        montage,
        NOTIFY_TRACK,
        start_time,
        duration,
        notify_class,
    )
    if notify_state is None:
        unreal.log_error(f"添加 ComboHit 失败: {montage.get_path_name()}")
        return False

    socket_names = unreal.Array(unreal.Name)
    socket_names.append(unreal.Name(socket_name))
    if not _set_prop(notify_state, ["socket_names", "SocketNames"], socket_names):
        unreal.log_warning("未能设置 SocketNames，请在编辑器中手动填写。")

    _set_prop(notify_state, ["sphere_radius", "SphereRadius"], 30.0)
    _set_prop(notify_state, ["draw_debug", "b_draw_debug", "bDrawDebug"], False)

    unreal.log(
        f"已添加 ComboHit socket={socket_name} @ {start_time}+{duration} -> {montage.get_path_name()}"
    )
    return True


def main():
    notify_class = _get_combo_hit_class()
    if notify_class is None:
        unreal.log_error(
            "找不到 AnimNotifyState_ComboHit。请先编译 GASTestDemo1 模块后再运行本脚本。"
        )
        return

    for montage_path, socket_name, start_time, duration in MONTAGE_SETUPS:
        montage = unreal.EditorAssetLibrary.load_asset(montage_path)
        if montage is None:
            unreal.log_error(f"加载失败: {montage_path}")
            continue

        _ensure_notify_track(montage)
        removed = _remove_send_event_notifies(montage)
        if removed > 0:
            unreal.log(f"已移除 {removed} 个旧 SendEvent Notify: {montage_path}")

        _add_combo_hit(montage, socket_name, start_time, duration, notify_class)
        unreal.EditorAssetLibrary.save_asset(montage_path)

    unreal.log("Primary Combo Hit Notify 配置完成。请在蒙太奇时间轴上核对命中窗口与输入窗口位置。")


if __name__ == "__main__":
    main()
