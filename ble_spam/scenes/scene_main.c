#include "scenes.h"
#include "../ble_spam_app.h"
#include "../views/ble_spam_view.h"
#include <furi.h>
#include <string.h>

// ===== Scene 0: Submenu =====

enum BleSpamSubmenuIndex {
    BleSpamSubmenuIndexRunSpam,
    BleSpamSubmenuIndexCustomName,
    BleSpamSubmenuIndexAutoWalk,
    BleSpamSubmenuIndexTracker,
    BleSpamSubmenuIndexRaceDetector,
};

static void ble_spam_scene_submenu_callback(void* context, uint32_t index) {
    furi_assert(context);
    BleSpamApp* app = context;
    view_dispatcher_send_custom_event(app->view_dispatcher, index);
}

void ble_spam_scene_submenu_on_enter(void* context) {
    furi_assert(context);
    BleSpamApp* app = context;

    submenu_reset(app->submenu);
    submenu_set_header(app->submenu, "BLE Spam");
    submenu_add_item(
        app->submenu,
        "Run spam",
        BleSpamSubmenuIndexRunSpam,
        ble_spam_scene_submenu_callback,
        app);
    submenu_add_item(
        app->submenu,
        "Custom pair name",
        BleSpamSubmenuIndexCustomName,
        ble_spam_scene_submenu_callback,
        app);
    submenu_add_item(
        app->submenu,
        "Auto walk",
        BleSpamSubmenuIndexAutoWalk,
        ble_spam_scene_submenu_callback,
        app);
    submenu_add_item(
        app->submenu,
        "Tracker",
        BleSpamSubmenuIndexTracker,
        ble_spam_scene_submenu_callback,
        app);
    submenu_add_item(
        app->submenu,
        "Race detector",
        BleSpamSubmenuIndexRaceDetector,
        ble_spam_scene_submenu_callback,
        app);

    view_dispatcher_switch_to_view(app->view_dispatcher, BleSpamViewSubmenu);
}

bool ble_spam_scene_submenu_on_event(void* context, SceneManagerEvent event) {
    furi_assert(context);
    BleSpamApp* app = context;
    bool consumed = false;

    if(event.type == SceneManagerEventTypeCustom) {
        switch(event.event) {
        case BleSpamSubmenuIndexRunSpam:
            scene_manager_next_scene(app->scene_manager, BleSpamSceneRunning);
            consumed = true;
            break;
        case BleSpamSubmenuIndexCustomName:
            scene_manager_next_scene(app->scene_manager, BleSpamSceneTextInput);
            consumed = true;
            break;
        case BleSpamSubmenuIndexAutoWalk:
            scene_manager_next_scene(app->scene_manager, BleSpamSceneAutoWalk);
            consumed = true;
            break;
        case BleSpamSubmenuIndexTracker:
            scene_manager_next_scene(app->scene_manager, BleSpamSceneTrackerScan);
            consumed = true;
            break;
        case BleSpamSubmenuIndexRaceDetector:
            scene_manager_next_scene(app->scene_manager, BleSpamSceneRaceDetector);
            consumed = true;
            break;
        default:
            break;
        }
    }

    return consumed;
}

void ble_spam_scene_submenu_on_exit(void* context) {
    furi_assert(context);
    BleSpamApp* app = context;
    submenu_reset(app->submenu);
}

// ===== Scene 1: Running =====

static void ble_spam_scene_running_refresh_view(BleSpamApp* app) {
    ble_spam_view_update(
        app->view_running,
        ble_spam_engine_get_dev_name(ble_spam_engine_get_dev(app->spam_engine)),
        ble_spam_engine_is_running(app->spam_engine),
        ble_spam_engine_get_beacon_ok(app->spam_engine),
        ble_spam_engine_get_packet_count(app->spam_engine),
        ble_spam_engine_get_hold(app->spam_engine));
}

void ble_spam_scene_running_on_enter(void* context) {
    furi_assert(context);
    BleSpamApp* app = context;
    app->running = false;
    ble_spam_scene_running_refresh_view(app);
    view_dispatcher_switch_to_view(app->view_dispatcher, BleSpamViewRunning);
}

bool ble_spam_scene_running_on_event(void* context, SceneManagerEvent event) {
    furi_assert(context);
    BleSpamApp* app = context;
    bool consumed = false;

    if(event.type == SceneManagerEventTypeCustom) {
        uint8_t dev = ble_spam_engine_get_dev(app->spam_engine);
        uint8_t dev_count = ble_spam_engine_get_dev_count();
        uint32_t hold;

        switch(event.event) {
        case BleSpamCustomEventToggle:
            if(ble_spam_engine_is_running(app->spam_engine)) {
                ble_spam_engine_stop(app->spam_engine);
            } else {
                ble_spam_engine_start(app->spam_engine);
            }
            ble_spam_scene_running_refresh_view(app);
            consumed = true;
            break;
        case BleSpamCustomEventSelectPrev:
            ble_spam_engine_set_dev(app->spam_engine, (dev + dev_count - 1) % dev_count);
            ble_spam_scene_running_refresh_view(app);
            consumed = true;
            break;
        case BleSpamCustomEventSelectNext:
            ble_spam_engine_set_dev(app->spam_engine, (dev + 1) % dev_count);
            ble_spam_scene_running_refresh_view(app);
            consumed = true;
            break;
        case BleSpamCustomEventSpeedDown: // Up key: longer hold
            hold = ble_spam_engine_get_hold(app->spam_engine) + 100;
            ble_spam_engine_set_hold(app->spam_engine, hold);
            ble_spam_scene_running_refresh_view(app);
            consumed = true;
            break;
        case BleSpamCustomEventSpeedUp: // Down key: shorter hold
            hold = ble_spam_engine_get_hold(app->spam_engine);
            if(hold > 100) hold -= 100;
            ble_spam_engine_set_hold(app->spam_engine, hold);
            ble_spam_scene_running_refresh_view(app);
            consumed = true;
            break;
        default:
            break;
        }
    } else if(event.type == SceneManagerEventTypeTick) {
        // Periodic stats refresh from the worker thread
        ble_spam_scene_running_refresh_view(app);
    }

    // Back (and everything else unconsumed) lets the scene manager pop
    return consumed;
}

void ble_spam_scene_running_on_exit(void* context) {
    furi_assert(context);
    BleSpamApp* app = context;
    ble_spam_engine_stop(app->spam_engine);
    app->running = false;
}

// ===== Scene 2: Text input (custom pair name) =====

static void ble_spam_scene_text_input_callback(void* context) {
    furi_assert(context);
    BleSpamApp* app = context;
    view_dispatcher_send_custom_event(app->view_dispatcher, BleSpamCustomEventTextInputDone);
}

void ble_spam_scene_text_input_on_enter(void* context) {
    furi_assert(context);
    BleSpamApp* app = context;

    text_input_set_header_text(app->text_input, "Pair name");
    text_input_set_result_callback(
        app->text_input,
        ble_spam_scene_text_input_callback,
        app,
        app->custom_pair_name,
        sizeof(app->custom_pair_name),
        true);

    view_dispatcher_switch_to_view(app->view_dispatcher, BleSpamViewTextInput);
}

bool ble_spam_scene_text_input_on_event(void* context, SceneManagerEvent event) {
    furi_assert(context);
    BleSpamApp* app = context;
    bool consumed = false;

    if(event.type == SceneManagerEventTypeCustom) {
        if(event.event == BleSpamCustomEventTextInputDone) {
            // Text was written directly into app->custom_pair_name by the module
            scene_manager_previous_scene(app->scene_manager);
            consumed = true;
        }
    }

    return consumed;
}

void ble_spam_scene_text_input_on_exit(void* context) {
    furi_assert(context);
    BleSpamApp* app = context;
    text_input_reset(app->text_input);
}

// ===== Scene 3: Walk scan =====

void ble_spam_scene_walk_scan_on_enter(void* context) {
    furi_assert(context);
    BleSpamApp* app = context;
    view_dispatcher_switch_to_view(app->view_dispatcher, BleSpamViewWalkScan);
}

bool ble_spam_scene_walk_scan_on_event(void* context, SceneManagerEvent event) {
    furi_assert(context);
    UNUSED(context);
    UNUSED(event);
    return false;
}

void ble_spam_scene_walk_scan_on_exit(void* context) {
    furi_assert(context);
    UNUSED(context);
}

// ===== Scene 4: Walk detail =====

void ble_spam_scene_walk_detail_on_enter(void* context) {
    furi_assert(context);
    BleSpamApp* app = context;
    view_dispatcher_switch_to_view(app->view_dispatcher, BleSpamViewWalkDetail);
}

bool ble_spam_scene_walk_detail_on_event(void* context, SceneManagerEvent event) {
    furi_assert(context);
    UNUSED(context);
    UNUSED(event);
    return false;
}

void ble_spam_scene_walk_detail_on_exit(void* context) {
    furi_assert(context);
    UNUSED(context);
}

// ===== Scene 5: Auto walk =====

void ble_spam_scene_auto_walk_on_enter(void* context) {
    furi_assert(context);
    BleSpamApp* app = context;
    view_dispatcher_switch_to_view(app->view_dispatcher, BleSpamViewAutoWalk);
}

bool ble_spam_scene_auto_walk_on_event(void* context, SceneManagerEvent event) {
    furi_assert(context);
    UNUSED(context);
    UNUSED(event);
    return false;
}

void ble_spam_scene_auto_walk_on_exit(void* context) {
    furi_assert(context);
    UNUSED(context);
}

// ===== Scene 6: Tracker scan =====

void ble_spam_scene_tracker_scan_on_enter(void* context) {
    furi_assert(context);
    BleSpamApp* app = context;
    view_dispatcher_switch_to_view(app->view_dispatcher, BleSpamViewTrackerScan);
}

bool ble_spam_scene_tracker_scan_on_event(void* context, SceneManagerEvent event) {
    furi_assert(context);
    UNUSED(context);
    UNUSED(event);
    return false;
}

void ble_spam_scene_tracker_scan_on_exit(void* context) {
    furi_assert(context);
    UNUSED(context);
}

// ===== Scene 7: Tracker geiger =====

void ble_spam_scene_tracker_geiger_on_enter(void* context) {
    furi_assert(context);
    BleSpamApp* app = context;
    view_dispatcher_switch_to_view(app->view_dispatcher, BleSpamViewTrackerGeiger);
}

bool ble_spam_scene_tracker_geiger_on_event(void* context, SceneManagerEvent event) {
    furi_assert(context);
    UNUSED(context);
    UNUSED(event);
    return false;
}

void ble_spam_scene_tracker_geiger_on_exit(void* context) {
    furi_assert(context);
    UNUSED(context);
}

// ===== Scene 8: Race detector =====

void ble_spam_scene_race_detector_on_enter(void* context) {
    furi_assert(context);
    BleSpamApp* app = context;
    view_dispatcher_switch_to_view(app->view_dispatcher, BleSpamViewRaceDetector);
}

bool ble_spam_scene_race_detector_on_event(void* context, SceneManagerEvent event) {
    furi_assert(context);
    UNUSED(context);
    UNUSED(event);
    return false;
}

void ble_spam_scene_race_detector_on_exit(void* context) {
    furi_assert(context);
    UNUSED(context);
}
