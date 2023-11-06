# TuyaBLE

An ESP32 library to connect to Tuya BLE devices based on Arduino-NimBLE.

## How to use?

The core of the library is `TuyaBLEDevice`. This class contains the logic to connect to a Tuya device over BLE and do a key-exchange and get and set DataPoints.

## What are DataPoints?

Tuya smart devices communicate thru so-called DataPoints. A DataPoint has a numerical index, a type (boolean/string/etc) and a value. Clients can send DataPoints to
the device to change settings or perform functionality. In turn, the device can report state and settings by sending DataPoints to the client.

For example, sending DataPoint 6 to a simple smart lock will unlock the lock. In turn, the lock will report its unlock status by sending DataPoint 47 as a boolean back 
to the client.

## How to Use?

You can create a `TuyaBLEDevice` or a subclass, like `TuyaBLESimpleLock` using one of the two dedicated constructors:

- `TuyaBLEDevice(TuyaBLEAdvertisedDeviceInfo, TuyaDeviceCredentials)` if you are scanning devices
- `TuyaBLEDevice(NimBLEAddress, TuyaDeviceCredentials)` if you know the address of the device

In both cases, you need to provide credentials.

### TuyaDeviceCredentials

When Tuya devices are registered in one of their apps, they get a `deviceId`, `localKey` and `uuid`. Extracting the `localKey` and `deviceId` can be done thru the Tuya Cloud API, search the internet for more information. Once you have these pieces of information, you need to pass them to the `TuyaBLEDevice` on creation, since those values are used to encrypt the communication.


### Scanning

You can discover existing by Tuya BLE devices by actively scanning for BLE advertisements. Use `TuyaBLEAdvertisedDeviceInfo::fromBLEAdvertisedDevice()` to check if a `NimBLEAdvertisedDevice` is a Tuya device and what its `uuid` is: this function will return null if the device is not a Tuya device and a `TuyaBLEAdvertisedDeviceInfo` class if it is.


Example of scanning devices:
```
NimBLEScan *pBLEScan;
std::shared_ptr<TuyaBLEDevice> _tuyaBleDevice;
class MyAdvertisedDeviceCallbacks : public NimBLEScanCallbacks {
    void onResult(NimBLEAdvertisedDevice *advertisedDevice) {
      auto info = TuyaBLEAdvertisedDeviceInfo::fromBLEAdvertisedDevice(*advertisedDevice);
      if(info) {
        Serial.println(advertisedDevice->getAddress().toString().c_str());
        Serial.println(info->uuid());

        _tuyaBleDevice = std::make_shared<TuyaBLEDevice>(info, credentials);
        // you cannot connect here, because you are still scanning:
        // stop scanning and connect to the device in the next loop()
      }
    }
};

void startScan() {
  NimBLEScan* scan = NimBLEDevice::getScan();
  pBLEScan = scan;
  scan->setActiveScan(true);
  scan->setScanCallbacks(new MyAdvertisedDeviceCallbacks());
  scan->start(0, true);
}
*/
```

### Connecting and Callbacks

To communicate with the tuya device, you first need to connect. This kicks of a key-exchange between the device and client to establish a session. When the device is ready for communication, the `isReady()` function returns `true`. Use `device.setOnReadyCallback()` to get notified when the device is ready. When the device is ready, you can ask for datapoint updates and send datapoints.

### DataPoints

Use `TuyaBLEDevice.requestDataPointsUpdate()` to ask the device for data points. DataPoints can be send back to the client in multiple batches. For each datapoint, the `onReceivedDataPointCallback` is called with the DataPoint that got updated. Use the `onUpdatedReportedDataPointsCallback` to get notified when a batch of DataPoint has been received.

You can read DataPoints received by the device using the `reportedDataPoints()`, `reportedDataPoint(uint8_t)` or one of the dedicated helpers, such as `reportedBooleanDataPoint(uint8_t, defaultValue)`. A DataPoint is of type `TuyaDataPoint`, which has methods for reading the id, type and value.

Sending datapoints is done using the `sendDataPoints()` method, this method takes a vector of `TuyaDataPoint`s and an optional callback that will be invoked when the device reports that it sucessfully received the datapoint. You can quickly create Datapoints using the factory methods, such as `TuyaDataPoint::boolean(9, true)`.


## Example

This example connects to a simple tuya BLE smart lock
```
#include <Arduino.h>
#include <NimBLEDevice.h>
#include "TuyaBLEDevice.h"
#include "TuyaBLESimpleLock.h"

auto credentials = TuyaDeviceCredentials("<uuid>", "<id>", "<localKey>");
auto mac = NimBLEAddress("aa:bb:cc:dd:ee:ff"); // mac address of the device

std::shared_ptr<TuyaBLESimpleLock> lock;

void setup() {
  Serial.begin(115200);
  while(!Serial);

  // initialize bluetooth
  NimBLEDevice::init("");

  // create our lock by address
  lock = std::make_shared<TuyaBLESimpleLock>(mac, credentials);

  // enable debug output
  lock->enableDebugLog();

  // register a callback for when the device is ready for communication
  lock->setOnReadyCallback([](TuyaBLEDevice* device) {
    // when ready, request datapoints from the device
    device->requestDataPointsUpdate();

    // also send an unlock command to the device for memberId=1
    Serial.println("READY, sending unlock!");
    lock->shortRangeUnlock(1, [](TuyaBLEDevice* device) {
      // when the device has received the unlock command, disconnect our BLE connection
      Serial.println("unlocked, disconnecting");
      device->disconnect();
    });
  });

  // print received datapoints
  lock->setOnReceivedDataPointCallback([](TuyaBLEDevice* device, const TuyaDataPoint& dp) {
    Serial.println("dp: " + dp.debugDescription());
  });

  // and connect once everything is set up
  lock->connect();
}

```