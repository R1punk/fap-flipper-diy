#include <furi.h>
#include <gui/elements.h>
#include <gui/view_dispatcher.h>
#include "tracker_geiger_view.h"

static void tracker_geiger_view_draw_callback(Canvas* canvas, void* model) {
    UNUSED(model);
    canvas_clear(canvas);
    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str(canvas, 4, 10, "Tracker Geiger");
    canvas_set_font(canvas, FontSecondary);
    canvas_draw_str(canvas, 4, 30, "RSSI: ---");
}

static bool tracker_geiger_view_input_callback(InputEvent* event, void* context) {
    UNUSED(event);
    UNUSED(context);
    return false;
}

View* tracker_geiger_view_alloc(void) {
    View* view = view_alloc();
    view_set_draw_callback(view, tracker_geiger_view_draw_callback);
    view_set_input_callback(view, tracker_geiger_view_input_callback);
    return view;
}

void tracker_geiger_view_free(View* view) {
    furi_assert(view);
    view_free(view);
}
