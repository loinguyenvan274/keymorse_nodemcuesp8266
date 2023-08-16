#include <Wire.h>
#include <U8g2lib.h>
#include <vector>

#define BUTTON_PIN D6
#define LED_PIN D8
#define SDA_PIN D2
#define SCL_PIN D1
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define LINE_HEIGHT 10

U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE, SCL_PIN, SDA_PIN);

String morseAlphabet[29] = {".-", "-...", "-.-.", "-..", ".", "..-.", "--.", "....", "..", ".---", "-.-", ".-..", "--", "-.", "---", ".--.", "--.-", ".-.", "...", "-", "..-", "...-", ".--", "-..-", "-.--", "--..", "..--", "..-.-"};
char alphabet[29] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', ' ', '\n'};

unsigned long buttonPressTimestamp = 0;
unsigned long lastButtonReleaseTimestamp = 0;
String morseCode = "";
std::vector<String> lines;

void setup() {
    Serial.begin(9600);
    Wire.begin(SDA_PIN, SCL_PIN);

    pinMode(BUTTON_PIN, INPUT_PULLUP);
    pinMode(LED_PIN, OUTPUT);

    u8g2.begin();
    u8g2.setFont(u8g2_font_ncenB08_tr);
    u8g2.drawStr(56, 32, "Start");
    u8g2.sendBuffer();
    delay(4000);
}

bool isBlinkingFast = false;
unsigned long lastBlinkTimestamp = 0;

void loop() {
    static bool prevButtonState = HIGH;
    bool currentButtonState = digitalRead(BUTTON_PIN);
    unsigned long currentTime = millis();

    if (currentButtonState == LOW && (currentTime - buttonPressTimestamp) > 3100) {
        isBlinkingFast = true;
    } else {
        isBlinkingFast = false;
    }

    if (isBlinkingFast && (currentTime - lastBlinkTimestamp) > 100) {  // Nháy LED mỗi 100ms
        digitalWrite(LED_PIN, !digitalRead(LED_PIN));
        lastBlinkTimestamp = currentTime;
    }

    handleButtonPress(prevButtonState, currentButtonState, currentTime);
    processMorseCode(currentTime);

    if (!isBlinkingFast) {
        digitalWrite(LED_PIN, !currentButtonState);
    }

    displayMorseAndText();
    prevButtonState = currentButtonState;
}

unsigned long blockButtonUntil = 0;  // Biến này sẽ lưu thời gian mà bạn cần block nút đến

void handleButtonPress(bool &prevButtonState, bool currentButtonState, unsigned long currentTime) {
    // Kiểm tra xem nút có bị block không
    if (currentTime < blockButtonUntil) return;

    if (prevButtonState == HIGH && currentButtonState == LOW) {
        buttonPressTimestamp = currentTime;
    } else if (prevButtonState == LOW && currentButtonState == HIGH) {
        unsigned long duration = currentTime - buttonPressTimestamp;

        if (duration < 200) {
            morseCode += ".";
        } else if (duration < 1000) {
            morseCode += "-";
        } else if (duration >= 3000) {
            resetDisplay();
        }
        lastButtonReleaseTimestamp = currentTime;
    }
}

void processMorseCode(unsigned long currentTime) {
    if (morseCode.endsWith("..--")) {
        if (!lines.empty() && !lines.back().isEmpty()) lines.back() += " ";
        morseCode = "";
    } else if (morseCode.endsWith("..-.-")) {
        lines.push_back("");
        morseCode = "";
    } else if (morseCode.endsWith(".....")) {
        if (!lines.empty() && !lines.back().isEmpty()) {
            lines.back().remove(lines.back().length() - 1);  // Xóa kí tự cuối
            morseCode = "";
            blockButtonUntil = currentTime + 500;  // Vô hiệu hóa nút trong 1 giây
        }
    } else if (currentTime - lastButtonReleaseTimestamp > 1500 && !morseCode.isEmpty()) {
        int index = findMorseIndex(morseCode);
        if (index != -1) {
            if (lines.empty()) lines.push_back("");
            lines.back() += alphabet[index];
            morseCode = "";
        } else {
            delay(500);  // Hiển thị mã Morse không hợp lệ trong 0.5 giây
            morseCode = "";
        }
    }
}


void displayMorseAndText() {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_ncenB08_tr);

    int y = 10;

    for (const auto &line : lines) {
        u8g2.drawStr(0, y, line.c_str());
        y += LINE_HEIGHT;
    }

    // Center the Morse code
    int morseWidth = u8g2.getStrWidth(morseCode.c_str());
    int xCentered = (SCREEN_WIDTH - morseWidth) / 2;
    u8g2.drawStr(xCentered, SCREEN_HEIGHT - LINE_HEIGHT, morseCode.c_str());

    u8g2.sendBuffer();
}

int findMorseIndex(String morse) {
    for (int i = 0; i < 29; i++) {
        if (morse == morseAlphabet[i]) {
            return i;
        }
    }
    return -1;
}

void resetDisplay() {
    morseCode = "";
    lines.clear();
}
