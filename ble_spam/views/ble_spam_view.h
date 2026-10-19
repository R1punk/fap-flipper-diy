#pragma once
#include <gui/view.h>

View* ble_spam_view_alloc(void);
void ble_spam_view_free(View* view);

// Refresh the running view state
void ble_spam_view_update(
    View* view,
    const char* device_name,
    bool running,
    bool beacon_ok,
    uint32_t packet_count,
    uint32_t hold_ms);