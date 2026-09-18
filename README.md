# ESP32 Audio Recorder & Player (INMP441 & MAX98357A)

An ESP32-based project that records audio using an I2S digital microphone (`INMP441`), stores it as an uncompressed 16-bit PCM `.wav` file on a MicroSD card via SPI, and plays it back directly through an I2S Class-D amplifier (`MAX98357A`).

---

## 🛠 Hardware Required

| Device | Description / Specs |
| :--- | :--- |
| **ESP32 DevKit V1** | Main Microcontroller (30 pins) |
| **INMP441** | Omnidirectional I2S Digital Microphone |
| **MAX98357A** | I2S Class-D Mono Amplifier |
| **MicroSD Card Module** | SPI Interface Storage Module |
| **Speaker** | 8Ω / 3W |
| **Push Button & LED** | Control Button + Onboard LED (GPIO2) |

---

## 🔌 Wiring Diagram & Pinout

### 1. MicroSD Card Module (SPI)
| MicroSD Pin | ESP32 Pin | Notes |
| :--- | :--- | :--- |
| **CS** | GPIO 5 | Chip Select |
| **MOSI** | GPIO 23 | SPI MOSI |
| **MISO** | GPIO 19 | SPI MISO |
| **SCK** | GPIO 18 | SPI Clock |
| **VCC** | 5V / 3.3V | According to module specs |
| **GND** | GND | Common Ground |

### 2. INMP441 Microphone (I2S - Recording)
| INMP441 Pin | ESP32 Pin | Notes |
| :--- | :--- | :--- |
| **SCK (BCLK)**| GPIO 14 | Serial Data Clock |
| **WS (LRC)** | GPIO 15 | Word Select / Left Right Clock |
| **SD (DOUT)** | GPIO 32 | Serial Data Out |
| **L/R** | GND | Tied to GND for Left Channel |
| **VDD** | 3.3V | **Do NOT connect to 5V** |
| **GND** | GND | Common Ground |

### 3. MAX98357A Amplifier (I2S - Playback)
| MAX98357A Pin | ESP32 Pin | Notes |
| :--- | :--- | :--- |
| **BCLK** | GPIO 22 | Bit Clock |
| **LRC (LRCK)**| GPIO 25 | Left/Right Clock |
| **DIN** | GPIO 26 | Data In |
| **GAIN** | 5V | Set Gain to +12dB (Hardware Boost) |
| **VIN** | 5V (VIN) | Capped at 5V for maximum output power |
| **GND** | GND | Common Ground |

### 4. Control Button
| Component | ESP32 Pin | Notes |
| :--- | :--- | :--- |
| **Button** | GPIO 4 | Configured as `INPUT_PULLUP` (Other pin to GND) |

---

## 🚀 Getting Started

1. **Software Prerequisites:**
   * Install [Arduino IDE](https://www.arduino.cc/en/software).
   * Install the ESP32 Board package (`Tools` -> `Board` -> `Boards Manager` -> Search `esp32`).
2. **MicroSD Card Preparation:**
   * Format your MicroSD card to **FAT32**.
   * Insert the card into the module before powering up the ESP32.
3. **Flashing the Code:**
   * Open the `.ino` file in Arduino IDE.
   * Select Board: **ESP32 Dev Module**.
   * Select your COM Port and click **Upload**.

---

## 📖 How It Works

* **Start Recording:** Press the **Push Button (GPIO 4)** once.
  * *Status:* `Onboard LED` turns **ON** (solid).
* **Stop Recording:** Press the **Push Button** again.
  * *Status:* `Onboard LED` flashes rapidly for 5 seconds while finalizing the `.wav` header.
* **Auto Playback:** After 5 seconds, the recorded file (`/record.wav`) plays automatically through the speaker.
  * *Status:* `Onboard LED` blinks slowly during playback.
* **Record Again:** Once playback finishes, press the button to start a new recording session (overwrites previous file).

---

## ⚙️ Audio Specifications

* **Format:** WAV (Uncompressed PCM)
* **Sample Rate:** 16,000 Hz
* **Bit Depth:** 16-bit
* **Channels:** Mono (1 channel)
