#pragma once
#include <gui/view.h>

#include "ble_spam_app.h"

View* ble_auto_walk_view_alloc(void);
void ble_auto_walk_view_free(View* view);