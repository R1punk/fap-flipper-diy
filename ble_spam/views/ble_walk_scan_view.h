#pragma once
#include <gui/view.h>

#include "ble_spam_app.h"

View* ble_walk_scan_view_alloc(void);
void ble_walk_scan_view_free(View* view);