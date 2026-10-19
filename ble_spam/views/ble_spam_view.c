#include <furi.h>
#include <string.h>
#include <gui/elements.h>
#include <gui/view_dispatcher.h>
#include "ble_spam_app.h"
#include "ble_spam_view.h"

// Running (spam) view: device selector + live stats.

typedef struct {
    const char* device_name;
    bool running;
    bool beacon_ok;
    uint32_t packet_count;
    uint32_t hold_ms;
} BleSpamViewModel;

static void ble_spam_view_draw_callback(Canvas* canvas, void* model) {
    BleSpamViewModel* m = model;
    char buf[48];

    canvas_clear(canvas);
    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str_aligned(
        canvas, 64, 8, AlignCenter, AlignCenter, m->device_name);

    canvas_set_font(canvas, FontSecondary);
    if(m->running && !m->beacon_ok) {
        // Beacon failed to start - radio stack likely off
        snprintf(buf, sizeof(buf), "Beacon FAIL!");
        canvas_draw_str(canvas, 2, 24, buf);
        snprintf(buf, sizeof(buf), "Check BLE in settings");
        canvas_draw_str(canvas, 2, 34, buf);
        snprintf(buf, sizeof(buf), "Packets: %lu", m->packet_count);
        canvas_draw_str(canvas, 2, 44, buf);
    } else {
        snprintf(buf, sizeof(buf), "Packets: %lu", m->packet_count);
        canvas_draw_str(canvas, 2, 24, buf);
        snprintf(buf, sizeof(buf), "Hold: %lums", m->hold_ms);
        canvas_draw_str(canvas, 2, 34, buf);
        canvas_draw_str(canvas, 2, 44, "Beacon: ON");
    }

    elements_button_center(canvas, m->running ? "STOP" : "START");
    elements_button_left(canvas, "<");
    elements_button_right(canvas, ">");
}

static bool ble_spam_view_input_callback(InputEvent* event, void* context) {
    furi_assert(context);
    ViewDispatcher* view_dispatcher = context;

    if(event->type == InputTypeShort || event->type == InputTypeRepeat) {
        switch(event->key) {
        case InputKeyOk:
            view_dispatcher_send_custom_event(view_dispatcher, BleSpamCustomEventToggle);
            return true;
        case InputKeyLeft:
            view_dispatcher_send_custom_event(view_dispatcher, BleSpamCustomEventSelectPrev);
            return true;
        case InputKeyRight:
            view_dispatcher_send_custom_event(view_dispatcher, BleSpamCustomEventSelectNext);
            return true;
        case InputKeyUp:
            view_dispatcher_send_custom_event(view_dispatcher, BleSpamCustomEventSpeedDown);
            return true;
        case InputKeyDown:
            view_dispatcher_send_custom_event(view_dispatcher, BleSpamCustomEventSpeedUp);
            return true;
        default:
            break;
        }
    }

    return false;
}

void ble_spam_view_update(
    View* view,
    const char* device_name,
    bool running,
    bool beacon_ok,
    uint32_t packet_count,
    uint32_t hold_ms) {
    furi_assert(view);
    with_view_model(
        view,
        BleSpamViewModel * model,
        {
            model->device_name = device_name;
            model->running = running;
            model->beacon_ok = beacon_ok;
            model->packet_count = packet_count;
            model->hold_ms = hold_ms;
        },
        true);
}

View* ble_spam_view_alloc(void) {
    View* view = view_alloc();
    view_allocate_model(view, ViewModelTypeLocking, sizeof(BleSpamViewModel));
    view_set_draw_callback(view, ble_spam_view_draw_callback);
    view_set_input_callback(view, ble_spam_view_input_callback);
    return view;
}

void ble_spam_view_free(View* view) {
    furi_assert(view);
    view_free(view);
}
