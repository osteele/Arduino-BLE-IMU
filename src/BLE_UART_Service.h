static const char NF_UART_SERVICE_UUID[] =
    "6E400001-B5A3-F393-E0A9-E50E24DCCA9E";
static const char NF_UART_RX_CHAR_UUID[] =
    "6E400002-B5A3-F393-E0A9-E50E24DCCA9E";
static const char NF_UART_TX_CHAR_UUID[] =
    "6E400003-B5A3-F393-E0A9-E50E24DCCA9E";

// If true, send the timestamp every UART_TX_HEARTBEAT_DELAY ticks
static const bool SEND_UART_TX_HEARTBEAT = false;
static const int UART_TX_HEARTBEAT_DELAY = 10 / 60;

static BLECharacteristic *txChar;
static BLECharacteristic *rxChar;

class UARTRxCallbacks : public BLECharacteristicCallbacks {
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
        txChar->setValue(data, sizeof data - 1);
        txChar->notify();
      }
    }
  }
};

BLEService *uartService = NULL;

inline void addUARTService(BLEServer *bleServer) {
  // UART service and characteristics
  uartService = bleServer->createService(NF_UART_SERVICE_UUID);

  txChar = uartService->createCharacteristic(
      NF_UART_TX_CHAR_UUID, BLECharacteristic::PROPERTY_NOTIFY);
  txChar->addDescriptor(new BLE2902());

  rxChar = uartService->createCharacteristic(NF_UART_RX_CHAR_UUID,
                                             BLECharacteristic::PROPERTY_WRITE);
  rxChar->setCallbacks(new UARTRxCallbacks());
}

inline void startUARTService() { uartService->start(); }

inline void tickUARTService() {
  static unsigned long nextTxTimeMs = 0;
  unsigned long now = millis();
  if (SEND_UART_TX_HEARTBEAT && now > nextTxTimeMs) {
    static char buffer[10];
    int len = snprintf(buffer, sizeof buffer, "%ld\n", now);
    if (0 <= len && len <= sizeof buffer) {
      txChar->setValue((uint8_t *)buffer, len);
      txChar->notify();
    } else {
      Serial.println("Tx buffer overflow");
    }
    nextTxTimeMs = now + UART_TX_HEARTBEAT_DELAY;
  }
}
