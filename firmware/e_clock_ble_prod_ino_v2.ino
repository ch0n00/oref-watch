#include <Adafruit_NeoPixel.h>
#include <BLEDevice.h>
#include <BLEServer.h>

#define LED_PIN      5
#define LED_COUNT    1

#define MOTOR_PIN    3
#define BUTTON_PIN   7

#define SERVICE_UUID        "7b7f0001-1111-2222-3333-abcdef123456"
#define CHARACTERISTIC_UUID "7b7f0002-1111-2222-3333-abcdef123456"

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

int pressCount = 0;
unsigned long lastPress = 0;
bool lastButton = HIGH;

// current blinking state
bool blinking = false;
bool ledState = false;
unsigned long lastBlink = 0;
int blinkR = 0;
int blinkG = 0;
int blinkB = 0;
const unsigned long blinkInterval = 500;

void setColor(int r, int g, int b) {
  strip.setPixelColor(0, strip.Color(r, g, b));
  strip.show();
}

void startBlinking(int r, int g, int b) {
  blinkR = r;
  blinkG = g;
  blinkB = b;
  blinking = true;
  ledState = false;
  lastBlink = millis();
}

void stopBlinking() {
  blinking = false;
  ledState = false;
  setColor(0, 0, 0);
}

void handleBlinking() {
  if (!blinking) return;

  if (millis() - lastBlink >= blinkInterval) {
    lastBlink = millis();
    ledState = !ledState;

    if (ledState) {
      setColor(blinkR, blinkG, blinkB);
    } else {
      setColor(0, 0, 0);
    }
  }
}

void vibrateWithLed(int count, int duration, int r, int g, int b) {
  blinking = false;

  for (int i = 0; i < count; i++) {
    setColor(r, g, b);
    digitalWrite(MOTOR_PIN, HIGH);
    delay(duration);

    setColor(0, 0, 0);
    digitalWrite(MOTOR_PIN, LOW);
    delay(200);
  }

  startBlinking(r, g, b);
}

void runState(char cmd) {
  switch(cmd) {
    case '0':
      digitalWrite(MOTOR_PIN, LOW);
      stopBlinking();
      break;

    case '1':
      // yellow: LED follows vibration, then keeps blinking yellow
      vibrateWithLed(5, 1000, 255, 120, 0);
      break;

    case '2':
      // red: LED follows vibration, then keeps blinking red
      vibrateWithLed(7, 300, 255, 0, 0);
      break;

    case '3':
      // green: LED follows vibration, then keeps blinking green
      vibrateWithLed(1, 5000, 0, 255, 0);
      break;
  }
}

class Callbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *c) {
    String value = c->getValue();

    if (value.length() > 0) {
      runState(value[0]);
    }
  }
};

class ServerCallbacks : public BLEServerCallbacks {
  void onDisconnect(BLEServer* server) {
    delay(200);
    BLEDevice::startAdvertising();
  }
};

void setup() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  pinMode(MOTOR_PIN, OUTPUT);
  digitalWrite(MOTOR_PIN, LOW);

  strip.begin();
  strip.setBrightness(20);
  strip.show();
  setColor(0, 0, 0);

  BLEDevice::init("Oref-Watch");

  BLEServer *server = BLEDevice::createServer();
  server->setCallbacks(new ServerCallbacks());

  BLEService *service = server->createService(SERVICE_UUID);

  BLECharacteristic *characteristic =
      service->createCharacteristic(
          CHARACTERISTIC_UUID,
          BLECharacteristic::PROPERTY_WRITE
      );

  characteristic->setCallbacks(new Callbacks());

  service->start();

  BLEAdvertising *adv = BLEDevice::getAdvertising();
  adv->addServiceUUID(SERVICE_UUID);

  BLEDevice::startAdvertising();
}

void loop() {
  handleBlinking();

  bool button = digitalRead(BUTTON_PIN);

  if (button == LOW && lastButton == HIGH) {
    delay(30);

    if (digitalRead(BUTTON_PIN) == LOW) {
      pressCount++;
      lastPress = millis();
    }
  }

  lastButton = button;

  if (pressCount > 0 && millis() - lastPress > 1000) {
    if (pressCount == 1)
      runState('1');
    else if (pressCount == 2)
      runState('2');
    else if (pressCount == 3)
      runState('3');
    else if (pressCount == 4)
      runState('0');

    pressCount = 0;
  }
}