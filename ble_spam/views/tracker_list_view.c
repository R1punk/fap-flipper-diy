#include <furi.h>
#include <gui/elements.h>
#include <gui/view_dispatcher.h>
#include "tracker_list_view.h"

static void tracker_list_view_draw_callback(Canvas* canvas, void* model) {
    UNUSED(model);
    canvas_clear(canvas);
    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str(canvas, 4, 10, "BLE Tracker");
    canvas_set_font(canvas, FontSecondary);
    canvas_draw_str(canvas, 4, 30, "Scanning trackers...");
}

static bool tracker_list_view_input_callback(InputEvent* event, void* context) {
    UNUSED(event);
    UNUSED(context);
    return false;
}

View* tracker_list_view_alloc(void) {
    View* view = view_alloc();
    view_set_draw_callback(view, tracker_list_view_draw_callback);
    view_set_input_callback(view, tracker_list_view_input_callback);
    return view;
}

void tracker_list_view_free(View* view) {
    furi_assert(view);
    view_free(view);
}
