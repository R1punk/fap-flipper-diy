#pragma once

#include <furi.h>
#include <stdint.h>

// Attack types understood by the spam engine
typedef enum {
    BleSpamAttackAppleDevice,
    BleSpamAttackAppleAction,
    BleSpamAttackAppleNotYourDevice,
    BleSpamAttackFastPair,
    BleSpamAttackSwiftPair,
    BleSpamAttackSamsungBuds,
    BleSpamAttackSamsungWatch,
    BleSpamAttackXiaomi,
    BleSpamAttackPairSpam,
    BleSpamAttackPairSpamRickroll,
    BleSpamAttackPairSpamCustom,
    BleSpamAttackCount,
} BleSpamAttackType;

// Tracked device info for the BLE tracker feature
typedef struct {
    char name[32];
    uint8_t address[6];
    int8_t rssi;
    uint32_t last_seen;
    bool connected;
} TrackerDevice;

// Well-known 128-bit service UUIDs used by advertising spam payloads
// (Apple Fast Pair / Swift Pair / Samsung / Xiaomi etc.)

// 128-bit UUID as 16 bytes (little-endian byte order as broadcast)
typedef struct {
    uint8_t uuid[16];
} BleSpamUuid128;

// Fast Pair (Google) service UUID: 0000fe2c-0000-1000-8000-00805f9b34fb
#define BLE_SPAM_UUID_FAST_PAIR \
    {0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 0x00, 0x00, 0x2c, 0xfe, 0x00, 0x00}

// Swift Pair (Microsoft) service UUID: 00000611-0000-1000-8000-00805f9b34fb
#define BLE_SPAM_UUID_SWIFT_PAIR \
    {0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 0x00, 0x00, 0x11, 0x06, 0x00, 0x00}

// Samsung Watch service UUID: 00009004-0000-1000-8000-00805f9b34fb
#define BLE_SPAM_UUID_SAMSUNG_WATCH \
    {0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 0x00, 0x00, 0x04, 0x90, 0x00, 0x00}

// Xiaomi service UUID: 0000fe95-0000-1000-8000-00805f9b34fb
#define BLE_SPAM_UUID_XIAOMI \
    {0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 0x00, 0x00, 0x95, 0xfe, 0x00, 0x00}

// Apple manufacturer ID used in manufacturer-specific data
#define BLE_SPAM_APPLE_COMPANY_ID 0x004C

// Return a pointer to the UUID bytes for the given attack type
const uint8_t* ble_uuid_db_get_service_uuid(BleSpamAttackType attack);