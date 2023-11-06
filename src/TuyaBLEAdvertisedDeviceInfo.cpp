#include "TuyaBLEAdvertisedDeviceInfo.h"

#include "Buffer.h"

std::optional<TuyaBLEAdvertisedDeviceInfo> TuyaBLEAdvertisedDeviceInfo::fromBLEAdvertisedDevice(NimBLEAdvertisedDevice bleAdvertisedDevice) {
    Buffer manufacturerData(bleAdvertisedDevice.getManufacturerData());
    if(manufacturerData.size() < 24) return std::nullopt;

    // companyid = 0x07D0
    if(manufacturerData[0] != 0xD0 || manufacturerData[1] != 0x07) return std::nullopt;

    bool isBound = (manufacturerData[2] & 0x80) != 0;
    uint8_t protocolVersion = manufacturerData[3];
    uint8_t encryptionMethod = manufacturerData[4];
    uint16_t communicationCapacity = (manufacturerData[5] << 8) | manufacturerData[6];
    // reserved field: data[7]
    Buffer encryptedUuid = manufacturerData.subRangeWithStartAndLength(8, 16);

    Buffer serviceData(bleAdvertisedDevice.getServiceData(NimBLEUUID(uint16_t(0xA201))));
    if(serviceData.size() == 0) return std::nullopt;

    Buffer digest = serviceData.suffixFrom(1).md5();
    Buffer uuid = encryptedUuid.aesCbc128Decrypt(digest, digest);

    return TuyaBLEAdvertisedDeviceInfo(bleAdvertisedDevice.getAddress(), isBound, protocolVersion, encryptionMethod, communicationCapacity, uuid.asString());
}