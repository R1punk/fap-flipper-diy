#include <furi.h>
#include <gui/elements.h>
#include <gui/view_dispatcher.h>
#include "ble_walk_detail_view.h"

static void ble_walk_detail_view_draw_callback(Canvas* canvas, void* model) {
    UNUSED(model);
    canvas_clear(canvas);
    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str(canvas, 4, 10, "BLE Walk Detail");
    canvas_set_font(canvas, FontSecondary);
    canvas_draw_str(canvas, 4, 30, "Device details");
}

static bool ble_walk_detail_view_input_callback(InputEvent* event, void* context) {
    UNUSED(event);
    UNUSED(context);
    return false;
}

View* ble_walk_detail_view_alloc(void) {
    View* view = view_alloc();
    view_set_draw_callback(view, ble_walk_detail_view_draw_callback);
    view_set_input_callback(view, ble_walk_detail_view_input_callback);
    return view;
}

void ble_walk_detail_view_free(View* view) {
    furi_assert(view);
    view_free(view);
}
