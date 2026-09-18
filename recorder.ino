#include <Arduino.h>
#include <SD.h>
#include <SPI.h>
#include "driver/i2s.h"

// --- CẤU HÌNH CHÂN PIN ---
#define SD_CS          5
#define SPI_MOSI      23
#define SPI_MISO      19
#define SPI_SCK       18

#define BUTTON_PIN     4
#define LED_ONBOARD    2  // LED tích hợp sẵn trên ESP32 DevKit V1

#define I2S_PORT       I2S_NUM_0

// Chân MAX98357A (Loa)
#define I2S_LOA_BCLK   22
#define I2S_LOA_LRC    25
#define I2S_LOA_DOUT   26

// Chân INMP441 (Micro)
#define I2S_MIC_BCLK   14
#define I2S_MIC_WS     15
#define I2S_MIC_DIN    32

#define SAMPLE_RATE    16000

// Cấu trúc WAV Header
struct WAVHeader {
    char riff[4] = {'R', 'I', 'F', 'F'};
    uint32_t chunkSize = 0;
    char wave[4] = {'W', 'A', 'V', 'E'};
    char fmt[4] = {'f', 'm', 't', ' '};
    uint32_t subchunk1Size = 16;
    uint16_t audioFormat = 1;
    uint16_t numChannels = 1;
    uint32_t sampleRate = SAMPLE_RATE;
    uint32_t byteRate = SAMPLE_RATE * 1 * 2;
    uint16_t blockAlign = 2;
    uint16_t bitsPerSample = 16;
    char data[4] = {'d', 'a', 't', 'a'};
    uint32_t subchunk2Size = 0;
};

enum State { IDLE, RECORDING, PLAYING };
State currentState = IDLE;

File recFile;
uint32_t totalDataBytes = 0;

// Khai báo trước nguyên mẫu hàm
void playRecording();

// Khởi tạo I2S cho Micro INMP441 (Ghi âm)
void initI2S_Micro() {
    i2s_driver_uninstall(I2S_PORT);
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
        .sample_rate = SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 8,
        .dma_buf_len = 512,
        .use_apll = false
    };
    i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_MIC_BCLK,
        .ws_io_num = I2S_MIC_WS,
        .data_out_num = I2S_PIN_NO_CHANGE,
        .data_in_num = I2S_MIC_DIN
    };
    i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
    i2s_set_pin(I2S_PORT, &pin_config);
}

// Khởi tạo I2S cho MAX98357A (Phát loa)
void initI2S_Loa(uint32_t sampleRate) {
    i2s_driver_uninstall(I2S_PORT);
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate = sampleRate,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 8,
        .dma_buf_len = 512,
        .use_apll = false
    };
    i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_LOA_BCLK,
        .ws_io_num = I2S_LOA_LRC,
        .data_out_num = I2S_LOA_DOUT,
        .data_in_num = I2S_PIN_NO_CHANGE
    };
    i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
    i2s_set_pin(I2S_PORT, &pin_config);
}

void startRecording() {
    Serial.println(F(">>> BẮT ĐẦU GHI ÂM..."));
    digitalWrite(LED_ONBOARD, HIGH); // LED sáng liên tục khi đang GHI ÂM

    if (SD.exists("/record.wav")) SD.remove("/record.wav");
    recFile = SD.open("/record.wav", FILE_WRITE);
    if (!recFile) {
        Serial.println(F("Lỗi mở file!"));
        digitalWrite(LED_ONBOARD, LOW);
        return;
    }

    WAVHeader header;
    recFile.write((uint8_t*)&header, sizeof(WAVHeader));
    totalDataBytes = 0;
    initI2S_Micro();
    currentState = RECORDING;
}

void stopRecording() {
    Serial.println(F(">>> DỪNG GHI ÂM. Đang lưu file..."));
    digitalWrite(LED_ONBOARD, LOW);

    WAVHeader header;
    header.subchunk2Size = totalDataBytes;
    header.chunkSize = 36 + totalDataBytes;

    recFile.seek(0);
    recFile.write((uint8_t*)&header, sizeof(WAVHeader));
    recFile.close();

    i2s_driver_uninstall(I2S_PORT);
    currentState = IDLE;

    // Nhấp nháy LED nhanh trong 5 giây chờ
    Serial.println(F("Lưu xong. Chờ 5 giây..."));
    for (int i = 0; i < 20; i++) {
        digitalWrite(LED_ONBOARD, !digitalRead(LED_ONBOARD));
        delay(250);
    }
    digitalWrite(LED_ONBOARD, LOW);

    playRecording();
}

void playRecording() {
    Serial.println(F(">>> PHÁT LẠI FILE GHI ÂM..."));
    File file = SD.open("/record.wav");
    if (!file) {
        Serial.println(F("Lỗi mở file phát!"));
        return;
    }

    WAVHeader header;
    file.read((uint8_t*)&header, sizeof(WAVHeader));

    initI2S_Loa(header.sampleRate);
    currentState = PLAYING;

    uint8_t buffer[512];
    size_t bytesRead = 0;
    size_t bytesWritten = 0;
    unsigned long lastBlink = 0;

    while (file.available()) {
        bytesRead = file.read(buffer, sizeof(buffer));
        i2s_write(I2S_PORT, buffer, bytesRead, &bytesWritten, portMAX_DELAY);

        // Nhấp nháy LED chậm (500ms) khi PHÁT LOA
        if (millis() - lastBlink > 500) {
            lastBlink = millis();
            digitalWrite(LED_ONBOARD, !digitalRead(LED_ONBOARD));
        }
    }

    file.close();
    i2s_driver_uninstall(I2S_PORT);
    digitalWrite(LED_ONBOARD, LOW);
    currentState = IDLE;
    Serial.println(F(">>> PHÁT XONG!"));
}

void setup() {
    Serial.begin(115200);
    pinMode(BUTTON_PIN, INPUT_PULLUP);

    pinMode(LED_ONBOARD, OUTPUT);
    digitalWrite(LED_ONBOARD, LOW);

    SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI, SD_CS);
    if (!SD.begin(SD_CS)) {
        Serial.println(F("Lỗi kết nối thẻ SD!"));
        while (true);
    }
    Serial.println(F("Hệ thống sẵn sàng. Bấm nút để bắt đầu ghi âm."));
}

void loop() {
    static bool lastBtnState = HIGH;
    bool btnState = digitalRead(BUTTON_PIN);

    if (lastBtnState == HIGH && btnState == LOW) {
        delay(50); // Debounce
        if (digitalRead(BUTTON_PIN) == LOW) {
            if (currentState == IDLE) {
                startRecording();
            } else if (currentState == RECORDING) {
                stopRecording();
            }
        }
    }
    lastBtnState = btnState;

    if (currentState == RECORDING) {
        uint8_t i2sBuffer[512];
        size_t bytesRead = 0;
        i2s_read(I2S_PORT, i2sBuffer, sizeof(i2sBuffer), &bytesRead, portMAX_DELAY);
        if (bytesRead > 0) {
            recFile.write(i2sBuffer, bytesRead);
            totalDataBytes += bytesRead;
        }
    }
}