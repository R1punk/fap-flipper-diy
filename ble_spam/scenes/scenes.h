#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <gui/scene_manager.h>

// Generate scene enums (Submenu=0, Running=1, ..., RaceDetector=8, SceneNum=9)
#define ADD_SCENE(prefix, name, id) BleSpamScene##id,
typedef enum {
#include "scene_config.h"
    BleSpamSceneNum,
} BleSpamScene;
#undef ADD_SCENE

// Global scene handlers registry
extern const SceneManagerHandlers ble_spam_scene_handlers;

// Generate scene on_enter handlers declaration
#define ADD_SCENE(prefix, name, id) void prefix##_scene_##name##_on_enter(void*);
#include "scene_config.h"
#undef ADD_SCENE

// Generate scene on_event handlers declaration
#define ADD_SCENE(prefix, name, id) \
    bool prefix##_scene_##name##_on_event(void* context, SceneManagerEvent event);
#include "scene_config.h"
#undef ADD_SCENE

// Generate scene on_exit handlers declaration
#define ADD_SCENE(prefix, name, id) void prefix##_scene_##name##_on_exit(void* context);
#include "scene_config.h"
#undef ADD_SCENE