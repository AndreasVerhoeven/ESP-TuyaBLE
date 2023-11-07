#include "TuyaBLEDevice.h"

#include <NimBLEDevice.h>
#include <NimBLEClient.h>
#include <NimBLEService.h>
#include <NimBLERemoteCharacteristic.h>

#include "Buffer.h"
#include "CryptoHelper.h"

/// A parsed response packet
struct TuyaBLEResponseParsedPacket {
  uint32_t packetNumber;
  uint32_t messageLength;
  uint8_t protocolVersion;
  Buffer data;

 static TuyaBLEResponseParsedPacket fromData(const Buffer& data) {
  if(data.size() < 1) return TuyaBLEResponseParsedPacket();

  TuyaBLEResponseParsedPacket parsedPacket;

  size_t offset = 0;
  parsedPacket.packetNumber = data.readPackedInt(offset);
  parsedPacket.messageLength = 0;
  parsedPacket.protocolVersion = 0;
  if(parsedPacket.packetNumber == 0) {
    parsedPacket.messageLength = data.readPackedInt(offset);
    parsedPacket.protocolVersion = data.readUint8(offset) >> 4;
  }

  parsedPacket.data = data.suffixFrom(offset);
  return parsedPacket;
 }
};

/// A parsed received message
class TuyaBLEReceivedMessage {
public:
  TuyaBLEFunctionCode functionCode;
  uint32_t sequenceNumber = 0;
  uint32_t responseToSequenceNumber = 0;
  Buffer data;

  String debugDescription() const {
    String output;
    output += "code = " + String(static_cast<uint16_t>(functionCode), HEX);
    output += ", seq = " + String(sequenceNumber);
    output += ", rseq = " + String(responseToSequenceNumber);
    output += ", data = " + data.debugDescription();
    return output;
  }
};

const String TuyaBLEDevice::emptyString = String();

void TuyaBLEDevice::setCredentials(const TuyaDeviceCredentials& credentials) {
  this->_credentials = credentials;
  _localKeyMD5 = Buffer();
}

void TuyaBLEDevice::clearExpectedResponse() {
  _expectedResponsePacketNumber = 0;
	_expectedResponseDataLength = 0;
	_receivedData = Buffer();
}

void TuyaBLEDevice::onNotify(NimBLERemoteCharacteristic* characteristic, uint8_t* data, size_t length, bool isNotify) {
  auto packet = TuyaBLEResponseParsedPacket::fromData(Buffer(data, length));
  if(packet.packetNumber != _expectedResponsePacketNumber) return;

  //Serial.printf("Received packet %ld\n", packet->packetNumber);
		
	if(packet.packetNumber == 0) {
		_receivedData = Buffer();
		_expectedResponseDataLength = packet.messageLength;
	}

  _receivedData.append(packet.data);
  if(_receivedData.size() < _expectedResponseDataLength) {
    _expectedResponsePacketNumber += 1;
  } else if(_receivedData.size() == _expectedResponseDataLength) {
    auto completeData = _receivedData;
    clearExpectedResponse();
    handleReceivedMessageData(completeData);
  } else {
    // dunno what to do here
  }
}

void TuyaBLEDevice::handleReceivedMessageData(const Buffer& data) {
		// which gets encrypted as data:
		//
		// .|0123456789ABCDEF|0123456789ABCDEF
		// F|IIIIIIIIIIIIIIII|E..............E
		//
		// F = security flag (04 = use md5(local_key[0..<6]), 05 = use session key, 06 = no key)
		// I...I = random IV vector (16 bytes)
		// E...E = aes(key, iv, data)

    if(data.size() < 16 + 16 + 1) return;
		
		size_t offset = 0;
    TuyaBLESecurityFlag securityFlag = static_cast<TuyaBLESecurityFlag>(data.readUint8(offset));
    Buffer iv = data.readBuffer(offset, 16);
    Buffer encryptedMessageData = data.suffixFrom(offset);
    Buffer key = keyToUseForFlag(securityFlag);
    Buffer decryptedMessageData = encryptedMessageData.aesCbc128Decrypt(key, iv);
		parseAndHandleReceivedMessage(decryptedMessageData);
}

void TuyaBLEDevice::parseAndHandleReceivedMessage(const Buffer& data) {
  // format:
  // 0123|45678|9A|BC|D....|..
  // SSSS|RRRR||CC|LL|D...D|XX
  //
  // SSSS = message serial number
  // RRRR = serial number this message is responding to, 0 if new message
  // CC = one of TuyaFunctionCode
  // LL = length of data
  // D...D = data of this message, `LL` number of bytes
  // XX = crc16(SSSS|AAAA|CC|LL|D...D) (seed = 0xFFFF)
  // PP = padding until a multiple of 16 bytes

  TuyaBLEReceivedMessage receivedMessage;

  size_t offset = 0;
  receivedMessage.sequenceNumber = data.readBigEndianUint32(offset);
  receivedMessage.responseToSequenceNumber = data.readBigEndianUint32(offset);
  receivedMessage.functionCode = static_cast<TuyaBLEFunctionCode>(data.readBigEndianUint16(offset));
  uint16_t dataLength = data.readBigEndianUint16(offset);
  receivedMessage.data = data.readBuffer(offset, static_cast<size_t>(dataLength));
  uint16_t crc = data.readBigEndianUint16(offset);
  
  handleReceivedFunction(receivedMessage);
}

void TuyaBLEDevice::handleReceivedFunction(const TuyaBLEReceivedMessage& message) {
  if(_isDebugLogEnabled)
    debugLog("[Received] message: " + message.debugDescription());

  if(message.functionCode == TuyaBLEFunctionCode::senderDeviceInfo) {
    handleReceivedResponseSenderDeviceInfo(message);
  } else if(message.functionCode == TuyaBLEFunctionCode::senderPair) {
    handleReceivedResponseSenderPair(message);
  } else if(message.functionCode == TuyaBLEFunctionCode::senderDps) {
    handleReceivedResponseSenderDps(message);
  } else if(message.functionCode == TuyaBLEFunctionCode::receiveTime1Req) {
    handleReceivedRequestReceiveTime1Req(message);
  } else if(message.functionCode == TuyaBLEFunctionCode::receiveDp) {
    handleReceivedReceiveDP(message);
  }
}

void TuyaBLEDevice::handleReceivedResponseSenderDps(const TuyaBLEReceivedMessage& message) {
  if(auto callback = _waitingOnResponseSendDpsCallbacks[message.responseToSequenceNumber]) {
    _waitingOnResponseSendDpsCallbacks.erase(message.responseToSequenceNumber);
    callback(this);
  }
}

void TuyaBLEDevice::handleReceivedRequestReceiveTime1Req(const TuyaBLEReceivedMessage& message) {
  // we discard those, because our time might not be great
}

void TuyaBLEDevice::handleReceivedResponseSenderDeviceInfo(const TuyaBLEReceivedMessage& message) {
  const Buffer& data = message.data;
  if(data.size() < 46) return;

  _infoDeviceVersion = String(data[0]) + "." + String(data[1]);
  _infoProtocolVersion = String(data[2]) + "." + String(data[3]);
  _infoHardwareVersion = String(data[12]) + "." + String(data[13]);

  Buffer srand = data.subRangeWithStartAndLength(6, 6);
  Buffer authKey = data.subRangeWithStartAndLength(14, 32);
  _sessionKey = (_localKeyFirstSixBytes + srand).md5();

    if(_isDebugLogEnabled) {
    debugLog("[Received] senderDeviceInfo response: key handshake complete");
    debugLog("[Info] device version: " + _infoDeviceVersion);
    debugLog("[Info] protocol version: " + _infoProtocolVersion);
    debugLog("[Info] hardware version: " + _infoHardwareVersion);
    debugLog("[Info] session srand nonce: " + srand.debugDescription());
    debugLog("[Info] session key: " + _sessionKey.debugDescription());
  }

  sendPairingRequest();
}

void TuyaBLEDevice::handleReceivedResponseSenderPair(const TuyaBLEReceivedMessage& message) {
  if(message.data.size() < 1) return;
  bool success = message.data[0] != 0;

  if(_isDebugLogEnabled) {
    debugLog("[Response] Sender pair: " + String(success ? "success" : "failed"));
  }

  _isReady = true;

  if(_onReadyCallback)
    _onReadyCallback(this);
}

void TuyaBLEDevice::handleReceivedReceiveDP(const TuyaBLEReceivedMessage& message) {
  size_t offset = 0;
  const Buffer& data = message.data;

  std::vector<TuyaDataPoint> receivedDataPoints;

  while(data.size() - offset >= 4) {

    uint8_t dp = data.readUint8(offset);
    TuyaDataPointType type = static_cast<TuyaDataPointType>(data.readUint8(offset));

    TuyaDataPoint dataPoint(dp, type);
    uint8_t dataLength = data.readUint8(offset);
    Buffer itemData = data.subRangeWithStartAndLength(offset, static_cast<size_t>(dataLength));

    switch(type) {
      case TuyaDataPointType::raw:
       dataPoint.setRaw(itemData); 
      break;

      case TuyaDataPointType::boolean:
        dataPoint.setBoolean(itemData.asBigEndianUnsignedInt() != 0);
      break;

      case TuyaDataPointType::value:
        dataPoint.setValue(itemData.asBigEndianSignedInt());
      break;

      case TuyaDataPointType::string:
        dataPoint.setString(itemData.asString());
      break;

      case TuyaDataPointType::enumeration:
        dataPoint.setEnumeration(itemData.asBigEndianUnsignedInt());
      break;

      case TuyaDataPointType::bitmap:
        dataPoint.setBitmap(itemData);
      break;
    }

    offset += dataLength;

    if(_isDebugLogEnabled) {
      debugLog("[Received] Datapoint: " + dataPoint.debugDescription());
    }

    receivedDataPoints.push_back(dataPoint);
    _reportedDataPoints.insert(std::pair<uint16_t, TuyaDataPoint>(dataPoint.dp(), dataPoint));

    if(_onReceivedDataPointCallback)
      _onReceivedDataPointCallback(this, dataPoint);
  }

  if(_onUpdatedReportedDataPointsCallback)
    _onUpdatedReportedDataPointsCallback(this);
}

void TuyaBLEDevice::sendDataPoints(const std::vector<TuyaDataPoint>& dps, std::function<void(TuyaBLEDevice*)> callback) {
  Buffer data;
  for(auto&& dataPoint : dps) {
    data.append(dataPoint.dp());
    data.append(static_cast<uint8_t>(dataPoint.type()));

    size_t numberOfLengthBytes = (_deviceInfo.protocolVersion() >= 4 ? 2 : 1);
    switch(dataPoint.type()) {
      case TuyaDataPointType::raw:
        data.appendBigEndianWithNumberOfBytes(dataPoint.raw().size(), numberOfLengthBytes);
        data.append(dataPoint.raw());
      break;

      case TuyaDataPointType::boolean:
        data.appendBigEndianWithNumberOfBytes(1, numberOfLengthBytes);
        data.append(static_cast<uint8_t>(dataPoint.boolean() ? 1 : 0));
      break;

      case TuyaDataPointType::value:
        data.appendBigEndianWithNumberOfBytes(4, numberOfLengthBytes);
        data.append(dataPoint.value());
      break;

      case TuyaDataPointType::string:
        data.appendBigEndianWithNumberOfBytes(dataPoint.string().length(), numberOfLengthBytes);
        data.append(dataPoint.string());
      break;

      case TuyaDataPointType::enumeration:
        data.appendBigEndianWithNumberOfBytes(4, numberOfLengthBytes);
        data.append(dataPoint.value());
      break;

      case TuyaDataPointType::bitmap:
        data.appendBigEndianWithNumberOfBytes(dataPoint.bitmap().size(), numberOfLengthBytes);
        data.append(dataPoint.bitmap());
      break;
    }
  }

  _waitingOnResponseSendDpsCallbacks[_messageSequenceNumber + 1] = callback;
  sendMessage(TuyaBLEFunctionCode::senderDps, data, 0, true);
}

void TuyaBLEDevice::sendDataPoint(const TuyaDataPoint& dp, std::function<void(TuyaBLEDevice*)> callback) {
  sendDataPoints(std::vector<TuyaDataPoint>{dp}, callback);
}

#define TUYA_REGULAR_LOCK
void TuyaBLEDevice::sendUnlock() {
  #ifdef TUYA_REGULAR_LOCK
  Buffer data;
  data.append(0x06); // dp id
  data.append(static_cast<uint8_t>(TuyaDataPointType::raw)); // type
  data.append(2); // length
  data.append(0); // unlock
  data.append(1); // member id

  #else
  Buffer data;
  data.append(0x47);
  data.append(static_cast<uint8_t>(TuyaDataPointType::raw)); // type
  data.append(19); // length
  data.appendBigEndian(uint16_t(0xFFFF)); // central id
  data.appendBigEndian(uint16_t(1)); // peripheral id
  data.append(String("16183634")); // central random number
  data.append(0); // lock = 0, unlock = 1
  data.appendBigEndian(uint32_t(0x653c0c53)); // timestamp
  data.appendBigEndian(uint16_t(1)); // member id
  #endif

  sendMessage(TuyaBLEFunctionCode::senderDps, data, 0, true);
}

void TuyaBLEDevice::sendPairingRequest() {
  Buffer data;
  data.append(uuid());
  data.append(_localKeyFirstSixBytes);
  data.append(_credentials.deviceId());
  data.append(Buffer(max(size_t(0), 44 - data.size())));
  sendMessage(TuyaBLEFunctionCode::senderPair, data, 0, true);
}

bool TuyaBLEDevice::connect() {
  if(_client != nullptr && _client->isConnected()) {
    debugLog("[Device] already connected");
    return false;
  }

  _isReady = false;
  _client = NimBLEDevice::createClient();

  if(!_client->connect(_deviceInfo.address(), false)) {
    debugLog("[Device] could not connect");
    return false;
  }

  debugLog("[Device] ble connected, checking service and characteristic");

  _service = _client->getService(NimBLEUUID(uint16_t(0x1910)));
  if(_service == nullptr) {
    debugLog("[Device] service 1910 not found");
    NimBLEDevice::deleteClient(_client);
    _client = nullptr;
    return false;
  }

  _readCharacteristic = _service->getCharacteristic(NimBLEUUID(uint16_t(0x2B10)));
  _writeCharacteristic = _service->getCharacteristic(NimBLEUUID(uint16_t(0x2B11)));

  if(_readCharacteristic == nullptr || _writeCharacteristic == nullptr) {
    disconnect();
    return false;
  }
  if(_readCharacteristic->canNotify()) {
    notify_callback callback = std::bind(&TuyaBLEDevice::onNotify, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);
    _readCharacteristic->subscribe(true, callback);
  }

  debugLog("[Device] fully connected");
  sendDeviceInfoRequest();

  if(_onConnectedCallback)
    _onConnectedCallback(this);

  return true;
}

bool TuyaBLEDevice::disconnect() {
  if(_client == nullptr || !_client->isConnected()) 
    return false;

  onDisconnect();
  return true;
}

void TuyaBLEDevice::onDisconnect() {
  auto client = _client;

  if(_readCharacteristic)
    _readCharacteristic->unsubscribe(false);
    
  _isReady = false;
  _client = nullptr;
  _service = nullptr;
  _readCharacteristic = nullptr;
  _writeCharacteristic = nullptr;
  _waitingOnResponseSendDpsCallbacks.clear();

  client->disconnect();

  // this crashes :(
  //NimBLEDevice::deleteClient(client);

  if(_onDisconnectedCallback)
    _onDisconnectedCallback(this);
}

void TuyaBLEDevice::requestDataPointsUpdate() {
  sendMessage(TuyaBLEFunctionCode::senderDeviceStatus, Buffer(), 0, true);
}

Buffer TuyaBLEDevice::ensureLocalKeyMD5() {
  if(_localKeyMD5.size() == 0) {
    _localKeyFirstSixBytes = Buffer(_credentials.localKey().substring(0, 6).c_str(), 6);
    _localKeyMD5 = _localKeyFirstSixBytes.md5();
  } 

  return _localKeyMD5;
}

Buffer TuyaBLEDevice::keyToUseForFlag(TuyaBLESecurityFlag flag) {
  switch(flag) {
      case TuyaBLESecurityFlag::localKey: return ensureLocalKeyMD5();
      case TuyaBLESecurityFlag::sessionKey: return _sessionKey;
      default: return Buffer();
  }
}

Buffer TuyaBLEDevice::createMessage(TuyaBLEFunctionCode code, const Buffer& data, uint32_t sequenceNumber, uint32_t responseTo) {
    Buffer messageData;
    messageData.appendBigEndian(sequenceNumber);
    messageData.appendBigEndian(responseTo);
    messageData.appendBigEndian(static_cast<uint16_t>(code));
    messageData.appendBigEndian(static_cast<uint16_t>(data.size()));
    messageData.append(data);
    messageData.appendBigEndian(messageData.crc16());

    if(_isDebugLogEnabled) {
      debugLog("[Sending] unpadded message(seq = " + String(sequenceNumber)
        + ",  rseq = "  + String(responseTo) +
        + ", code = " + static_cast<uint16_t>(code)
        +  "): " 
        + messageData.debugDescription()
      );
    }

    messageData.padToNumberOfBytes(16);

    TuyaBLESecurityFlag securityFlag = code == TuyaBLEFunctionCode::senderDeviceInfo ? TuyaBLESecurityFlag::localKey : TuyaBLESecurityFlag::sessionKey;
    Buffer iv = Buffer::aesInitializationVector();
    auto key = keyToUseForFlag(securityFlag);

    Buffer encryptedMessage;
    encryptedMessage.append(static_cast<uint8_t>(securityFlag));
    encryptedMessage.append(iv);
    encryptedMessage.append(messageData.aesCbc128Encrypt(key, iv));

    return encryptedMessage;
 }

 void TuyaBLEDevice::sendMessage(TuyaBLEFunctionCode code, const Buffer& data, uint32_t responseTo, bool expectsResponse) {
  _messageSequenceNumber++;
  Buffer message = createMessage(code, data, _messageSequenceNumber, responseTo);

  std::vector<Buffer> packets = splitIntoPackets(message);
  for(auto&& packet : packets) {
    if(_writeCharacteristic->writeValue(packet.data(), packet.size(), false) == false) {
      debugLog("[Error] could not send packet");
    }
  }

  if(expectsResponse == true) {
    clearExpectedResponse();
    _waitingOnResponseSequenceNumber = _messageSequenceNumber;
  }
 }

 std::vector<Buffer> TuyaBLEDevice::splitIntoPackets(const Buffer& data) {
    std::vector<Buffer> packets;
    // we can only send 20 bytes per package, so we need to split our message into packets
    
    // format:
    // N|L|V|D...D
    // N+1|D...D
    // N+2|D...D
    //
    // N = packet number, a packed int
    // L = length of the message data we're splitting into packages (only set in the first package), a packed int
    // V = protocol version << 4 (currently 3 << 4)
    // D = data, the length must be so that the total package size <= 20 bytes


    size_t position = 0;
    size_t length = data.size();
    unsigned int packetNumber = 0;

    const size_t maximumGattMtuLength = 20;

    while(position < length) {
        Buffer packet;
        packet.appendPackedInt(packetNumber);
        
        if(packetNumber == 0) {
            packet.appendPackedInt(length);
            packet.append(_deviceInfo.protocolVersion() << 4);
        }

        size_t dataLength = std::min(maximumGattMtuLength - packet.size(), length - position);
        packet.append(data, position, dataLength);
        packets.push_back(packet);

        packetNumber++;
        position += dataLength;
    }

    return packets;
 }

void TuyaBLEDevice::sendDeviceInfoRequest() {
		sendMessage(TuyaBLEFunctionCode::senderDeviceInfo, Buffer(), 0, true);

}

void TuyaBLEDevice::debugLog(const String& message) {
  if(!isDebugLogEnabled()) return;

  if(_onDebugLogCallback)
    _onDebugLogCallback(this, message);
  else
    Serial.println(message);
}

void TuyaBLEDevice::enableDebugLog() {
  _isDebugLogEnabled = true;
  _onDebugLogCallback = nullptr;
}
  
void TuyaBLEDevice::enableDebugLog(std::function<void(TuyaBLEDevice*, const String&)> callback) {
  _isDebugLogEnabled = true;
  _onDebugLogCallback = callback;
}

void TuyaBLEDevice::disableDebugLog() {
  _isDebugLogEnabled = false;
}