/*
    Video: https://www.youtube.com/watch?v=oCMOYS71NIU
    Based on Neil Kolban example for IDF: https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLE%20Tests/SampleNotify.cpp
    Ported to Arduino ESP32 by Evandro Copercini
    updated by chegewara

   Create a BLE server that, once we receive a connection, will send periodic notifications.
   The service advertises itself as: 4fafc201-1fb5-459e-8fcc-c5c9c331914b
   And has a characteristic of: beb5483e-36e1-4688-b7f5-ea07361b26a8

   The design of creating the BLE server is:
   1. Create a BLE Server
   2. Create a BLE Service
   3. Create a BLE Characteristic on the Service
   4. Create a BLE Descriptor on the characteristic
   5. Start the service.
   6. Start advertising.

   A connect hander associated with the server starts a background task that performs notification
   every couple of seconds.
*/
/*
 * This is the same as the prvious BLE_power code but includes input from a sensor functionality.
 */
// import libraries
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// declare server and characteristics
BLEServer* pServer = NULL;
BLECharacteristic* pPowMeasCharacteristic = NULL;
BLECharacteristic* pPowFeatCharacteristic = NULL;
BLECharacteristic* pSensPosCharacteristic = NULL;
BLEDescriptor* pPowMeas1Descriptor = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;

// values
int16_t value = 1;
byte sensorlocation[1] = {5};
int16_t powMeasFlags = 0;
int16_t powMeas[2] = {powMeasFlags, value};
uint32_t powFeat[1] = {0000000000000}; // 4 bytes

// UUIDs
#define SERVICE_UUID       BLEUUID((uint16_t)0x1818)
#define CHAR_POWMEAS_UUID  BLEUUID((uint16_t)0x2A63)
#define DESC_POWMEAS1_UUID BLEUUID((uint16_t)0x2902)
#define DESC_POWMEAS2_UUID BLEUUID((uint16_t)0x2903)
#define CHAR_POWFEAT_UUID  BLEUUID((uint16_t)0x2A65)
#define CHAR_SENSPOS_UUID  BLEUUID((uint16_t)0x2A5D)

// sensor values
const int pin = A0;  // Using pin 0 on ESP32 board
float voltage = 3.26; // as measured with multimeter from 3V out pin
float temp = 0; // initialize temperature

// code begins here 
class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
};

void InitBLE() {
    // Create the BLE Device
  BLEDevice::init("Fake power data lol");

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service for Cycling Power
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create a BLE Characteristic for Power Measurement
  pPowMeasCharacteristic = pService->createCharacteristic(
                      CHAR_POWMEAS_UUID,
                      BLECharacteristic::PROPERTY_NOTIFY
                    );
  // Create BLE Descriptors for Power Measurement TODO
  //            pPowMeas1Descriptor = new BLEDescriptor(DESC_POWMEAS1_UUID);
  //            BLEDescriptor *pPowMeas2Descriptor = new BLEDescriptor(DESC_POWMEAS2_UUID);

  //            attach descriptors to the Power Measurement characteristic TODO
  pPowMeasCharacteristic->addDescriptor(new BLE2902());
  //            pPowMeasCharacteristic->addDescriptor(pPowMeas2Descriptor);

  //            set values for descriptors TODO
  //            pPowMeas1Descriptor->setValue(presentationFormat1, sizeof presentationFormat1);
  //            pPowMeas2Descriptor->setValue(presentationFormat2, sizeof presentationFormat2);
  
  // Create a BLE Characteristic for Power Feature
  pPowFeatCharacteristic = pService->createCharacteristic(
                      CHAR_POWFEAT_UUID,
                      BLECharacteristic::PROPERTY_READ
                    );
  // Create a BLE Characteristic for Sensor Position
  pSensPosCharacteristic = pService->createCharacteristic(
                      CHAR_SENSPOS_UUID,
                      BLECharacteristic::PROPERTY_READ
                    );

  // Start the service
  pService->start();

  // Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x0);  // set value to 0x00 to not advertise this parameter
  BLEDevice::startAdvertising();
}



void setup() {
  Serial.begin(115200);
  InitBLE();
  Serial.println("Waiting a client connection to notify...");
}

void loop() {
    // notify changed value
    if (deviceConnected) { // TODO add in values and characteristics
        Serial.println("Connected");
        pPowMeasCharacteristic->setValue((uint8_t*)&powMeas, sizeof powMeas); // was (uint8_t ... , 4);
        pPowMeasCharacteristic->notify();

        pPowFeatCharacteristic->setValue((uint8_t*)&powFeat, sizeof powFeat); // was (uint8_t ... , 4);

        pSensPosCharacteristic->setValue((uint8_t*)&sensorlocation, sizeof sensorlocation); // was (uint8_t ... , 4);

        // reads the sensor input to obtain current power value
        powMeas[1] = 42*readSensor();
        
        Serial.println(powMeas[1]);
        delay(500); // bluetooth stack will go into congestion, if too many packets are sent, in 6 hours test i was able to go as low as 3ms
    }
    // disconnecting
    if (!deviceConnected && oldDeviceConnected) {
        Serial.println("Disonnected");
        delay(500); // give the bluetooth stack the chance to get things ready
        pServer->startAdvertising(); // restart advertising
        Serial.println("start advertising");
        oldDeviceConnected = deviceConnected;
    }
    // connecting
    if (deviceConnected && !oldDeviceConnected) {
        // do stuff here on connecting
        oldDeviceConnected = deviceConnected;
    }
}

float readSensor()
{
  /*
   * Temperature sensor reading for now. Will add strain gauge functionality later. 
   * But it will be simple since I'll just need to adjust the formula.
   */
  // vout = tc*ta + v0
  // (vout-v0)/tc = ta
  // based on data sheet
  // https://www.sparkfun.com/datasheets/DevTools/LilyPad/MCP9700.pdf

  // read sensor pin
  float temp = analogRead(pin);

  // adjust for 12-bit ADC
  temp = temp*voltage/4095;

  // return adjusted temp reading
  return (temp-.500)*100;
}
