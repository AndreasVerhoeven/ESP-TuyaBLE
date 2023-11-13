#include "TuyaBLEAdvancedLock.h"
#include "mbedtls/base64.h"
#include <time.h>


/// sets the needed values from an API value
void TuyaBLEAdvancedLock::setFromTuyaDP71Base64EncodedValue(const String& base64EncodedValue) {
    Buffer data(base64EncodedValue.length() * 4 / 3);
    size_t outputLength = 0;
    mbedtls_base64_decode(data.data(), data.size(), &outputLength, (const unsigned char*)base64EncodedValue.c_str(), base64EncodedValue.length());

    if(outputLength >= 12) {
        size_t offset = 0;
        peripheralId = data.readBigEndianUint16(offset);
        centralId = data.readBigEndianUint16(offset);
        centralRandomNumber = String(data.data() + 4, 8);
    }
}

void TuyaBLEAdvancedLock::lockUnlock(uint8_t memberId, bool shouldLock, std::function<void(TuyaBLEDevice*)> callback) {
    /// https://developer.tuya.com/en/docs/iot/title?id=K9nmje3twsy7n#title-27-Locking%20and%20unlocking    
    Buffer data;
    data.appendBigEndian(centralId); // central id = 0xFFFF
    data.appendBigEndian(peripheralId); // peripheral id
    data.append(centralRandomNumber); // 8 characters
    data.append(uint8_t(shouldLock ? 0 : 1)); // lock = 0, unlock = 1
    data.appendBigEndian(uint32_t(time(nullptr))); // timestamp
    data.append(0x00); // mobile phone
    data.append(memberId);

    sendDataPoint(TuyaDataPoint::raw(dpLockUnlock, data), callback);
}