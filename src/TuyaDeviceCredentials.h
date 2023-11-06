#ifndef TUYA_CREDENTIALS_123
#define TUYA_CREDENTIALS_123

#include <Arduino.h>

/// Credentials for a TuyaDevice: these can be found in the CloudAPI.
struct TuyaDeviceCredentials {
private:
    /// @brief the uuid of this device
    String _uuid;

    /// @brief the id of the device
    String _deviceId;

    /// @brief  the local key of the device
    String _localKey;

public:
    TuyaDeviceCredentials(const String& uuid, const String& deviceId, const String& localKey) 
    :  _uuid(uuid), _deviceId(deviceId), _localKey(localKey) {
    }

    const String& uuid() const { return _uuid; }
    const String& deviceId() const { return _deviceId; }
    const String& localKey() const { return _localKey; }
};

#endif//TUYA_CREDENTIALS_123