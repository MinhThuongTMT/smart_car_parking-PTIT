#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ESP32Servo.h>

#define SS_PIN_1 5
#define RST_PIN_1 2
#define SS_PIN_2 15
#define RST_PIN_2 13
#define SERVO_PIN_4 4
#define SERVO_PIN_16 16
#define CAMBIEN 27
#define TRIG_PIN 12
#define ECHO_PIN 14

long duration;
float distanceCm;
bool giatri;
int giatriTruoc = 0;
int soLuongXe = 0; // Biến đếm số lượng xe

MFRC522 rfid1(SS_PIN_1, RST_PIN_1);
MFRC522 rfid2(SS_PIN_2, RST_PIN_2);
MFRC522::MIFARE_Key key;

LiquidCrystal_I2C lcd(0x27, 16, 2);
Servo servo4;
Servo servo16;

void printHex(byte *buffer, byte bufferSize);
void printDec(byte *buffer, byte bufferSize);

void setup() {
    Serial.begin(115200);
    SPI.begin();
    rfid1.PCD_Init();
    rfid2.PCD_Init();
    lcd.begin();
    lcd.backlight();

    servo4.attach(SERVO_PIN_4);
    servo16.attach(SERVO_PIN_16);

    pinMode(CAMBIEN, INPUT);
    pinMode(TRIG_PIN, OUTPUT);
    pinMode(ECHO_PIN, INPUT);

    for (byte i = 0; i < 6; i++) {
        key.keyByte[i] = 0xFF;
    }
    
    Serial.println(F("RFID System Initialized."));
}

void loop() {
  
    if (rfid1.PICC_IsNewCardPresent() && rfid1.PICC_ReadCardSerial()) {
        Serial.println(F("RFID 1 Detected."));
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(F("-- OPEN --"));
        servo4.write(90);
        delay(5000);
        servo4.write(0);
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(F("-- CLOSED --"));
        rfid1.PICC_HaltA();
        rfid1.PCD_StopCrypto1();
    }
    
    if (rfid2.PICC_IsNewCardPresent() && rfid2.PICC_ReadCardSerial()) {
        Serial.println(F("RFID 2 Detected."));
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(F("-- OPEN --"));
        servo16.write(90);
        delay(5000);
        servo16.write(0);
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(F("-- CLOSED --"));
        rfid2.PICC_HaltA();
        rfid2.PCD_StopCrypto1();
    }

    // Phát tín hiệu siêu âm
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);

    // Đọc tín hiệu phản hồi
    duration = pulseIn(ECHO_PIN, HIGH);
    distanceCm = duration * 0.034 / 2;  // Tính khoảng cách (cm)

    giatri = digitalRead(CAMBIEN) == LOW;
  
   if (giatri&&giatriTruoc==0 && ( distanceCm >= 2.0 && distanceCm <= 10.0)) // Khi trạng thái chuyển từ có vật cản sang không có vật cản
   {
    soLuongXe++; // Tăng số lượng xe
    Serial.print("Xe vừa đi qua. Tổng số lượng xe: ");
    Serial.println(soLuongXe);

    // Cập nhật LCD
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("So xe: ");
    lcd.setCursor(7, 0);
    lcd.print(soLuongXe);
    delay(500);
  }

  giatriTruoc = giatri; 
  Serial.print("Giá trị cảm biến là: ");
  Serial.println(giatri);
  Serial.println("   ");
  Serial.print("Distance (cm): ");
  Serial.println(distanceCm);
  
  delay(500); // Chờ 500ms trước khi lặp lại
}

void printHex(byte *buffer, byte bufferSize) {
    for (byte i = 0; i < bufferSize; i++) {
        Serial.print(buffer[i] < 0x10 ? " 0" : " ");
        Serial.print(buffer[i], HEX);
    }
}

void printDec(byte *buffer, byte bufferSize) {
    for (byte i = 0; i < bufferSize; i++) {
        Serial.print(buffer[i] < 0x10 ? " 0" : " ");
        Serial.print(buffer[i], DEC);
    }
}
