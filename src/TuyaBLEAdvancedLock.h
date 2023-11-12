#ifndef TUYA_BLE_ADVANCED_LOCK_123
#define TUYA_BLE_ADVANCED_LOCK_123

#include "TuyaBLEDevice.h"
#include <time.h>

class TuyaBLEAdvancedLock: public TuyaBLEDevice {
public:
    static const uint8_t dpUnlockStatus = 47; // [receive] bool, true = unlocked, false = locked
    static const uint8_t dpLockUnlock = 71; // [send] raw

    // get these from the tuya API: dp 71 returns a base64 encoded binary string:
    // tuya api value: "AAH//zE2MTgzNjM0AWVCKTMAAA==="
    // hex value:       00 01 ff ff 31 31 31 31 31 31 31 31 01 65 42 29 33 00 00
    //                  PP PP CC CC RR RR RR RR RR RR RR RR OO TT TT TT TT XX YY
    //  -> PP PP   => peripheral id
    //  -> CC CC   => central id
    //  -> RR...RR => central random number as an ascii string (in this case "11111111")
    //  -> OO      => operation (0 = lock, 1 = unlock)
    //  -> TT..TT  => unix timestamp
    //  -> XX      => mobile phone
    // ->  YY      => member id
    uint16_t centralId = 0xFFFF;
    uint16_t peripheralId = 0x0001;    
    String centralRandomNumber; // 8 characters

    using TuyaBLEDevice::TuyaBLEDevice;

    /// sets the needed values from an API value
    void setFromTuyaDP71Base64EncodedValue(const String& base64EncodedValue);

    bool isLocked() const { return reportedBooleanDataPoint(dpUnlockStatus, false); }

    void unlock(uint8_t memberId = 1, std::function<void(TuyaBLEDevice*)> callback = nullptr) {
        lockUnlock(memberId, false, callback);
    }

    void lock(uint8_t memberId = 1, std::function<void(TuyaBLEDevice*)> callback = nullptr) {
        lockUnlock(memberId, true, callback);
    }


private:
    void lockUnlock(uint8_t memberId, bool shouldLock, std::function<void(TuyaBLEDevice*)> callback);
};

#endif//TUYA_BLE_ADVANCED_LOCK_123