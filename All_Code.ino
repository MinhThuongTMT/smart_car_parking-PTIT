#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ESP32Servo.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <WebServer.h>
#include <TimeLib.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

#define SS_PIN_1 5
#define RST_PIN_1 2
#define SS_PIN_2 15
#define RST_PIN_2 13
#define SERVO_PIN_4 4
#define SERVO_PIN_16 16

// Khai báo biến NTP
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 7 * 3600, 60000); // GMT+7, cập nhật mỗi phút

MFRC522 rfid1(SS_PIN_1, RST_PIN_1);
MFRC522 rfid2(SS_PIN_2, RST_PIN_2);
MFRC522::MIFARE_Key key;

LiquidCrystal_I2C lcd(0x27, 16, 2);
Servo servo4;
Servo servo16;

WebServer server(80);
String logData = "";

void initializeSystem();
void handleRFID(MFRC522 &rfid, Servo &servo, String gateName);
void displayMessage(const char *message);
void moveServo(Servo &servo, int position, int delayTime);
void handleRoot();
void openGate1();
void closeGate1();
void openGate2();
void closeGate2();
String getTimeStamp();

void setup() {
    Serial.begin(115200);
    server.on("/log", handleLog);
    lcd.begin();
    lcd.backlight();
    displayMessage("Starting...");
    delay(1000);
    
    
    WiFiManager wifiManager;
    displayMessage("Connecting WiFi...");
    Serial.println("Connecting to WiFi...");
    
    if (!wifiManager.autoConnect("ESP32-TMT")) {
        Serial.println("Failed to connect. Restarting...");
        displayMessage("WiFi Failed!");
        delay(2000);
        ESP.restart();
    }
    
    Serial.println("WiFi Connected");
    displayMessage("WiFi Connected!");
    delay(2000);


    timeClient.begin();  // Khởi động NTP
    timeClient.update(); // Lấy thời gian ban đầu

    initializeSystem();
    server.on("/", handleRoot);
    server.on("/open1", openGate1);
    server.on("/close1", closeGate1);
    server.on("/open2", openGate2);
    server.on("/close2", closeGate2);
    server.begin();
    Serial.println("WebServer Started.");
    displayMessage("System Ready");
}

void loop() {
    server.handleClient();

    // Kiểm tra và xử lý quẹt thẻ RFID
    handleRFID(rfid1, servo4, "Cổng vào", "Xe vào");  // Web hiển thị: Xe vào
    handleRFID(rfid2, servo16, "Cổng ra", "Xe ra");   // Web hiển thị: Xe ra

    timeClient.update();
}


void initializeSystem() {
    SPI.begin();
    rfid1.PCD_Init();
    rfid2.PCD_Init();
    
    servo4.attach(SERVO_PIN_4);
    servo16.attach(SERVO_PIN_16);
    
    for (byte i = 0; i < 6; i++) {
        key.keyByte[i] = 0xFF;
    }
    
    Serial.println("RFID System Initialized.");
    displayMessage("RFID Ready");
}

void handleRFID(MFRC522 &rfid, Servo &servo, String gateName, String action) {
    if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
        Serial.println("RFID Detected at " + gateName);
        String entryTime = getTimeStamp();

        // Cập nhật log cho WebServer
        logData += "<tr><td>" + gateName + "</td><td>" + action + "</td><td>" + entryTime + "</td></tr>";

        // Hiển thị trên LCD: MỞ CỔNG
        displayMessage("-- OPEN --");

        // Mở cổng
        moveServo(servo, 90, 5000);

        // Đóng cổng
        moveServo(servo, 0, 0);

        // Hiển thị trên LCD: ĐÓNG CỔNG
        displayMessage("-- CLOSE --");

        // Dừng thẻ RFID
        rfid.PICC_HaltA();
        rfid.PCD_StopCrypto1();
    }
}


void displayMessage(const char *message) {
    lcd.clear();
    int padding = (16 - strlen(message)) / 2;
    lcd.setCursor(padding, 0);
    lcd.print(message);
}

void moveServo(Servo &servo, int position, int delayTime) {
    servo.write(position);
    if (delayTime > 0) {
        delay(delayTime);
    }
}

void handleRoot() {
    String html = R"rawliteral(
    <!DOCTYPE html>
    <html lang="en">
    <head>
        <meta charset="UTF-8">
        <meta name="viewport" content="width=device-width, initial-scale=1.0">
        <title>SMART PARKING</title>
        <style>
            body {
                font-family: Arial, sans-serif;
                background-color: #f4f4f4;
                text-align: center;
                margin: 0;
                padding: 20px;
            }
            .container {
                max-width: 900px;
                background: white;
                padding: 30px;
                border-radius: 10px;
                box-shadow: 0px 0px 15px rgba(0, 0, 0, 0.2);
                margin: auto;
            }
            h2 {
                font-size: 28px;
                color: #333;
            }
            .button {
                display: inline-block;
                width: 160px;
                padding: 15px;
                margin: 15px;
                font-size: 18px;
                color: white;
                border: none;
                border-radius: 8px;
                cursor: pointer;
            }
            .open {
                background: #28a745; /* Xanh lá */
            }
            .open:hover {
                background: #218838;
            }
            .close {
                background: #dc3545; /* Đỏ */
            }
            .close:hover {
                background: #c82333;
            }
            table {
                width: 100%;
                border-collapse: collapse;
                margin-top: 20px;
                font-size: 18px;
            }
            th, td {
                padding: 12px;
                border: 1px solid #ddd;
                text-align: center;
            }
            th {
                background: #007BFF;
                color: white;
                font-size: 20px;
            }
            .log-container {
                height: 300px;
                overflow-y: auto;
                border: 1px solid #ddd;
                padding: 10px;
                border-radius: 5px;
                background: #fff;
            }
        </style>
        <script>
            function sendRequest(endpoint) {
                fetch(endpoint).then(response => response.text()).then(data => {
                    console.log(data);
                    updateLog();
                });
            }
            function updateLog() {
                fetch('/log').then(response => response.text()).then(data => {
                    document.getElementById("logTable").innerHTML = data;
                });
            }
            setInterval(updateLog, 5000);
        </script>
    </head>
    <body>
        <div class="container">
            <h2>SMART PARKING PTIT</h2>
            <button class="button open" onclick="sendRequest('/open1')">MỞ CỔNG 1</button>
            <button class="button close" onclick="sendRequest('/close1')">ĐÓNG CỔNG 1</button><br>
            <button class="button open" onclick="sendRequest('/open2')">MỞ CỔNG 2</button>
            <button class="button close" onclick="sendRequest('/close2')">ĐÓNG CỔNG 2</button>
            <h3>DANH SÁCH CÁC XE</h3>
            <div class="log-container">
                <table>
                    <thead>
                        <tr>
                            <th>CỔNG</th>
                            <th>HÀNH ĐỘNG</th>
                            <th>THỜI GIAN</th>
                        </tr>
                    </thead>
                    <tbody id="logTable"></tbody>
                </table>
            </div>
        </div>
    </body>
    </html>
    )rawliteral";

    server.send(200, "text/html", html);
}



void handleLog() {
    server.send(200, "text/html", logData);
}

void openGate1() {
    moveServo(servo4, 90, 5000);
    moveServo(servo4, 0, 0);
    logData += "Gate 1 Opened - " + getTimeStamp() + "<br>";
    server.send(200, "text/plain", "Gate 1 Opened");
}

void closeGate1() {
    moveServo(servo4, 0, 0);
    logData += "Gate 1 Closed - " + getTimeStamp() + "<br>";
    server.send(200, "text/plain", "Gate 1 Closed");
}

void openGate2() {
    moveServo(servo16, 90, 5000);
    moveServo(servo16, 0, 0);
    logData += "Gate 2 Opened - " + getTimeStamp() + "<br>";
    server.send(200, "text/plain", "Gate 2 Opened");
}

void closeGate2() {
    moveServo(servo16, 0, 0);
    logData += "Gate 2 Closed - " + getTimeStamp() + "<br>";
    server.send(200, "text/plain", "Gate 2 Closed");
}

String getTimeStamp() {
    unsigned long epochTime = timeClient.getEpochTime();
    struct tm *timeInfo;
    time_t rawTime = epochTime;
    timeInfo = localtime(&rawTime);

    char buffer[30];
    strftime(buffer, sizeof(buffer), "%H:%M:%S - %d/%m/%Y", timeInfo);
    return String(buffer);
}

