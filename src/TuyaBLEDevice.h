#ifndef TUYA_DEVICE_123
#define TUYA_DEVICE_123

#include <Arduino.h>
#include <NimBLEAdvertisedDevice.h>

#include "TuyaDeviceCredentials.h"
#include "TuyaBLEConstants.h"
#include "TuyaDataPoint.h"
#include "TuyaBLEAdvertisedDeviceInfo.h"
#include "Buffer.h"

#include <vector>
#include <memory>

class NimBLEClient;
class NimBLERemoteService;
class NimBLERemoteCharacteristic;
class TuyaBLEReceivedMessage;
class TuyaBLEAdvertisedDeviceInfo;

class TuyaBLEDevice {
private:
    /// the address we need to connect to
    NimBLEAddress _address; 

    /// when connected, these are valid
    NimBLEClient* _client = nullptr;
    NimBLERemoteService* _service = nullptr;
    NimBLERemoteCharacteristic* _readCharacteristic = nullptr;
    NimBLERemoteCharacteristic* _writeCharacteristic = nullptr;

    /// info and credentails about the device so we can connect
    TuyaBLEAdvertisedDeviceInfo _deviceInfo;
    TuyaDeviceCredentials _credentials;

    /// connection info
    bool _isConnected = false;
    bool _isReady = false;
    uint32_t _messageSequenceNumber = 0;
    uint32_t _waitingOnResponseSequenceNumber = 0;
    uint32_t _expectedResponseDataLength = 0;
    uint32_t _expectedResponsePacketNumber = 0;
    Buffer _receivedData;

    /// keys to use
    Buffer _localKeyFirstSixBytes;
    Buffer _localKeyMD5;
    Buffer _sessionKey;

    // device info
    String _infoDeviceVersion;
    String _infoProtocolVersion;
    String _infoHardwareVersion;

    // datapoints
    std::map<uint8_t, TuyaDataPoint> _reportedDataPoints;
    std::map<uint32_t, std::function<void(TuyaBLEDevice*)>> _waitingOnResponseSendDpsCallbacks;

    // debug logging
    bool _isDebugLogEnabled = false;

    Buffer createMessage(TuyaBLEFunctionCode code, const Buffer& data, uint32_t sequenceNumber, uint32_t responseTo);
    std::vector<Buffer> splitIntoPackets(const Buffer& data);
    Buffer keyToUseForFlag(TuyaBLESecurityFlag flag);
    Buffer ensureLocalKeyMD5();

    // creating a session
    void sendDeviceInfoRequest();
    void sendPairingRequest();

    // handling received data
    void onNotify(NimBLERemoteCharacteristic* characteristic, uint8_t* data, size_t length, bool isNotify);
    void handleReceivedMessageData(const Buffer& data);
    void parseAndHandleReceivedMessage(const Buffer& data);
    void handleReceivedFunction(const TuyaBLEReceivedMessage& message);
    void clearExpectedResponse();
    
    // handle decoded responses
    void handleReceivedResponseSenderDeviceInfo(const TuyaBLEReceivedMessage& message);
    void handleReceivedResponseSenderPair(const TuyaBLEReceivedMessage& message);
    void handleReceivedResponseSenderDps(const TuyaBLEReceivedMessage& message);
    void handleReceivedRequestReceiveTime1Req(const TuyaBLEReceivedMessage& message);
    void handleReceivedReceiveDP(const TuyaBLEReceivedMessage& message);

    // callbacks
    std::function<void(TuyaBLEDevice*)> _onConnectedCallback;
    std::function<void(TuyaBLEDevice*)> _onDisconnectedCallback;
    std::function<void(TuyaBLEDevice*)> _onReadyCallback;
    std::function<void(TuyaBLEDevice*, const TuyaDataPoint&)> _onReceivedDataPointCallback;
    std::function<void(TuyaBLEDevice*)> _onUpdatedReportedDataPointsCallback;
    std::function<void(TuyaBLEDevice*, const String&)> _onDebugLogCallback;

    // for use in default arguments
    static const String emptyString;

protected:
    // this sends a raw message to the device
    void sendMessage(TuyaBLEFunctionCode code, const Buffer& data, uint32_t responseTo, bool expectsResponse);

    // called when disconnecting
    virtual void onDisconnect();
public:
    TuyaBLEDevice(TuyaBLEAdvertisedDeviceInfo info,const TuyaDeviceCredentials& credentials ) : _deviceInfo(info), _credentials(credentials) {}
    TuyaBLEDevice(const NimBLEAddress& address, const TuyaDeviceCredentials& credentials, uint8_t protocolVersion = 3) : _credentials(credentials) {
        _deviceInfo._address = address;
        _deviceInfo._uuid = credentials.uuid();
        _deviceInfo._protocolVersion = protocolVersion;
    }
    // credentials
    void setCredentials(const TuyaDeviceCredentials& credentials);
    const TuyaDeviceCredentials& credentials() const { return this->_credentials; }
    
    // information
    const NimBLEAddress& address() const { return _deviceInfo.address(); }
    bool isBound() const { return _deviceInfo.isBound(); }
    uint8_t protocolVersion() const { return _deviceInfo.protocolVersion(); }
    uint8_t encryptionMethod() const { return _deviceInfo.encryptionMethod(); }
    uint16_t communicationCapacity() const { return _deviceInfo.communicationCapacity(); }
    String uuid() const { return _deviceInfo.uuid(); }

    // connect / disconnect
    virtual bool connect();
    virtual bool disconnect();
    bool isConnected() const { return _client != nullptr && _client->isConnected(); };
    bool isReady() const { return _isReady; }

    // checking received dps
    void requestDataPointsUpdate();
    bool hasDataPoint(uint8_t dp) const { return _reportedDataPoints.find(dp) != _reportedDataPoints.end(); }
    const TuyaDataPoint& reportedDataPoint(uint8_t dp) const { 
        auto iter = _reportedDataPoints.find(dp);
        return iter == _reportedDataPoints.end() ? TuyaDataPoint::invalid : iter->second;
    }
    const std::map<uint8_t, TuyaDataPoint> reportedDataPoints() const { return _reportedDataPoints; }

    const Buffer& reportedRawDataPoint(uint8_t dp, const Buffer& defaultValue = Buffer::empty) const { 
        auto value = reportedDataPoint(dp);
        return value.isValid() ? value.raw() : defaultValue;
    }

    bool reportedBooleanDataPoint(uint8_t dp, bool defaultValue = false) const { 
        auto value = reportedDataPoint(dp);
        return value.isValid() ? value.boolean() : defaultValue;
    }

    int32_t reportedValueDataPoint(uint8_t dp, int32_t defaultValue = 0) const { 
        auto value = reportedDataPoint(dp);
        return value.isValid()? value.value() : defaultValue;
    }

    const String& reportedStringDataPoint(uint8_t dp, const String& defaultValue = TuyaBLEDevice::emptyString) const { 
        auto value = reportedDataPoint(dp);
        return value.isValid() ? value.string() : defaultValue;
    }

    uint8_t reportedEnumerationDataPoint(uint8_t dp, uint8_t defaultValue = 0) const { 
        auto value = reportedDataPoint(dp);
        return value.isValid() ? value.enumeration() : defaultValue;
    }

    const Buffer& reportedBitmapDataPoint(uint8_t dp, const Buffer& defaultValue = Buffer::empty) const { 
        auto value = reportedDataPoint(dp);
        return value.isValid() ? value.bitmap() : defaultValue;
    }

    // sending dps
    void sendDataPoints(const std::vector<TuyaDataPoint>& dps, std::function<void(TuyaBLEDevice*)> callback = nullptr);
    void sendDataPoint(const TuyaDataPoint& dp, std::function<void(TuyaBLEDevice*)> callback = nullptr);

    // device callbacks
    void setOnConnectedCallback(std::function<void(TuyaBLEDevice*)> callback) { _onConnectedCallback = callback; }
    void setOnDisconnectedCallback(std::function<void(TuyaBLEDevice*)> callback) { _onDisconnectedCallback = callback; }
    void setOnReadyCallback(std::function<void(TuyaBLEDevice*)> callback) { _onReadyCallback = callback; }
    void setOnReceivedDataPointCallback(std::function<void(TuyaBLEDevice*, const TuyaDataPoint&)> callback) { _onReceivedDataPointCallback = callback; }
    void setOnUpdatedReportedDataPointsCallback(std::function<void(TuyaBLEDevice*)> callback) { _onUpdatedReportedDataPointsCallback = callback; }

    // debugging
    bool isDebugLogEnabled() const { return _isDebugLogEnabled; }
    void debugLog(const String& message);
    void enableDebugLog();
    void enableDebugLog(std::function<void(TuyaBLEDevice*, const String&)> callback);
    void disableDebugLog();
};

#endif//TUYA_DEVICE_123