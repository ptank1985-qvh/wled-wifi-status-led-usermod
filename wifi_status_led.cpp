#include "wled.h"

class WiFiStatusLEDUsermod : public Usermod {

private:

  bool enabled = true;

  int8_t redPin = -1;
  int8_t bluePin = -1;

  bool polarityHigh = true;
  uint8_t blinkHz = 1;

  bool mqttEnabled = true;
  bool haEnabled = true;

  bool initDone = false;

  int8_t activeRedPin = -1;
  int8_t activeBluePin = -1;

  bool haConnected = false;

  unsigned long lastBlinkTime = 0;
  unsigned long lastHaFlashTime = 0;
  unsigned long haFlashStartTime = 0;
  unsigned long lastHaActivityTime = 0;

  bool blinkState = false;
  bool haFlashActive = false;

  static const char _name[];
  static const char _enabled[];

  void writeLED(int8_t pin, bool on)
  {
    if (pin < 0) return;

    bool outputState = polarityHigh ? on : !on;
    digitalWrite(pin, outputState ? HIGH : LOW);
  }

  void setRed(bool on)
  {
    writeLED(activeRedPin, on);
  }

  void setBlue(bool on)
  {
    writeLED(activeBluePin, on);
  }

  void setPurple(bool on)
  {
    setRed(on);
    setBlue(on);
  }

  void allOff()
  {
    setRed(false);
    setBlue(false);
  }

  void releasePins()
  {
    allOff();

    if (activeRedPin >= 0) {
      PinManager::deallocatePin(
        activeRedPin,
        PinOwner::UM_WiFiStatusLED
      );
      activeRedPin = -1;
    }

    if (activeBluePin >= 0) {
      PinManager::deallocatePin(
        activeBluePin,
        PinOwner::UM_WiFiStatusLED
      );
      activeBluePin = -1;
    }
  }

  void allocatePins()
  {
    activeRedPin = -1;
    activeBluePin = -1;

    if (!enabled) {
      allOff();
      return;
    }

    if (redPin >= 0) {
      if (PinManager::allocatePin(
            redPin,
            true,
            PinOwner::UM_WiFiStatusLED
          )) {
        activeRedPin = redPin;
        pinMode(activeRedPin, OUTPUT);
      } else {
        DEBUG_PRINTF(
          "WiFiStatusLED: Red GPIO %d unavailable.\n",
          redPin
        );
      }
    }

    if (bluePin >= 0) {
      if (
        bluePin != activeRedPin &&
        PinManager::allocatePin(
          bluePin,
          true,
          PinOwner::UM_WiFiStatusLED
        )
      ) {
        activeBluePin = bluePin;
        pinMode(activeBluePin, OUTPUT);
      } else if (bluePin == activeRedPin) {
        DEBUG_PRINTF(
          "WiFiStatusLED: Blue GPIO %d conflicts with Red GPIO.\n",
          bluePin
        );
      } else {
        DEBUG_PRINTF(
          "WiFiStatusLED: Blue GPIO %d unavailable.\n",
          bluePin
        );
      }
    }

    allOff();
  }

  void resetRuntime()
  {
    blinkState = false;
    lastBlinkTime = millis();

    haFlashActive = false;
    haFlashStartTime = 0;
    lastHaFlashTime = millis();
    lastHaActivityTime = 0;
  }

  /*
   * Home Assistant detection uses the same direct WebSocket/API
   * activity logic that works in the reference code.
   *
   * MQTT remains fully enabled and is still used for the normal
   * MQTT-connected purple status.
   */
  void checkDirectHAConnection()
  {
    if (!haEnabled) {
      haConnected = false;
      return;
    }

    unsigned long now = millis();

    if (ws.count() > 0) {
      if (!haConnected) {
        haConnected = true;
        lastHaFlashTime = now;
      }

      lastHaActivityTime = now;
    } else {
      if (
        haConnected &&
        (now - lastHaActivityTime > 15000UL)
      ) {
        haConnected = false;
        haFlashActive = false;
      }
    }
  }

  void updateStatusLED()
  {
    if (!enabled) {
      allOff();
      return;
    }

    /*
     * Direct Home Assistant connection:
     * Purple static + short OFF flash every 3 seconds.
     *
     * MQTT is NOT disabled here. MQTT continues to work normally.
     */
    checkDirectHAConnection();

    if (
      haEnabled &&
      WLED_CONNECTED &&
      haConnected
    ) {
      unsigned long now = millis();

      if (
        !haFlashActive &&
        (now - lastHaFlashTime >= 3000UL)
      ) {
        haFlashActive = true;
        haFlashStartTime = now;
        setPurple(false);
      }

      if (haFlashActive) {
        if (
          now - haFlashStartTime >= 180UL
        ) {
          haFlashActive = false;
          lastHaFlashTime = now;
          setPurple(true);
        } else {
          setPurple(false);
        }
      } else {
        setPurple(true);
      }

      return;
    }

    /*
     * MQTT connected:
     * Purple static.
     */
    if (
      mqttEnabled &&
      WLED_MQTT_CONNECTED
    ) {
      setPurple(true);
      return;
    }

    /*
     * WiFi connected:
     * Blue static.
     */
    if (WLED_CONNECTED) {
      setBlue(true);
      setRed(false);
      return;
    }

    /*
     * WLED AP mode:
     * Red static.
     */
    if (apActive) {
      setRed(true);
      setBlue(false);
      return;
    }

    /*
     * WiFi discovery / connecting:
     * Red blinking at selected frequency.
     */
    unsigned long halfPeriod =
      1000UL / (2UL * blinkHz);

    unsigned long now = millis();

    if (
      now - lastBlinkTime >= halfPeriod
    ) {
      lastBlinkTime = now;
      blinkState = !blinkState;
    }

    setRed(blinkState);
    setBlue(false);
  }

public:

  void setup() override
  {
    if (!enabled) {
      allOff();
      initDone = true;
      return;
    }

    allocatePins();
    resetRuntime();

    initDone = true;

    DEBUG_PRINTLN(
      F("WiFiStatusLED: setup complete.")
    );
  }

  void loop() override
  {
    updateStatusLED();
  }

  void connected() override
  {
    resetRuntime();
  }

  /*
   * WLED API/JSON activity is also used as an HA activity signal.
   * This is the key part copied from the working reference logic.
   */
  void addToJsonInfo(JsonObject& root) override
  {
    if (
      haEnabled &&
      WLED_CONNECTED
    ) {
      lastHaActivityTime = millis();
    }

    JsonObject user = root["u"];

    if (user.isNull()) {
      user = root.createNestedObject("u");
    }

    JsonArray status =
      user.createNestedArray(FPSTR(_name));

    status.add(enabled);

    if (!enabled) {
      return;
    }

    status.add(activeRedPin);
    status.add(activeBluePin);
    status.add(haConnected);
  }

  void addToConfig(JsonObject& root) override
  {
    JsonObject top =
      root.createNestedObject(FPSTR(_name));

    top[FPSTR(_enabled)] = enabled;

    JsonArray pins =
      top.createNestedArray("pin");

    pins.add(redPin);
    pins.add(bluePin);

    top["polarity"] =
      polarityHigh ? 0 : 1;

    top["blinkHz"] =
      blinkHz;

    top["mqttEnabled"] =
      mqttEnabled;

    top["haEnabled"] =
      haEnabled;
  }

  bool readFromConfig(JsonObject& root) override
  {
    int8_t oldRedPin = redPin;
    int8_t oldBluePin = bluePin;
    bool oldEnabled = enabled;

    JsonObject top =
      root[FPSTR(_name)];

    bool configComplete =
      !top.isNull();

    configComplete &=
      getJsonValue(
        top[FPSTR(_enabled)],
        enabled,
        true
      );

    configComplete &=
      getJsonValue(
        top["pin"][0],
        redPin,
        -1
      );

    configComplete &=
      getJsonValue(
        top["pin"][1],
        bluePin,
        -1
      );

    int8_t polarity =
      polarityHigh ? 0 : 1;

    configComplete &=
      getJsonValue(
        top["polarity"],
        polarity,
        0
      );

    polarityHigh =
      (polarity == 0);

    configComplete &=
      getJsonValue(
        top["blinkHz"],
        blinkHz,
        1
      );

    if (
      blinkHz != 1 &&
      blinkHz != 2 &&
      blinkHz != 4 &&
      blinkHz != 8
    ) {
      blinkHz = 1;
    }

    configComplete &=
      getJsonValue(
        top["mqttEnabled"],
        mqttEnabled,
        true
      );

    configComplete &=
      getJsonValue(
        top["haEnabled"],
        haEnabled,
        true
      );

    if (
      initDone &&
      (
        redPin != oldRedPin ||
        bluePin != oldBluePin ||
        enabled != oldEnabled
      )
    ) {
      releasePins();

      if (enabled) {
        allocatePins();
      }

      resetRuntime();
    }

    if (!haEnabled) {
      haConnected = false;
      haFlashActive = false;
    }

    return configComplete;
  }

  void appendConfigData() override
  {
    oappend(F(
      "addInfo('WiFi Status LED:pin[]',0,'','Red');"
    ));

    oappend(F(
      "addInfo('WiFi Status LED:pin[]',1,'','Blue');"
    ));

    oappend(F(
      "dd=addDropdown('WiFi Status LED','polarity');"
    ));

    oappend(F(
      "addOption(dd,'High',0);"
    ));

    oappend(F(
      "addOption(dd,'Low',1);"
    ));

    oappend(F(
      "dd=addDropdown('WiFi Status LED','blinkHz');"
    ));

    oappend(F(
      "addOption(dd,'1 Hz',1);"
    ));

    oappend(F(
      "addOption(dd,'2 Hz',2);"
    ));

    oappend(F(
      "addOption(dd,'4 Hz',4);"
    ));

    oappend(F(
      "addOption(dd,'8 Hz',8);"
    ));
  }

#ifndef WLED_DISABLE_MQTT

  /*
   * MQTT remains enabled.
   * We do NOT use MQTT to detect HA anymore.
   * The direct HA detection above is used instead.
   */
  void onMqttConnect(bool sessionPresent) override
  {
    if (
      !enabled ||
      !mqttEnabled
    ) {
      return;
    }

    /*
     * Keep the MQTT connection alive for normal WLED/MQTT
     * operation. No Home Assistant status subscription is needed.
     */
    DEBUG_PRINTLN(
      F("WiFiStatusLED: MQTT connected.")
    );
  }

  bool onMqttMessage(
    char* topic,
    char* payload
  ) override
  {
    /*
     * MQTT is intentionally kept available.
     * HA detection is handled through direct WLED activity.
     */
    (void)topic;
    (void)payload;

    return false;
  }

#endif

  uint16_t getId() override
  {
    return USERMOD_ID_WIFI_STATUS_LED;
  }
};

const char WiFiStatusLEDUsermod::_name[] PROGMEM =
  "WiFi Status LED";

const char WiFiStatusLEDUsermod::_enabled[] PROGMEM =
  "enabled";

static WiFiStatusLEDUsermod wifiStatusLED;

REGISTER_USERMOD(wifiStatusLED);
