#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <BLEBeacon.h>

int scanTime = 5;  // Tempo de escaneamento em segundos
BLEScan *pBLEScan;

// RSSI de referência a 1 metro
const int referenceRSSI = -65;
const float pathLossExponent = 2.0;  // Ajuste conforme o ambiente

class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    if (advertisedDevice.haveManufacturerData()) {
      String strManufacturerData = advertisedDevice.getManufacturerData();
      uint8_t cManufacturerData[100];
      int dataLength = strManufacturerData.length();

      if (dataLength > sizeof(cManufacturerData)) {
        dataLength = sizeof(cManufacturerData);
      }
      memcpy(cManufacturerData, strManufacturerData.c_str(), dataLength);

      if (dataLength >= 25) {  // Verifica o tamanho mínimo dos dados do fabricante
        BLEBeacon oBeacon = BLEBeacon();
        oBeacon.setData(strManufacturerData);

        uint16_t minor = swapEndian16(oBeacon.getMinor());

        if (minor == 30000) {
          String macAddress = advertisedDevice.getAddress().toString().c_str();
          String deviceName = advertisedDevice.haveName() ? advertisedDevice.getName().c_str() : "Desconhecido";
          int rssi = advertisedDevice.getRSSI();

          Serial.println("====================================");
          Serial.printf("Endereço MAC: %s\n", macAddress.c_str());
          Serial.printf("Nome do Dispositivo: %s\n", deviceName.c_str());
          Serial.printf(
            "ID: %04X \nMajor: %d (0x%04X) \nMinor: %d (0x%04X) \nUUID: %s \nPotência: %d\n",
            oBeacon.getManufacturerId(),
            swapEndian16(oBeacon.getMajor()), swapEndian16(oBeacon.getMajor()),
            minor, minor,
            oBeacon.getProximityUUID().toString().c_str(),
            oBeacon.getSignalPower()
          );

          float distance = calculateDistance(rssi) * 10;  // Distância em cm
          Serial.printf("Distância Estimada: %.2f cm\n", distance);

          if (advertisedDevice.haveServiceData()) {
            String serviceData = advertisedDevice.getServiceData();
            uint8_t* serviceDataBytes = (uint8_t*)serviceData.c_str();
            int serviceDataLength = serviceData.length();

            if (serviceDataLength > 0) {
              uint8_t batteryLevel = serviceDataBytes[0]; // Ajuste conforme necessário
              Serial.printf("Porcentagem da Bateria: %d%%\n", batteryLevel);
            } else {
              Serial.println("Dados de bateria não encontrados.");
            }
          } else {
            Serial.println("Serviço de bateria não encontrado.");
          }

          Serial.println("====================================");
        }
      }
    }
  }

  uint16_t swapEndian16(uint16_t value) {
    return (value >> 8) | (value << 8);
  }

  float calculateDistance(int rssi) {
    if (rssi == 0) {
      return -1.0;
    }
    return pow(10, ((referenceRSSI - rssi) / (10 * pathLossExponent)));
  }
};

void setup() {
  Serial.begin(115200);
  BLEDevice::init("");  // Inicialize o BLE com um nome, se necessário
  pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true);
}

void loop() {
  BLEScanResults* foundDevices = pBLEScan->start(scanTime, false);
  pBLEScan->clearResults();
  delay(100);  // Atraso entre os scans
}
