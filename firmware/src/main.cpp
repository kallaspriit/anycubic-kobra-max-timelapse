/**
 * 3D printer smartphone remote used for taking timelapse pictures.
 * 
 * The onboard LED is used to show connection state:
 * - constantly on: the board is waiting for binding
 * - blinking slowly: successfully paired and awake
 * - off: the board is in deep sleep
 * 
 * @author Priit Kallas <kallaspriit@gmail.com> 05.2022
 */

// possibly helpful links
// https://www.adafruit.com/product/4062
// https://learn.adafruit.com/introducing-the-adafruit-nrf52840-feather?view=all
// https://github.com/adafruit/Adafruit_nRF52_Arduino
// https://w3c.github.io/uievents/tools/key-event-viewer.html

#include <bluefruit.h>

// button pins mapping (A1 and A2 dont seem to work, auto wake up)
const int BUTTON_BOARD_PIN = 7;   // built-in user button
const int BUTTON_REMOTE_PIN = A0; // pin next to ground

// pin to use to show bluetooth connection status
// connection led is constantly on when pairing and blinks at interval if connected (to preserve battery)
const int CONNECTION_LED_PIN = LED_CONN;

// number of mapped buttons
const int BUTTON_COUNT = 2;

// button pins (mapping below matches button order)
const byte BUTTON_PINS[BUTTON_COUNT] = {
    BUTTON_BOARD_PIN,
    BUTTON_REMOTE_PIN,
};

// mapping of button index to action based on selected game
const uint8_t BUTTON_MAPPING[BUTTON_COUNT] = {
    // BUTTON_BOARD_PIN
    HID_KEY_VOLUME_DOWN,

    // BUTTON_REMOTE_PIN
    HID_KEY_VOLUME_DOWN,
};

// timing configuration
const unsigned long CONNECTED_BLINK_INTERVAL_MS = 10000;       // how often to blink if connected
const unsigned long CONNECTING_BLINK_INTERVAL_MS = 1000;       // how often to blink if not connected
const unsigned long CONNECTION_GIVE_UP_DURATION_MS = 30000;    // how long to attempt to connect to a host before giving up and goind to sleep
const unsigned long CONNECTION_BLINK_ON_DURATION_MS = 10;      // how long to show led when blinking
const unsigned long REPORT_BUTTONS_CHANGED_INTERVAL_MS = 1000; // minimum interval at which to check/report button presses

// runtime info
unsigned long lastButtonPressTime = 0;
unsigned long lastReportBatteryTime = 0;
unsigned long lastConnectionBlinkToggleTime = 0;
unsigned long lastConnectedTime = 0;
int lastPressedButtonCount = 0;
int connectionBlinkState = LOW;
int activeProfileIndex = 0;
bool wasConnected = false;
bool haveReportedBattery = false;

// services
BLEDis deviceInformationService;
BLEHidAdafruit hidService;

// starts bluetooth advertising
void startAdvertising()
{
  // advertising as HID service
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addTxPower();
  Bluefruit.Advertising.addAppearance(BLE_APPEARANCE_HID_KEYBOARD);

  // add the hid service
  Bluefruit.Advertising.addService(hidService);

  // also advertise device name
  Bluefruit.Advertising.addName();

  // enable auto advertising if disconnected
  Bluefruit.Advertising.restartOnDisconnect(true);

  // configure intervals for afast and slow modes (in unit of 0.625 ms)
  Bluefruit.Advertising.setInterval(32, 244);

  // configure number of seconds in fast advertising mode
  Bluefruit.Advertising.setFastTimeout(30);

  // start advertising forever
  Bluefruit.Advertising.start(0);
}

// called once on initial setup
void setup()
{
  // setup serial
  Serial.begin(115200);

  Serial.println("-- Printer remote --");

  // setup pins
  pinMode(CONNECTION_LED_PIN, OUTPUT);

  // setup button pins as inputs with pullup and wake-up sensing
  for (int i = 0; i < BUTTON_COUNT; i++)
  {
    pinMode(BUTTON_PINS[i], INPUT_PULLUP);
  }

  // configure bluetooth
  Bluefruit.begin();
  Bluefruit.setTxPower(4); // Check bluefruit.h for supported values
  // Bluefruit.setTxPower(-12); // Check bluefruit.h for supported values
  Bluefruit.setName("Printer remote");

  // disable automatic connection led
  Bluefruit.autoConnLed(false);

  // configure and start device information service
  deviceInformationService.setManufacturer("Stagnation Lab");
  deviceInformationService.setModel("Printer remote v1");
  deviceInformationService.begin();

  // start HID
  hidService.begin();

  // start advertising
  startAdvertising();

  // initialize last buttons changed time (board goes to sleep after a while)
  lastButtonPressTime = millis();
}

// main loop, called continuosly as fast as possible
void loop()
{
  unsigned int currentTime = millis();

  // detect connected state
  bool isConnected = Bluefruit.connected() > 0;
  bool connectionEstablished = isConnected && !wasConnected;
  bool connectionLost = !isConnected && wasConnected;

  // update was connected state
  wasConnected = isConnected;

  // update last connected time
  if (isConnected)
  {
    lastConnectedTime = currentTime;
  }

  // log connection established / lost
  if (connectionEstablished)
  {
    Serial.println("Connection established");

    // avoid quickly going to sleep
    lastButtonPressTime = currentTime;
  }
  else if (connectionLost)
  {
    Serial.println("Connection lost");

    // make sure to report battery again
    haveReportedBattery = false;
  }

  // calculate time since buttons were last pressed
  unsigned long timeSinceLastButtonPress = currentTime - lastButtonPressTime;

  // calculate time since last connected
  unsigned long timeSinceLastConnected = currentTime - lastConnectedTime;

  // consider button changes at certain interval (only if connected)
  if (isConnected && timeSinceLastButtonPress >= REPORT_BUTTONS_CHANGED_INTERVAL_MS)
  {

    // handle pressing any of the buttons
    uint8_t keyCodes[] = {HID_KEY_NONE,
                          HID_KEY_NONE,
                          HID_KEY_NONE,
                          HID_KEY_NONE,
                          HID_KEY_NONE,
                          HID_KEY_NONE};
    int pressedButtonCount = 0;

    for (int i = 0; i < BUTTON_COUNT; i++)
    {
      // bool isButtonDown = Buttons.down(i);
      bool isButtonDown = digitalRead(BUTTON_PINS[i]) == LOW;

      // skip if button is not down
      if (!isButtonDown)
      {
        continue;
      }

      // get pressed button key code
      uint8_t keyCode = BUTTON_MAPPING[i];
      int keyCodeIndex = pressedButtonCount++;

      // Serial.print("Button #");
      // Serial.print(i);
      // Serial.print(" is down, adding key code ");
      // Serial.print(keyCode);
      // Serial.print(" (");
      // Serial.print(pressedButtonCount);
      // Serial.println(")");

      // append to list of pressed buttons
      keyCodes[keyCodeIndex] = keyCode;

      // break loop if maximum number of buttons are already pressed
      if (pressedButtonCount == 6)
      {
        break;
      }
    }

    // report pressed buttons or release if number of pressed buttons changes
    if (pressedButtonCount != lastPressedButtonCount)
    {
      if (pressedButtonCount > 0)
      {
        Serial.print("Reporting ");
        Serial.print(pressedButtonCount);
        Serial.println(" buttons");

        hidService.keyboardReport(0, keyCodes);
      }
      else
      {
        Serial.println("Releasing buttons");

        hidService.keyRelease();
      }

      lastPressedButtonCount = pressedButtonCount;
    }

    // update last buttons changed time if any of the buttons was pressed
    if (pressedButtonCount > 0)
    {
      lastButtonPressTime = currentTime;
    }
  }

  // decide connection led state
  int targetConnectionLedState = LOW;

  // decide connection led blink interval (blinks faster if connecting)
  unsigned long blinkInterval = isConnected ? CONNECTED_BLINK_INTERVAL_MS : CONNECTING_BLINK_INTERVAL_MS;
  unsigned long timeSinceLastBlink = currentTime - lastConnectionBlinkToggleTime;

  // toggle blink led at interval
  if (timeSinceLastBlink >= blinkInterval)
  {
    lastConnectionBlinkToggleTime = currentTime;
  }

  if (timeSinceLastBlink <= CONNECTION_BLINK_ON_DURATION_MS)
  {
    targetConnectionLedState = HIGH;
  }
  else
  {
    targetConnectionLedState = LOW;
  }

  // only update connection led state if status has changed
  if (targetConnectionLedState != connectionBlinkState)
  {
    // log connecting duration
    if (!isConnected)
    {
      Serial.print("Connecting ");
      Serial.print(timeSinceLastConnected);
      Serial.print("/");
      Serial.print(CONNECTION_GIVE_UP_DURATION_MS);
      Serial.println("ms");
    }

    connectionBlinkState = targetConnectionLedState;

    digitalWrite(CONNECTION_LED_PIN, connectionBlinkState);
  }
}