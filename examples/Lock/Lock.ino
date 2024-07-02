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

void loop() {
  // Nothing to do here
}