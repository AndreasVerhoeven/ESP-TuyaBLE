#ifndef TUYA_BLE_SIMPLE_LOCK_123
#define TUYA_BLE_SIMPLE_LOCK_123

#include "TuyaBLEDevice.h"

class TuyaBLESimpleLock: public TuyaBLEDevice {
public:
    static const uint8_t dpShortRangeUnlock = 6; // [send] raw
    static const uint8_t dpBatteryLevel = 9; // [receive] enum, 0 = high, 1 = medium, 2 = low, 3 = exhausted
    static const uint8_t dpUnlockStatus = 47; // [receive] bool, true = unlocked, false = locked

    using TuyaBLEDevice::TuyaBLEDevice;

    bool isLocked() const { return reportedBooleanDataPoint(dpUnlockStatus, false); }

    void shortRangeUnlock(uint8_t memberId = 1, std::function<void(TuyaBLEDevice*)> callback = nullptr) {
        sendDataPoint(TuyaDataPoint::raw(dpShortRangeUnlock, {1, memberId}), callback);
    }

    void shortRangeLock(uint8_t memberId = 1, std::function<void(TuyaBLEDevice*)> callback = nullptr) {
        sendDataPoint(TuyaDataPoint::raw(dpShortRangeUnlock, {0, memberId}), callback);
    }
};

#endif//TUYA_BLE_SIMPLE_LOCK_123