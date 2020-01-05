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
  UARTRxCallbacks(BLECharacteristic *txChar) : txChar(txChar) {}

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
        txChar->setValue(data, sizeof data - 1);
        txChar->notify();
      }
    }
  }
  BLECharacteristic *txChar;
};

class UARTServiceHandler {
 public:
  UARTServiceHandler(BLEServer *bleServer) {
    uartService = bleServer->createService(NF_UART_SERVICE_UUID);

    txChar = uartService->createCharacteristic(
        NF_UART_TX_CHAR_UUID, BLECharacteristic::PROPERTY_NOTIFY);
    txChar->addDescriptor(new BLE2902());

    rxChar = uartService->createCharacteristic(
        NF_UART_RX_CHAR_UUID, BLECharacteristic::PROPERTY_WRITE);
    rxChar->setCallbacks(new UARTRxCallbacks(txChar));
  }

  void start() { uartService->start(); }

  void tick() {
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

 private:
  BLEService *uartService = NULL;
  BLECharacteristic *txChar;
  BLECharacteristic *rxChar;
  unsigned long nextTxTimeMs = 0;
};
