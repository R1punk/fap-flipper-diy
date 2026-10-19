#pragma once

#include <furi.h>
#include <stdint.h>

// Device/payload types supported by the spam engine
typedef enum {
    SpamDevTypeApplePair, // Apple ProximityPair (AirPods/AirTag popups)
    SpamDevTypeAppleAction, // Apple NearbyAction (Vision Pro)
    SpamDevTypeSamsung, // Samsung Buds (0xFE75)
    SpamDevTypeSamsungWatch, // Samsung Watch (0xFE75)
    SpamDevTypeFastPair, // Google Fast Pair (0xFE2C)
    SpamDevTypeSwiftPair, // Microsoft Swift Pair
    SpamDevTypeRotate, // Cycle through all of the above
} SpamDevType;

typedef struct BleSpamEngine BleSpamEngine;

// Allocate/free the spam engine (worker thread + beacon state)
BleSpamEngine* ble_spam_engine_alloc(void);
void ble_spam_engine_free(BleSpamEngine* engine);

// Device list
uint8_t ble_spam_engine_get_dev_count(void);
const char* ble_spam_engine_get_dev_name(uint8_t index);
uint8_t ble_spam_engine_get_dev(BleSpamEngine* engine);
void ble_spam_engine_set_dev(BleSpamEngine* engine, uint8_t index);

// Start/stop broadcasting the selected device payload
void ble_spam_engine_start(BleSpamEngine* engine);
void ble_spam_engine_stop(BleSpamEngine* engine);
bool ble_spam_engine_is_running(BleSpamEngine* engine);

// Stats
uint32_t ble_spam_engine_get_packet_count(BleSpamEngine* engine);

// Identity hold time in ms (200..10000): how long one MAC+payload identity
// is advertised continuously before rotating to a fresh one. Short holds
// change identity very often; long holds mimic a real stable device.
void ble_spam_engine_set_hold(BleSpamEngine* engine, uint32_t hold_ms);
uint32_t ble_spam_engine_get_hold(BleSpamEngine* engine);

// Last beacon start result (for on-screen diagnostics).
// true = beacon confirmed started, false = failed/never started.
bool ble_spam_engine_get_beacon_ok(BleSpamEngine* engine);