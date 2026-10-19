#include <furi.h>
#include "ble_spam_app.h"
#include "views/ble_spam_view.h"
#include "views/ble_walk_scan_view.h"
#include "views/ble_walk_detail_view.h"
#include "views/ble_auto_walk_view.h"
#include "views/tracker_list_view.h"
#include "views/tracker_geiger_view.h"
#include "views/race_detector_view.h"

#include "scenes/scenes.h"

static bool ble_spam_custom_event_callback(void* context, uint32_t event) {
    furi_assert(context);
    BleSpamApp* app = context;
    return scene_manager_handle_custom_event(app->scene_manager, event);
}

static bool ble_spam_back_event_callback(void* context) {
    furi_assert(context);
    BleSpamApp* app = context;
    return scene_manager_handle_back_event(app->scene_manager);
}

static void ble_spam_tick_event_callback(void* context) {
    furi_assert(context);
    BleSpamApp* app = context;
    scene_manager_handle_tick_event(app->scene_manager);
}

static BleSpamApp* ble_spam_app_alloc(void) {
    BleSpamApp* app = malloc(sizeof(BleSpamApp));
    app->gui = furi_record_open(RECORD_GUI);

    app->scene_manager = scene_manager_alloc(&ble_spam_scene_handlers, app);
    app->view_dispatcher = view_dispatcher_alloc();

    view_dispatcher_set_event_callback_context(app->view_dispatcher, app);
    view_dispatcher_set_custom_event_callback(app->view_dispatcher, ble_spam_custom_event_callback);
    view_dispatcher_set_navigation_event_callback(
        app->view_dispatcher, ble_spam_back_event_callback);
    view_dispatcher_set_tick_event_callback(
        app->view_dispatcher, ble_spam_tick_event_callback, 250);
    view_dispatcher_attach_to_gui(app->view_dispatcher, app->gui, ViewDispatcherTypeFullscreen);

    // Submenu view
    app->submenu = submenu_alloc();
    view_dispatcher_add_view(app->view_dispatcher, BleSpamViewSubmenu, submenu_get_view(app->submenu));

    // Text input view
    app->text_input = text_input_alloc();
    view_dispatcher_add_view(
        app->view_dispatcher, BleSpamViewTextInput, text_input_get_view(app->text_input));

    // Running view
    app->view_running = ble_spam_view_alloc();
    view_set_context(app->view_running, app->view_dispatcher);
    view_dispatcher_add_view(app->view_dispatcher, BleSpamViewRunning, app->view_running);

    // BLE Walk views
    app->view_walk_scan = ble_walk_scan_view_alloc();
    view_set_context(app->view_walk_scan, app->view_dispatcher);
    view_dispatcher_add_view(app->view_dispatcher, BleSpamViewWalkScan, app->view_walk_scan);

    app->view_walk_detail = ble_walk_detail_view_alloc();
    view_set_context(app->view_walk_detail, app->view_dispatcher);
    view_dispatcher_add_view(app->view_dispatcher, BleSpamViewWalkDetail, app->view_walk_detail);

    // BLE Auto-Walk view
    app->view_auto_walk = ble_auto_walk_view_alloc();
    view_set_context(app->view_auto_walk, app->view_dispatcher);
    view_dispatcher_add_view(app->view_dispatcher, BleSpamViewAutoWalk, app->view_auto_walk);

    // BLE Tracker views
    app->view_tracker_scan = tracker_list_view_alloc();
    view_set_context(app->view_tracker_scan, app->view_dispatcher);
    view_dispatcher_add_view(app->view_dispatcher, BleSpamViewTrackerScan, app->view_tracker_scan);

    app->view_tracker_geiger = tracker_geiger_view_alloc();
    view_set_context(app->view_tracker_geiger, app->view_dispatcher);
    view_dispatcher_add_view(app->view_dispatcher, BleSpamViewTrackerGeiger, app->view_tracker_geiger);

    // RACE Detector view
    app->view_race_detector = race_detector_view_alloc();
    view_set_context(app->view_race_detector, app->view_dispatcher);
    view_dispatcher_add_view(app->view_dispatcher, BleSpamViewRaceDetector, app->view_race_detector);

    // Spam engine (worker thread driving the extra beacon)
    app->spam_engine = ble_spam_engine_alloc();

    // Enter the first scene; its on_enter switches to the submenu view
    scene_manager_next_scene(app->scene_manager, BleSpamSceneSubmenu);

    return app;
}

static void ble_spam_app_free(BleSpamApp* app) {
    furi_assert(app);

    // Free views
    view_dispatcher_remove_view(app->view_dispatcher, BleSpamViewSubmenu);
    submenu_free(app->submenu);

    view_dispatcher_remove_view(app->view_dispatcher, BleSpamViewTextInput);
    text_input_free(app->text_input);

    view_dispatcher_remove_view(app->view_dispatcher, BleSpamViewRunning);
    ble_spam_view_free(app->view_running);

    view_dispatcher_remove_view(app->view_dispatcher, BleSpamViewWalkScan);
    ble_walk_scan_view_free(app->view_walk_scan);

    view_dispatcher_remove_view(app->view_dispatcher, BleSpamViewWalkDetail);
    ble_walk_detail_view_free(app->view_walk_detail);

    view_dispatcher_remove_view(app->view_dispatcher, BleSpamViewAutoWalk);
    ble_auto_walk_view_free(app->view_auto_walk);

    view_dispatcher_remove_view(app->view_dispatcher, BleSpamViewTrackerScan);
    tracker_list_view_free(app->view_tracker_scan);

    view_dispatcher_remove_view(app->view_dispatcher, BleSpamViewTrackerGeiger);
    tracker_geiger_view_free(app->view_tracker_geiger);

    view_dispatcher_remove_view(app->view_dispatcher, BleSpamViewRaceDetector);
    race_detector_view_free(app->view_race_detector);

    // Stop and free the spam engine
    ble_spam_engine_stop(app->spam_engine);
    ble_spam_engine_free(app->spam_engine);
    app->spam_engine = NULL;

    // Free scene manager
    scene_manager_free(app->scene_manager);

    // Free view dispatcher
    view_dispatcher_free(app->view_dispatcher);
    app->view_dispatcher = NULL;

    // Close gui record
    furi_record_close(RECORD_GUI);
    app->gui = NULL;

    free(app);
}

int32_t ble_spam_app(void* p) {
    UNUSED(p);
    BleSpamApp* app = ble_spam_app_alloc();
    view_dispatcher_run(app->view_dispatcher);
    ble_spam_app_free(app);
    return 0;
}
