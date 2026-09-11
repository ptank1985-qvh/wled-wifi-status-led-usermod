# WLED WiFi Status LED Usermod

A WLED Usermod that provides a simple visual indication of the WLED device's network and connection status using a dual-colour Red + Blue LED.

The Usermod monitors WiFi, WLED AP mode, MQTT, and Home Assistant activity and displays the corresponding status through the LED.

## Features

- WiFi connection status indication
- WLED Access Point (AP) mode indication
- MQTT connection indication
- Home Assistant connection indication
- Configurable Red and Blue GPIO pins
- Support for Common Anode or Common Cathode dual-colour LEDs
- Configurable LED polarity
- Configurable WiFi connection blink frequency
- GPIO conflict protection using WLED PinManager
- Optional MQTT status handling
- Optional Home Assistant detection
- Non-blocking status updates without long delays

---

## Recommended Hardware

This Usermod is specifically designed for a **3-pin, 2-colour dual-colour LED**.

### Recommended LED

- Dual-colour **Red + Blue LED**
- Common Anode (CA) or Common Cathode (CC)
- One common connection
- Separate Red and Blue control connections

The Usermod provides a polarity setting so the LED can be configured according to the type of hardware being used.

> **Important:** This Usermod is intended specifically for a Common Anode or Common Cathode dual-colour Red + Blue LED. Using different LED hardware or different LED colours may produce different visual results.

Use an appropriate current-limiting resistor for the LED and make sure the selected GPIO pins are suitable for your WLED hardware.

---

## LED Status

| WLED / Network Status | LED Output |
|---|---|
| WiFi discovery / connecting | Red blinking |
| WLED AP mode | Red |
| WiFi connected | Blue |
| MQTT connected | Red + Blue |
| Home Assistant connected | Red + Blue with a short OFF flash every 3 seconds |

### Combined Red + Blue

The Usermod controls the Red and Blue LED channels independently.

With the recommended Red + Blue dual-colour LED:

- Red channel ON → **Red**
- Blue channel ON → **Blue**
- Red + Blue channels ON → **Purple**

The Purple indication is produced by the dual-colour LED hardware when both Red and Blue channels are active.

The Usermod does **not** perform software colour mixing.

---

## Connection Priority

When multiple connection conditions are present, the Usermod uses the following priority:

1. Home Assistant connection
2. MQTT connection
3. WiFi connection
4. WLED AP mode
5. WiFi discovery / connecting

Home Assistant status takes priority when an active direct connection is detected.

---

## Configuration

The Usermod provides configuration through the WLED Usermod settings.

### Red GPIO

GPIO used to control the Red LED channel.

### Blue GPIO

GPIO used to control the Blue LED channel.

The Red and Blue GPIO pins must be different.

### LED Polarity

Select the polarity required by your LED hardware:

- **High** — LED channel is active when the GPIO is HIGH
- **Low** — LED channel is active when the GPIO is LOW

This allows the Usermod to work with the intended Common Anode or Common Cathode dual-colour LED configuration.

### Blink Frequency

The WiFi connecting indicator can be configured to:

- 1 Hz
- 2 Hz
- 4 Hz
- 8 Hz

---

## MQTT

MQTT handling can be enabled or disabled from the Usermod settings.

When MQTT is connected and no higher-priority Home Assistant status is active, both Red and Blue channels are turned ON.

The Usermod does not use MQTT to detect the Home Assistant connection.

MQTT remains available for normal WLED/MQTT operation.

---

## Home Assistant

Home Assistant detection can be enabled or disabled.

The Usermod uses direct WLED WebSocket/API activity as the Home Assistant activity signal.

When an active Home Assistant connection is detected:

- Red + Blue channels remain ON
- The LED briefly turns OFF every 3 seconds
- Normal MQTT functionality remains available

The OFF flash duration is approximately 180 ms.

---

## GPIO Protection

The Usermod uses WLED's **PinManager** when allocating the configured GPIO pins.

If a selected GPIO is already reserved or unavailable, the Usermod will not use that pin.

This helps prevent GPIO conflicts with other WLED functions or Usermods.

---

## Installation

This project is provided as a standalone WLED Usermod repository.

Repository:

https://github.com/ptank1985-qvh/wled-wifi-status-led-usermod

The Usermod is intended to be integrated into a WLED build using WLED's custom Usermod mechanism.

Please refer to the WLED documentation for the current procedure for adding standalone Usermods to a build.

---

## Compatibility

This Usermod is based on the WLED Usermod v2 API.

It is intended for WLED builds that provide the APIs used by this Usermod, including:

- Usermod v2
- WLED WiFi connection status
- WLED AP mode status
- MQTT status
- WebSocket/API activity
- WLED PinManager

Always test the Usermod with your specific WLED version and hardware before deploying it to a production device.

---

## Troubleshooting

### LED does not turn ON

Check:

1. The selected GPIO numbers.
2. LED wiring.
3. The Common Anode / Common Cathode configuration.
4. The selected polarity setting.
5. Whether the GPIO is already being used by another WLED feature or Usermod.

### Red and Blue appear reversed

Check the Red and Blue GPIO connections and verify that the configured GPIOs match the physical LED channels.

### Purple does not appear

Purple requires both the Red and Blue channels to be active simultaneously.

The actual combined colour depends on the LED hardware being used.

---

## Safety and Hardware Notes

Always use an appropriate current-limiting resistor with the LED.

Do not connect an LED directly to a GPIO without suitable current limiting.

Verify the electrical characteristics and GPIO limitations of your specific ESP/WLED hardware before making connections.

---

## License

MIT License

---

## Author

Created as a community WLED Usermod.

If you find a problem or have an improvement, please open an issue or pull request in this repository.
