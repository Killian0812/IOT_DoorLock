#include <Keypad.h>
#include <Arduino.h>
#include <U8x8lib.h>

#define PASSWORD "ABC"

// Setup OLED Screen
#define OLED_SCL_PIN 22
#define OLED_SDA_PIN 21
U8X8_SH1106_128X64_NONAME_HW_I2C u8x8(/* reset=*/U8X8_PIN_NONE, OLED_SCL_PIN, OLED_SDA_PIN);
#define U8LOG_WIDTH 16
#define U8LOG_HEIGHT 8
uint8_t u8log_buffer[U8LOG_WIDTH * U8LOG_HEIGHT];
U8X8LOG u8x8log;

// Setup Keypad
const uint8_t ROWS = 4;
const uint8_t COLS = 4;
char keys[ROWS][COLS] = {
  { '1', '2', '3', 'A' },
  { '4', '5', '6', 'B' },
  { '7', '8', '9', 'C' },
  { '*', '0', '#', 'D' }
};
uint8_t rowPins[ROWS] = { 32, 33, 25, 26 };  // Pins connected to R1, R2, R3, R4
uint8_t colPins[COLS] = { 27, 14, 12, 13 };  // Pins connected to C1, C2, C3, C4
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// Show Enter password screen
void showTyping() {
  u8x8log.print("Password:\n");
}

// Clear whole screen
void clearScreen() {
  u8x8.clear();
  u8x8.clearDisplay();
  u8x8.initDisplay();
}

// Show corresponding menu screen
int onMenu;
void showMenu() {
  switch (onMenu) {
    case 1:
      {
        u8x8log.print("Options:\n");
        u8x8log.print("A. Open door\n");
        u8x8log.print("B. Door mode\n");
        u8x8log.print("C. Change password\n");
        u8x8log.print("D. Back\n");
        break;
      }
    case 2:
      {
        u8x8log.print("Door mode:\n");
        u8x8log.print("A. Automatic\n");
        u8x8log.print("B. Locked\n");
        u8x8log.print("D. Back\n");
        break;
      }
    case 3:
      {
        u8x8log.print("New password:\n");
        u8x8log.print("D. Back\n");
      }
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("Serial connected");

  u8x8.begin();
  u8x8.setFont(u8x8_font_chroma48medium8_r);
  u8x8log.begin(u8x8, U8LOG_WIDTH, U8LOG_HEIGHT, u8log_buffer);
  u8x8log.setRedrawMode(1);
  if (onMenu)
    showMenu();
  else
    showTyping();
}

bool typing;
String s;

void loop() {
  char key = keypad.getKey();
  if (key != NO_KEY) {
    Serial.println(key);

    if (!typing) {
      typing = true;
      s = "";
    }
    if (typing) {
      if (onMenu)
        switch (onMenu) {
          case 1:
            if (key == 'A') {
              /// Open door
            } else if (key == 'B' && onMenu != 2) {
              onMenu = 2;
              setup();
            } else if (key == 'C' && onMenu != 3) {
              onMenu = 3;
              setup();
            } else if (key == 'D') {
              onMenu = 0;
              clearScreen();
              setup();
            }
            break;
          case 2:
            if (key == 'A') {
              /// Automatic
            } else if (key == 'B') {
              /// Locked
            } else if (key == 'D') {
              onMenu = 1;
              clearScreen();
              setup();
            }
            break;
          case 3:
            // Change password
            if (key == 'D') {
              onMenu = 1;
              clearScreen();
              setup();
            }
            break;
        }
      else if (key == '#') {
        u8x8log.print(s);
        u8x8log.print("\n");
        if (s == PASSWORD) {
          Serial.print("MATCHED\n");
          onMenu = 1;
          s = "";
          setup();
        }
      } else if (key == '*') {
        typing = false;
        s = "";
        setup();
      } else
        s += key;
    }
  }
}