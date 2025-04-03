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
#include "HX711.h"

// RFID1
#define SS_PIN_1 5      // Chân SS cho RFID 1 (Cổng vào) == SDA
#define RST_PIN_1 2     // Chân RST cho RFID 1
// RFID2
#define SS_PIN_2 15     // Chân SS cho RFID 2 (Cổng ra) == SDA
#define RST_PIN_2 3     // Chân RST cho RFID 2
// SERVO
#define SERVO_PIN_4 12   // Chân servo cổng 1
#define SERVO_PIN_16 13  // Chân servo cổng 2
// LOADCELL
#define LOADCELL_DOUT_PIN 32   // Chân DATA của HX711 thứ nhất
#define LOADCELL_SCK_PIN 33    // Chân SCK của HX711 thứ nhất
#define LOADCELL_DOUT_PIN_2 26 // Chân DATA của HX711 thứ hai
#define LOADCELL_SCK_PIN_2 27  // Chân SCK của HX711 thứ hai
#define LOADCELL_DOUT_PIN_3 4  // Chân DATA của HX711 thứ ba (D4 - GPIO4)
#define LOADCELL_SCK_PIN_3 16  // Chân SCK của HX711 thứ ba (D16 - GPIO16)
// Cảm biến vật cản hồng ngoại
#define IR_SENSOR_PIN 35
// Cảm biến khoảng cách Ultrasonic
#define TRIG_PIN 17
#define ECHO_PIN 14
// LED
#define LED_PIN 25  

// Khai báo đối tượng
HX711 scale;       // Cảm biến trọng lượng 1
HX711 scale2;      // Cảm biến trọng lượng 2
HX711 scale3;      // Cảm biến trọng lượng 3
float calibration_factor = 417.5; // Hệ số hiệu chuẩn
float calibration_factor3 = -417.5; // Hệ số hiệu chuẩn

// Khai báo biến NTP
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 7 * 3600, 60000); // GMT+7

MFRC522 rfid1(SS_PIN_1, RST_PIN_1); // Module RFID cổng vào
MFRC522 rfid2(SS_PIN_2, RST_PIN_2); // Module RFID cổng ra
MFRC522::MIFARE_Key key;

LiquidCrystal_I2C lcd(0x27, 16, 2);
Servo servo4;   // Servo cổng 1
Servo servo16;  // Servo cổng 2

String allowedUIDs[10]; // Danh sách UID của các thẻ đã quét tại cổng vào
int allowedCount = 0;   // Số lượng thẻ hiện tại trong danh sách

// Biến toàn cục để theo dõi thời gian và trạng thái LED
unsigned long ledTurnOnTime = 0; // Thời điểm bật LED
bool ledIsOn = false;            // Trạng thái LED

WebServer server(80);
String logData = "";

// Biến theo dõi trạng thái
int availableSpots = 3;  // Số vị trí còn trống ban đầu
int actualAvailableSpots = 3;  // Giả sử bãi có 4 chỗ trống ban đầu
bool wasOccupied1 = false; // Trạng thái trước đó của cảm biến 1
bool wasOccupied2 = false; // Trạng thái trước đó của cảm biến 2
bool wasOccupied3 = false; // Trạng thái trước đó của cảm biến 3

// Biến để lưu khoảng cách
float distance;

// Khai báo các hàm
void initializeSystem();
void handleRFID(MFRC522 &rfid, Servo &servo, String gateName, String action);
void displayMessage(const char *message);
void moveServo(Servo &servo, int position, int delayTime);
void handleRoot();
void handleLog();
void openGate1();
void closeGate1();
void openGate2();
void closeGate2();
String getTimeStamp();
void updateLCD();
void checkWeightSensors();
float measureDistance();  // Hàm đo khoảng cách
void checkForVehicle();   // Hàm kiểm tra xe và điều khiển LED

void setup() {
    Serial.begin(115200);
    lcd.begin();
    lcd.backlight();
    displayMessage("Starting...");
    delay(1000);
    // Kết nối WiFi
    WiFi.mode(WIFI_STA);  // Chế độ Station Mode
    WiFiManager wifiManager;
    wifiManager.setTimeout(30); // Giới hạn thời gian kết nối
    wifiManager.resetSettings(); // Xóa WiFi cũ để thử lại
    displayMessage("Connecting WiFi...");
    Serial.println("Connecting to WiFi...");

    if (!wifiManager.autoConnect("ESP32-TMT", "12345678")) {
       Serial.println("Failed to connect. Restarting...");
       displayMessage("WiFi Failed!");
       delay(2000);
       ESP.restart();
    }

    Serial.println("WiFi Connected");
    displayMessage("WiFi Connected!");

    // Lấy và hiển thị địa chỉ IP
    String ip = WiFi.localIP().toString();
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("IP Address:");
    lcd.setCursor(0, 1);
    lcd.print(ip);
    delay(5000); // Hiển thị trong 5 giây

    // Khởi động NTP
    timeClient.begin();
    timeClient.update();

    // Khởi tạo hệ thống
    initializeSystem();

    // Cấu hình WebServer
    server.on("/", handleRoot);
    
    server.on("/log", handleLog);
    
    server.on("/availableSpots", handleAvailableSpots); 

    server.on("/open1", openGate1);
    server.on("/close1", closeGate1);
    
    server.on("/open2", openGate2);
    server.on("/close2", closeGate2);
    
    server.begin();
    Serial.println("WebServer Started.");
    displayMessage("System Ready");

    // Khởi tạo cảm biến trọng lượng 1
    scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
    Serial.println("Đang hiệu chuẩn cân thứ nhất...");
    scale.tare();
    delay(1000);
    scale.set_scale(calibration_factor);
    Serial.println("Hiệu chuẩn cảm biến thứ nhất hoàn tất!");

    // Khởi tạo cảm biến trọng lượng 2
    scale2.begin(LOADCELL_DOUT_PIN_2, LOADCELL_SCK_PIN_2);
    Serial.println("Đang hiệu chuẩn cân thứ hai...");
    scale2.tare();
    delay(1000);
    scale2.set_scale(calibration_factor);
    Serial.println("Hiệu chuẩn cảm biến thứ hai hoàn tất!");

    // Khởi tạo cảm biến trọng lượng 3
    scale3.begin(LOADCELL_DOUT_PIN_3, LOADCELL_SCK_PIN_3);
    Serial.println("Đang hiệu chuẩn cân thứ ba...");
    scale3.tare();
    delay(1000);
    scale3.set_scale(calibration_factor3);
    Serial.println("Hiệu chuẩn cảm biến thứ ba hoàn tất!");
    Serial.println("Tất cả các cân đã sẵn sàng để đo!");

    // Cấu hình các chân cảm biến và LED
    pinMode(IR_SENSOR_PIN, INPUT);  // Cảm biến hồng ngoại
    pinMode(TRIG_PIN, OUTPUT);      // TRIG của Ultrasonic
    pinMode(ECHO_PIN, INPUT);       // ECHO của Ultrasonic
    pinMode(LED_PIN, OUTPUT);       // LED

    updateLCD(); // Hiển thị số vị trí còn trống ban đầu
}

void loop() {
    server.handleClient();
    timeClient.update();

    // Gọi các hàm xử lý
    handleRFID(rfid1, servo4, "Cổng vào", "Xe vào");
    handleRFID(rfid2, servo16, "Cổng ra", "Xe ra");
    checkWeightSensors();
    checkForVehicle(); // Kiểm tra xe và điều khiển LED

    delay(500); // Giảm tần suất lặp để tránh nhiễu
}

void initializeSystem() {
    SPI.begin();
    rfid1.PCD_Init();
    rfid2.PCD_Init();

    servo4.attach(SERVO_PIN_4);
    servo16.attach(SERVO_PIN_16);

    for (byte i = 0; i < 6; i++) {
        key.keyByte[i] = 0xFF; // Khóa mặc định cho MFRC522
    }

    Serial.println("RFID System Initialized.");
    displayMessage("RFID Ready");

    // Đặt cả 2 servo về vị trí đóng khi khởi động
    servo4.write(85);  // Đóng cổng vào
    servo16.write(3);  // Đóng cổng ra
}

// Hàm để thêm UID vào danh sách
void addUID(String uid) {
    if (allowedCount < 10) {
        allowedUIDs[allowedCount] = uid;
        allowedCount++;
    }
}

// Hàm để kiểm tra xem UID có trong danh sách hay không
bool isUIDAllowed(String uid) {
    for (int i = 0; i < allowedCount; i++) {
        if (allowedUIDs[i] == uid) {
            return true;
        }
    }
    return false;
}

// Hàm để xóa UID khỏi danh sách
void removeUID(String uid) {
    for (int i = 0; i < allowedCount; i++) {
        if (allowedUIDs[i] == uid) {
            for (int j = i; j < allowedCount - 1; j++) {
                allowedUIDs[j] = allowedUIDs[j + 1];
            }
            allowedCount--;
            break;
        }
    }
}

void handleRFID(MFRC522 &rfid, Servo &servo, String gateName, String action) {
    if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
        String uid = "";
        for (byte i = 0; i < rfid.uid.size; i++) {
            uid += String(rfid.uid.uidByte[i], HEX);
        }
        uid.toUpperCase();
        Serial.println("UID: " + uid);

        String entryTime = getTimeStamp();

        if (gateName == "Cổng vào") {
            addUID(uid);
            Serial.println("Thẻ được thêm vào danh sách tại Cổng vào");

            logData += "<tr><td>" + gateName + "</td><td>" + action + "</td><td>" + entryTime + "</td></tr>";

            displayMessage("-- OPEN --"); // ngược chiều
            moveServo(servo, 5, 5000); // mở 
            moveServo(servo, 85, 0); // đóng
            displayMessage("-- CLOSE --");
        } 
        else if (gateName == "Cổng ra") {
            if (isUIDAllowed(uid)) {
                removeUID(uid);
                Serial.println("Thẻ hợp lệ, đã xóa khỏi danh sách");

                logData += "<tr><td>" + gateName + "</td><td>" + action + "</td><td>" + entryTime + "</td></tr>";

                displayMessage("-- OPEN --"); // cùng chiều 
                moveServo(servo, 85, 5000); // mở 85
                moveServo(servo, 3, 0); //  đóng 3
                displayMessage("-- CLOSE --");
            } else {
                Serial.println("!!!ERROR CARD!!!");
                displayMessage("!!!ERROR CARD!!!");
                delay(2000);
                updateLCD();
            }
        }

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

void updateLCD() {
    char buffer[16];
    sprintf(buffer, "Spots: %d", availableSpots);
    displayMessage(buffer);
}

void checkWeightSensors() {
    // Cảm biến 1
    if (scale.is_ready()) {
        float weight1 = scale.get_units(10); // Trung bình 10 lần đo
        if (abs(weight1) < 2) weight1 = 0; // Ngưỡng nhiễu

        Serial.print("Khối lượng cảm biến 1: ");
        Serial.print(weight1);
        Serial.println(" g");

        bool isOccupied1 = weight1 > 5;
        if (isOccupied1 && !wasOccupied1) {
            if (availableSpots > 0) {
                availableSpots--;
                updateLCD();
            }
        } else if (!isOccupied1 && wasOccupied1) {
            if (availableSpots < 4) {
                availableSpots++;
                updateLCD();
            }
        }
        wasOccupied1 = isOccupied1;
    } else {
        Serial.println("Cảm biến 1 chưa sẵn sàng!");
    }

    // Cảm biến 2
    if (scale2.is_ready()) {
        float weight2 = scale2.get_units(10);
        if (abs(weight2) < 2) weight2 = 0;

        Serial.print("Khối lượng cảm biến 2: ");
        Serial.print(weight2);
        Serial.println(" g");

        bool isOccupied2 = weight2 > 5;
        if (isOccupied2 && !wasOccupied2) {
            if (availableSpots > 0) {
                availableSpots--;
                updateLCD();
            }
        } else if (!isOccupied2 && wasOccupied2) {
            if (availableSpots < 4) {
                availableSpots++;
                updateLCD();
            }
        }
        wasOccupied2 = isOccupied2;
    } else {
        Serial.println("Cảm biến 2 chưa sẵn sàng!");
    }

    // Cảm biến 3
    if (scale3.is_ready()) {
        float weight3 = scale3.get_units(10);
        if (abs(weight3) < 2) weight3 = 0;

        Serial.print("Khối lượng cảm biến 3: ");
        Serial.print(weight3);
        Serial.println(" g");

        bool isOccupied3 = weight3 > 6; 
        if (isOccupied3 && !wasOccupied3) {
            if (availableSpots > 0) {
                availableSpots--;
                updateLCD();
            }
        } else if (!isOccupied3 && wasOccupied3) {
            if (availableSpots < 4) {
                availableSpots++;
                updateLCD();
            }
        }
        wasOccupied3 = isOccupied3;
    } else {
        Serial.println("Cảm biến 3 chưa sẵn sàng!");
    }
}

// Hàm đo khoảng cách từ cảm biến Ultrasonic
float measureDistance() {
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);
    
    long duration = pulseIn(ECHO_PIN, HIGH);
    float distance = duration * 0.034 / 2; // Tính khoảng cách (cm)
    return distance;
}

// Hàm kiểm tra xe và điều khiển LED
void checkForVehicle() {
    int irState = digitalRead(IR_SENSOR_PIN); // Đọc trạng thái cảm biến hồng ngoại
    distance = measureDistance();             // Đo khoảng cách từ cảm biến siêu âm
    
    // Kiểm tra điều kiện: Có vật cản và khoảng cách từ 4m đến 10m (400cm đến 1000cm)
    bool isVehicleDetected = (irState == LOW && distance >= 400 && distance <= 1000);
    
    // Kiểm tra cả ba cảm biến trọng lượng
    bool allScalesOccupied = wasOccupied1 && wasOccupied3;
    
    // Trường hợp phát hiện xe và cần bật LED
    if (isVehicleDetected && allScalesOccupied && !ledIsOn) {
        digitalWrite(LED_PIN, HIGH); // Bật LED
        ledIsOn = true;              // Cập nhật trạng thái LED
        Serial.println("Phát hiện xe và cả ba cảm biến trọng lượng có vật! LED sáng liên tục.");
        
        // Giảm số chỗ trống nếu còn chỗ
        if (availableSpots > 0) {
            availableSpots--;
            updateLCD(); // Cập nhật hiển thị số chỗ trống trên LCD
        }
    }
    
    // Lưu giá trị trước đó của số chỗ trống
    static int previousSpots = availableSpots;
    
    // Kiểm tra nếu số chỗ trống tăng lên (có xe rời bãi)
    if (availableSpots > previousSpots) {    
        // Tắt LED nếu đang sáng
        if (ledIsOn) {
            digitalWrite(LED_PIN, LOW); // Tắt LED
            ledIsOn = false;            // Cập nhật trạng thái LED
            Serial.println("Số chỗ trống tăng, LED tắt.");
        }
        updateLCD(); // Cập nhật hiển thị trên LCD
    }
    
    // Cập nhật giá trị previousSpots cho lần kiểm tra tiếp theo
    previousSpots = availableSpots;
}
void handleAvailableSpots() {
    server.send(200, "text/plain", String(availableSpots));
}

void handleRoot() {
    String html = R"rawliteral(
    <!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>NHOM 1 - PTIT</title>
    <link rel="stylesheet" href="https://cdnjs.cloudflare.com/ajax/libs/font-awesome/5.15.4/css/all.min.css">
    <style>
        body {


            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            background-image: url("https://res.cloudinary.com/dkqutpweh/image/upload/v1743181539/background_kckgfq.jpg");
            background-size: cover;
            background-position: center;
            text-align: center;
            margin: 0;
            padding: 20px;
            color: #333;
        }
        .container {
            max-width: 900px;
            background: white;
            padding: 30px;
            border-radius: 10px;
            box-shadow: 0px 0px 15px rgba(0, 0, 0, 0.2);
            margin: auto;
        }
        .header {
            display: flex;
            justify-content: space-between;
            align-items: center;
            margin-bottom: 20px;
        }
        .logo img {
            height: 50px;
        }
        .notification {
            font-size: 24px;
            color: #007BFF;
            cursor: pointer;
        }
        h2 {
            margin: 0;
            font-size: 35px;
            color: hsl(213, 100%, 50%);
        }
        .spots-box {
            background-color: #2758b8;
            color: white;
            padding: 10px;
            border-radius: 10px;
            margin: 20px 0;
            font-size: 25px;
            font-weight: bold;
            box-shadow: 0px 4px 10px rgba(0, 0, 0, 0.1);
        }
        .switch-row {
            display: flex;
            justify-content: center;
            align-items: center;
            margin: 20px 5;
        }
        .switch-container {
            display: flex;
            justify-content: center;
            align-items: center;
            margin: 20px 0;
        }
        .switch {
            position: relative;
            display: inline-block;
            width: 60px;
            height: 34px;
            margin: 0 10px;
        }
        .switch input {
            opacity: 0;
            width: 0;
            height: 0;
        }
        .slider {
            position: absolute;
            cursor: pointer;
            top: 0;
            left: 0;
            right: 0;
            bottom: 0;
            background-color: #ccc;
            transition: .4s;
            border-radius: 34px;
        }
        .slider:before {
            position: absolute;
            content: "";
            height: 26px;
            width: 26px;
            left: 4px;
            bottom: 4px;
            background-color: white;
            transition: .4s;
            border-radius: 50%;
        }
        input:checked + .slider {
            background-color: #003661;
        }
        input:checked + .slider:before {
            transform: translateX(26px);
        }
        .gate-label {
            font-size: 20px;
            color: hsl(0, 0%, 0%);
            margin: 0 10px;
            font-weight: bold;
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
            position: sticky;
            top: 0;
            z-index: 1;
        }
        .log-container {
            height: 300px;
            overflow-y: auto;
            border: 1px solid #ddd;
            padding: 10px;
            border-radius: 5px;
            background: #fff;
        }
        .error-message {
            color: red;
            font-size: 16px;
            margin-top: 10px;
        }
    </style>
    <script>
        function toggleGate(gateNumber, isOpen) {
            const endpoint = isOpen ? `/open${gateNumber}` : `/close${gateNumber}`;
            fetch(endpoint)
                .then(response => {
                    if (!response.ok) {
                        throw new Error('Network response was not ok');
                    }
                    return response.text();
                })
                .then(data => {
                    console.log(data);
                    updateLog();
                    updateAvailableSpots();
                })
                .catch(error => {
                    console.error('Error:', error);
                    document.getElementById('errorMessage').innerText = 'Có lỗi xảy ra khi điều khiển cổng. Vui lòng thử lại sau.';
                });
        }
        function updateLog() {
            fetch('/log')
                .then(response => response.text())
                .then(data => {
                    document.getElementById("logTable").innerHTML = data;
                })
                .catch(error => {
                    console.error('Error fetching log:', error);
                });
        }
        function updateAvailableSpots() {
            fetch('/availableSpots')
                .then(response => {
                    if (!response.ok) {
                        throw new Error('Network response was not ok');
                    }
                    return response.text();
                })
                .then(data => {
                    document.getElementById("availableSpots").innerText = data;
                    document.getElementById('errorMessage').innerText = '';
                })
                .catch(error => {
                    console.error('Error fetching available spots:', error);
                    document.getElementById('availableSpots').innerText = 'N/A';
                    document.getElementById('errorMessage').innerText = 'Không thể tải số vị trí còn trống. Vui lòng thử lại sau.';
                });
        }
        setInterval(function() {
            updateLog();
            updateAvailableSpots();
        }, 5000);
        window.onload = function() {
            updateAvailableSpots();
        };
        document.querySelector('.notification').addEventListener('click', function() {
            fetch('/log')
                .then(response => response.text())
                .then(data => {
                    alert('Danh sách sự kiện gần đây:\n' + data);
                })
                .catch(error => {
                    console.error('Error fetching log:', error);
                    alert('Không thể tải thông báo. Vui lòng thử lại sau.');
                });
        });
    </script>
</head>
<body>
    <div class="container">
        <header class="header">
            <div class="logo">
                <img src="https://res.cloudinary.com/dkqutpweh/image/upload/v1743181208/PTIT-01_wweubu.png" alt="Logo Trường">
            </div>
            <h2>SMART CAR PARKING</h2>
            <div class="notification">
                <i class="fa fa-bell"></i>
            </div>
        </header>
        <div class="spots-box">
            Số vị trí còn trống: <span id="availableSpots">Loading...</span>
        </div>
        <div class="switch-row">
            <div class="switch-container">
                <label class="gate-label">Cổng vào</label>
                <label class="switch">
                    <input type="checkbox" onchange="toggleGate(1, this.checked)">
                    <span class="slider"></span>
                </label>
            </div>
            <div class="switch-container">
                <label class="gate-label">Cổng ra</label>
                <label class="switch">
                    <input type="checkbox" onchange="toggleGate(2, this.checked)">
                    <span class="slider"></span>
                </label>
            </div>
        </div>
        <div id="errorMessage" class="error-message"></div>
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
    moveServo(servo4, 5, 0);
    logData += "<tr><td>Cổng vào</td><td>Xe vào</td><td>" + getTimeStamp() + "</td></tr>";
    server.send(200, "text/plain", "Gate 1 Opened");
}

void closeGate1() {
    moveServo(servo4, 85, 0);
    logData += "<tr><td>Cổng vào</td><td>Đóng cổng</td><td>" + getTimeStamp() + "</td></tr>";
    server.send(200, "text/plain", "Gate 1 Closed");
}

void openGate2() {
    moveServo(servo16, 85, 0);
    logData += "<tr><td>Cổng ra</td><td>Xe ra</td><td>" + getTimeStamp() + "</td></tr>";
    server.send(200, "text/plain", "Gate 2 Opened");
}

void closeGate2() {
    moveServo(servo16, 3, 0);
    logData += "<tr><td>Cổng ra</td><td>Đóng cổng</td><td>" + getTimeStamp() + "</td></tr>";
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
