#include <Keypad.h>
#include <Arduino.h>
#include <U8x8lib.h>
#include <ESP32Servo.h>

String PASSWORD = "1234";

#define buzzer_PIN 18  // Chân GPIO nối với buzzer

// SR04
#define SR04_TRIG_PIN 23  // Vị trí chân GPIO của ESP32 được nối với Trig của SR04
#define SR04_ECHO_PIN 5   // Vị trí chân GPIO của ESP32 được nối với Echo của SR04

// Setup Servo
#define servoPin 4

// Setup OLED Screen
#define OLED_SCL_PIN 22
#define OLED_SDA_PIN 21
U8X8_SH1106_128X64_NONAME_HW_I2C u8x8(/* reset=*/U8X8_PIN_NONE, OLED_SCL_PIN, OLED_SDA_PIN);
#define U8LOG_WIDTH 16
#define U8LOG_HEIGHT 8
uint8_t u8log_buffer[U8LOG_WIDTH * U8LOG_HEIGHT];
U8X8LOG u8x8log;

Servo myservo;
bool typing, enableAuto = false;
String s, tmp;  // Lưu mật khẩu
unsigned long prevMillis = 0;

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
        u8x8log.print("D. Back\n");
        u8x8log.print("New password:\n");
      }
  }
}

// Open buzzer
void beep() {
  digitalWrite(buzzer_PIN, HIGH);
  delay(100);
  digitalWrite(buzzer_PIN, LOW);
}

// Lấy khoảng cách đo từ cảm biến
int GetDistance() {
  digitalWrite(SR04_TRIG_PIN, LOW);  // Đưa chân Trig xuống mức thấp trong 2uS
  delayMicroseconds(2);
  digitalWrite(SR04_TRIG_PIN, HIGH);  // Gửi luồng siêu âm kéo dài 10uS
  delayMicroseconds(10);
  digitalWrite(SR04_TRIG_PIN, LOW);                          // Tắt luồng siêu âm
  unsigned int microseconds = pulseIn(SR04_ECHO_PIN, HIGH);  // Đợi cho tới khi có phản hồi
  return microseconds / 58;                                  // Từ thời gian hành trình tính toán khoảng cách
}

unsigned int distance;  // Khoảng cách từ vật thể tới cảm biến siêu âm

// Automatic Door
void autoDoor() {
  distance = GetDistance();
  myservo.write(0);
  if (distance <= 7 && distance > 0) {
    Serial.print(distance);
    myservo.write(180);
    delay(2000);
  }
}

// Open Door
void openDoor() {
  int timeT = 0;
  unsigned int current = millis();
  int pos = 0;
  for (pos = 0; pos <= 90; ++pos) {
    myservo.write(pos);
    delay(15);
  }
  while (timeT < 5000) {
    unsigned int t = millis();
    timeT = t - current;
    distance = GetDistance();
    if (distance <= 7 && distance > 0) {
      myservo.write(0);
    }
  }
  myservo.write(0);
}


int changePasswordPhase = 0;                        // Xác định phase đổi mật khẩu
void handleTyping(char key, bool onChangePassword)  // Hàm xử lý ký tự nhập vào
{
  if (key == '#')  // Submit
  {
    u8x8log.print("\n");
    if (!onChangePassword)  // Nếu đang không trong chế độ đổi mk
    {
      if (s == PASSWORD)  // Match password
      {
        u8x8log.print("SUCCESS\n");
        Serial.print("SUCCESS\n");
        onMenu = 1;  // Mở menu 1
        s = "";
        setup();  // render lại menu
      } else      // Not match
      {
        u8x8log.print("WRONG PASSWORD\n");
        Serial.print("FAILED\n");
        s = "";
        setup();
      }
    } else {
      if (changePasswordPhase == 0)  // phase 0: nhập mật khẩu mới
      {
        changePasswordPhase++;
        u8x8log.print("Enter again:\n");
        tmp = s;
        s = "";
      } else if (changePasswordPhase == 1)  // phase 1: xác nhận mật khẩu mới
      {
        if (s == tmp) {
          PASSWORD = s;
          u8x8log.print("SUCCESS");
          s = "";
          tmp = "";
          changePasswordPhase = 0;
          onMenu = 0;
          setup();
        } else {
          changePasswordPhase = 0;
          u8x8log.print("NOT MATCHED");
          s = "";
          tmp = "";
          onMenu = 3;
          setup();
        }
      }
    }
  } else if (key == '*')  // clear input
  {
    typing = false;
    s = "";
    setup();
  } else  // Ký tự bình thường
  {
    u8x8log.print("*");
    s += key;
  }
}

void setup() {
  // SR04 phát xung âm khi có tín hiệu điều khiển từ chân Trig, và nhận siêu âm
  // phản hồi thì báo về qua Echo. Vì vậy Trig là chân OUTPUT, còn Echo là chân INPUT
  pinMode(SR04_TRIG_PIN, OUTPUT);
  pinMode(SR04_ECHO_PIN, INPUT);
  pinMode(buzzer_PIN, OUTPUT);

  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);
  myservo.setPeriodHertz(50);  // standard 50 hz servo
  myservo.attach(servoPin, 1000, 2000);

  Serial.begin(115200);
  Serial.println("Serial connected");

  u8x8.begin();
  u8x8.setFont(u8x8_font_chroma48medium8_r);
  u8x8log.begin(u8x8, U8LOG_WIDTH, U8LOG_HEIGHT, u8log_buffer);
  u8x8log.setRedrawMode(1);
  if (onMenu)  // Render menu
    showMenu();
  else  // Render nhập mật khẩu
    showTyping();
}

void loop() {
  unsigned long curMillis = millis();  // Lưu giá trị hiện tại của Timer
  char key = keypad.getKey();

  if (key != NO_KEY)  // Nếu người dùng ấn phím
  {
    Serial.println(key);
    beep();

    if (!typing)  // Bắt đầu nhập
    {
      typing = true;
      s = "";  // Reset xâu
    }
    if (typing)  // Đang nhập
    {
      if (onMenu)  // Đang ở menu
      {
        switch (onMenu) {
          case 1:  // Menu 1
            if (key == 'A') {
              /// Open door
              openDoor();
              onMenu = 0;
              clearScreen();
              setup();
            } else if (key == 'B' && onMenu != 2)  // Chuyển menu 2
            {
              onMenu = 2;
              setup();
            } else if (key == 'C' && onMenu != 3)  // Chuyển menu 3
            {
              onMenu = 3;
              setup();
            } else if (key == 'D')  // Quay lại
            {
              onMenu = 0;
              clearScreen();
              setup();
            }
            break;
          case 2:            // Menu 2
            if (key == 'A')  // Chế độ cửa tự động
            {
              /// Automatic
              enableAuto = true;
              onMenu = 0;
              clearScreen();
              setup();
            } else if (key == 'B')  // Chế độ cần mật khẩu
            {
              /// Locked
              enableAuto = false;
              onMenu = 0;
              clearScreen();
              setup();
            } else if (key == 'D')  // Quay lại
            {
              onMenu = 1;
              clearScreen();
              setup();
            }
            break;
          case 3:            // Menu 3
            if (key == 'D')  // Quay lại
            {
              onMenu = 1;
              clearScreen();
              setup();
            } else {
              handleTyping(key, true);  // Xử lý ký tự nhập
              break;
            }
        }
      } else
        handleTyping(key, false);
    }
  } else if (enableAuto && ((curMillis - prevMillis) >= 1000)) {
    unsigned long t = curMillis - prevMillis;
    Serial.print("Period: ");
    Serial.println(t);
    prevMillis = curMillis;
    autoDoor();
  }
}