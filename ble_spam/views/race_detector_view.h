#pragma once
#include <gui/view.h>

#include "ble_spam_app.h"

View* race_detector_view_alloc(void);
void race_detector_view_free(View* view);