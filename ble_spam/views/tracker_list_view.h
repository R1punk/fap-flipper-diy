#pragma once
#include <gui/view.h>

#include "ble_spam_app.h"

View* tracker_list_view_alloc(void);
void tracker_list_view_free(View* view);