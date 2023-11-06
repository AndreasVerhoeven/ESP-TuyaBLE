#ifndef TUYA_BLE_CONSTANTS_123
#define TUYA_BLE_CONSTANTS_123

/// function codes of messages send between device and client
///  - senderXXX usually means that the connected client sends this message to the device
///	 - receiveXXX usually means that the device sends this message to the client
enum class TuyaBLEFunctionCode: uint16_t {

	/// requests device info and the "srand" for establishing a sessionKey
	senderDeviceInfo = 0x0000,

	/// establishes a secure connection by sending uuid, id and localKey to the device
	senderPair = 0x0001,

	/// sends datapoints to the device
	senderDps = 0x0002,

	/// requests datapoints to be send back using the `receiveDp` message
	/// note multiple `receiveDp` messages might be received and their
	/// responseSequenceNumber will be 0
	senderDeviceStatus = 0x003,
		
	/// removes this device from the app
	senderUnbind = 0x0005,

	/// completely resets this device
	senderDeviceReset = 0x0006,
		
	/// over-the-air update messages
	senderOtaStart = 0x000C,
	senderOtaFile = 0x000D,
	senderOtaOffset = 0x000E,
	senderOtaUpgrade = 0x000F,
	senderOtaOver = 0x0010,
		
	/// sends datapoints to the device for protocol v4
	senderDpsV4 = 0x0027,
		
	/// device sends datapoints to the connected client,
	/// in response of a `senderDeviceStatus` message
	/// or when a datapoint changes
	receiveDp = 0x8001,

	receiveTimeDp = 0x8003,
	receiveSignDp = 0x8004,
	receiveSignTimeDp = 0x8005,
		
	/// device sends datapoints to the connected client, for protocol v4,
	/// in response of a `senderDeviceStatus` message
	/// or when a datapoint changes
	receiveDpV4 = 0x8006,
    receiveTimeDpV4 = 0x8007,
		
	receiveTime1Req = 0x8011,
	receiveTime2Req = 0x8012,
};

/// security flag defines which key is being used for encrypting messages
enum class TuyaBLESecurityFlag: uint8_t {
    /// used when doing a key-exchange with senderDeviceInfo
	/// key = md5(localKeyFromCloudApi.prefixUpTo(6))
    localKey = 0x04,

	/// used when a key-exchange has been done and we have a session key
	/// key = md5(srand + localKeyFromCloudApi.prefixUpTo(6))
    sessionKey = 0x05,
};

#endif//TUYA_BLE_CONSTANTS_123