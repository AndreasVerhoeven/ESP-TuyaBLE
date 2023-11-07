#ifndef TUYA_BLE_ADVERTISED_DEVICE_INFO_123
#define TUYA_BLE_ADVERTISED_DEVICE_INFO_123

#include <Arduino.h>
#include <NimBLEDevice.h>

class TuyaBLEDevice;

/// The info extracted from a BLE advertised device. Note that scans need to be active, not passive (otherwise we cannot read the manufacturer data).
class TuyaBLEAdvertisedDeviceInfo {
    friend class TuyaBLEDevice;

protected:
    bool _isValid = true;
    NimBLEAddress _address;
    bool _isBound = false;
    uint8_t _protocolVersion = 3;
    uint8_t _encryptionMethod = 0;
    uint16_t _communicationCapacity = 0;
    String _uuid;

    TuyaBLEAdvertisedDeviceInfo() : _isValid(false) {}
    TuyaBLEAdvertisedDeviceInfo(const NimBLEAddress& address, bool isBound, uint8_t protocolVersion, uint8_t encryptionMethod, uint16_t communicationCapacity, const String& uuid) :
    _address(address), _isBound(isBound), _protocolVersion(protocolVersion), _encryptionMethod(encryptionMethod), _communicationCapacity(communicationCapacity), _uuid(uuid) {}

public:
    /// returns the tuya device info for an advertised device, if it is a tuya ble device. std::nullopt otherwise.
    TuyaBLEAdvertisedDeviceInfo fromBLEAdvertisedDevice(NimBLEAdvertisedDevice bleAdvertisedDevice);

    static const TuyaBLEAdvertisedDeviceInfo invalid;

    bool isValid() const { return _isValid; }

    const NimBLEAddress& address() const { return _address; }
    bool isBound() const { return _isBound; }
    uint8_t protocolVersion() const { return _protocolVersion; }
    uint8_t encryptionMethod() const { return _encryptionMethod; }
    uint16_t communicationCapacity() const { return _communicationCapacity; }
    const String& uuid() const { return _uuid; }

};


#endif//TUYA_BLE_ADVERTISED_DEVICE_INFO_123