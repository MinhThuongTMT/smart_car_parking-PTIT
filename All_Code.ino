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

MFRC522 rfid1(SS_PIN_1, RST_PIN_1);
MFRC522 rfid2(SS_PIN_2, RST_PIN_2);
MFRC522::MIFARE_Key key;

LiquidCrystal_I2C lcd(0x27, 16, 2);
Servo servo4;
Servo servo16;

void printHex(byte *buffer, byte bufferSize);
void printDec(byte *buffer, byte bufferSize);

void setup() {
    Serial.begin(9600);
    SPI.begin();
    rfid1.PCD_Init();
    rfid2.PCD_Init();
    lcd.begin();
    lcd.backlight();

    servo4.attach(SERVO_PIN_4);
    servo16.attach(SERVO_PIN_16);

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
