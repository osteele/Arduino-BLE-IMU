#include "BLE_Service_Handler.h"

static const char NF_UART_SERVICE_UUID[] =
    "6E400001-B5A3-F393-E0A9-E50E24DCCA9E";
static const char NF_UART_RX_CHAR_UUID[] =
    "6E400002-B5A3-F393-E0A9-E50E24DCCA9E";
static const char NF_UART_TX_CHAR_UUID[] =
    "6E400003-B5A3-F393-E0A9-E50E24DCCA9E";

// If true, send the timestamp every UART_TX_HEARTBEAT_DELAY ticks
static const bool SEND_UART_TX_HEARTBEAT = false;
static const int UART_TX_HEARTBEAT_DELAY = (1000 - 10) / 10;

class UARTRxCallbacks : public BLECharacteristicCallbacks {
 public:
  UARTRxCallbacks(BLECharacteristic *txChar) : txChar_(txChar) {}

 private:
  void onWrite(BLECharacteristic *ch) {
    std::string value = ch->getValue();
    if (value.length() > 0) {
      Serial.print("Rx: ");
      Serial.write((value + "\0").c_str());
      if (value[value.length() - 1] != '\n') {
        Serial.println();
      }
      if (value == "ping" || value == "ping\n") {
        Serial.write("Tx: pong\n");
        static uint8_t data[] = "pong\n";
        txChar_->setValue(data, sizeof data - 1);
        txChar_->notify();
      }
    }
  }
  BLECharacteristic *txChar_;
};

class BLE_UARTServiceHandler : public BLEServiceHandler {
 public:
  BLE_UARTServiceHandler(BLEServer *bleServer)
      : BLEServiceHandler(bleServer, NF_UART_SERVICE_UUID) {
    txChar_ = bleService_->createCharacteristic(
        NF_UART_TX_CHAR_UUID, BLECharacteristic::PROPERTY_NOTIFY);
    txChar_->addDescriptor(new BLE2902());

    rxChar_ = bleService_->createCharacteristic(
        NF_UART_RX_CHAR_UUID, BLECharacteristic::PROPERTY_WRITE);
    rxChar_->setCallbacks(new UARTRxCallbacks(txChar_));
  }

  void tick() {
    unsigned long now = millis();
    if (SEND_UART_TX_HEARTBEAT && now > nextTxTimeMs_) {
      static char buffer[10];
      int len = snprintf(buffer, sizeof buffer, "%ld\n", now);
      if (0 <= len && len <= sizeof buffer) {
        txChar_->setValue((uint8_t *)buffer, len);
        txChar_->notify();
      } else {
        Serial.println("Tx buffer overflow");
      }
      nextTxTimeMs_ = now + UART_TX_HEARTBEAT_DELAY;
    }
  }

 private:
  BLECharacteristic *txChar_;
  BLECharacteristic *rxChar_;
  unsigned long nextTxTimeMs_ = 0;
};
