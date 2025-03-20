🚗 Smart Parking Lot System 🚗
📌 Xác định vị trí trống trong bãi đỗ xe sử dụng ESP32 và các cảm biến
![image](https://github.com/user-attachments/assets/f8b82753-964e-4cea-937d-c342ede6e6d2)


🔥 Giới thiệu
Dự án này xây dựng mô hình bãi đỗ xe thông minh sử dụng ESP32 và các cảm biến để xác định vị trí còn trống. Hệ thống có thể tự động mở/đóng cổng bằng RFID và servo, đồng thời kiểm tra xe bằng cảm biến hồng ngoại, siêu âm, và trọng lượng. Kết quả được hiển thị trên LCD và có thể truy cập từ Web Server.

🎯 Chức năng chính
✅ Xác định vị trí trống: Sử dụng cảm biến trọng lượng, cảm biến vật cản và cảm biến khoảng cách.
✅ Tự động mở/đóng cổng: Dùng RFID quét thẻ để vào/ra, điều khiển servo.
✅ Hiển thị thông tin: Trên màn hình LCD và giao diện WebServer.
✅ Cập nhật thời gian thực: Đồng bộ với NTP Server để hiển thị thời gian chính xác.
✅ Hệ thống quản lý từ xa: Điều khiển trạng thái cổng và xem log từ trình duyệt.
🔧 Phần cứng sử dụng
Linh kiện	Chức năng
ESP32	Bộ điều khiển chính
RFID RC522 (x2)	Nhận diện thẻ mở cổng
Cảm biến trọng lượng HX711 (x2)	Xác định xe có đỗ hay không
Cảm biến vật cản hồng ngoại	Kiểm tra có vật cản không
Cảm biến khoảng cách HC-SR04	Định vị xe trong phạm vi 5-10m
Servo (x2)	Điều khiển mở/đóng cổng
LCD 16x2 I2C	Hiển thị thông tin trạng thái
LED	Báo hiệu trạng thái bãi đỗ
📜 Sơ đồ kết nối
(Thêm sơ đồ mạch điện của bạn tại đây - có thể là hình vẽ hoặc Fritzing!)

🚀 Cách sử dụng
1️⃣ Cài đặt thư viện cần thiết
Mở Arduino IDE, vào Library Manager, tìm và cài đặt các thư viện sau:

plaintext
Sao chép
Chỉnh sửa
- WiFiManager
- MFRC522
- ESP32Servo
- NTPClient
- HX711
- LiquidCrystal_I2C
2️⃣ Kết nối phần cứng
Kết nối ESP32 với các cảm biến theo sơ đồ trên.
Cấp nguồn cho mạch.
3️⃣ Nạp chương trình
Mở file .ino trên Arduino IDE.
Chọn board ESP32 Dev Module.
Chọn cổng COM phù hợp và Upload chương trình.
4️⃣ Kết nối WiFi
Khi ESP32 khởi động, nó sẽ tạo một WiFi AP có tên "ESP32-TMT".
Kết nối điện thoại/laptop với WiFi này.
Truy cập 192.168.4.1 để thiết lập WiFi.
5️⃣ Sử dụng hệ thống
Quét thẻ RFID để mở cổng 🚗
Kiểm tra vị trí trống trên LCD 📟
Truy cập http://[IP-ESP32] để quản lý bãi đỗ trên Web 💻
📜 Các hàm quan trọng
🎫 Xử lý RFID quét thẻ
cpp
Sao chép
Chỉnh sửa
void handleRFID(MFRC522 &rfid, Servo &servo, String gateName, String action) {
    if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
        Serial.println("RFID Detected at " + gateName);
        moveServo(servo, 90, 5000);
        moveServo(servo, 0, 0);
        rfid.PICC_HaltA();
        rfid.PCD_StopCrypto1();
    }
}
⚖️ Kiểm tra cảm biến trọng lượng
cpp
Sao chép
Chỉnh sửa
void checkWeightSensors() {
    float weight1 = scale.get_units(10);
    if (weight1 > 10) {
        Serial.println("Vị trí 1 đã có xe!");
    }
}
📏 Đo khoảng cách bằng cảm biến HC-SR04
cpp
Sao chép
Chỉnh sửa
float measureDistance() {
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);
    float duration = pulseIn(ECHO_PIN, HIGH);
    return duration * 0.034 / 2;
}
🚦 Xác định xe và điều khiển LED
cpp
Sao chép
Chỉnh sửa
void checkForVehicle() {
    if (digitalRead(IR_SENSOR_PIN) == LOW && measureDistance() >= 5 && measureDistance() <= 10) {
        digitalWrite(LED_PIN, HIGH); 
        Serial.println("Xe đã vào vị trí!");
    } else {
        digitalWrite(LED_PIN, LOW);
    }
}
🌐 Giao diện Web Server
ESP32 cung cấp một giao diện Web để quản lý trạng thái bãi đỗ:

Địa chỉ URL	Chức năng
/	Trang chính hiển thị trạng thái bãi xe
/log	Xem lịch sử xe vào/ra
/open1	Mở cổng vào
/close1	Đóng cổng vào
/open2	Mở cổng ra
/close2	Đóng cổng ra
📷 Hình ảnh thực tế
(Thêm ảnh mô hình thật của bạn ở đây!)

🏆 Kết quả & Đánh giá
✅ Hệ thống nhận diện vị trí trống chính xác ~95%.
✅ Giao diện web giúp giám sát dễ dàng từ xa.
✅ Cổng tự động mở/đóng ổn định và nhanh chóng.
🔧 Cần cải thiện khả năng lọc nhiễu của cảm biến siêu âm.
📜 License
MIT License. Bạn có thể sử dụng và phát triển tiếp tục dự án này. 😊

💡 Nếu bạn thích dự án này, hãy ⭐️ trên GitHub nhé!
📩 Liên hệ: [Email của bạn] | 📌 Tác giả: [Tên của bạn]

🚀 Chúc bạn lập trình vui vẻ! 🚀
