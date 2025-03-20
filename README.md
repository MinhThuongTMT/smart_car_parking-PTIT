🚗 SMART CAR PARKING PTIT 🚗

📌 Xác định vị trí trống trong bãi đỗ xe sử dụng ESP32 và các cảm biến
![image](https://github.com/user-attachments/assets/f8b82753-964e-4cea-937d-c342ede6e6d2)


🔥 Giới thiệu

Dự án này xây dựng mô hình bãi đỗ xe thông minh sử dụng ESP32 và các cảm biến để xác định vị trí còn trống. Hệ thống có thể tự động mở/đóng cổng bằng RFID và servo, đồng thời kiểm tra xe bằng cảm biến hồng 

ngoại, siêu âm, và trọng lượng. Kết quả được hiển thị trên LCD và có thể truy cập từ Web Server.

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

![image](https://github.com/user-attachments/assets/3d5326fd-7a27-4932-8000-4ee54b4db5c4)


🚀 Cách sử dụng

1️⃣ Cài đặt thư viện cần thiết

Mở Arduino IDE, vào Library Manager, tìm và cài đặt các thư viện sau:

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

void handleRFID(MFRC522 &rfid, Servo &servo, String gateName, String action) {
    
    if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
        
        Serial.println("RFID Detected at " + gateName);
        
        String entryTime = getTimeStamp();

        logData += "<tr><td>" + gateName + "</td><td>" + action + "</td><td>" + entryTime + "</td></tr>";

        displayMessage("-- OPEN --");
        
        moveServo(servo, 90, 5000);

        moveServo(servo, 0, 0);
        
        displayMessage("-- CLOSE --");

        rfid.PICC_HaltA();
       
        rfid.PCD_StopCrypto1();
    
    }

}

⚖️ Kiểm tra cảm biến trọng lượng

void checkWeightSensors() {
    
    // Cảm biến 1
    
    if (scale.is_ready()) {
        
        float weight1 = scale.get_units(10); // Trung bình 10 lần đo
        
        if (abs(weight1) < 2) weight1 = 0; // Ngưỡng nhiễu
  
        Serial.print("Khối lượng cảm biến 1: ");
        
        Serial.print(weight1);
       
        Serial.println(" g");

        bool isOccupied1 = weight1 > 10;
        
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

        bool isOccupied2 = weight2 > 8;
       
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
}

📏 Đo khoảng cách bằng cảm biến HC-SR04

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
🚦 Xác định xe và điều khiển LED

void checkForVehicle() {
  
  int irState = digitalRead(IR_SENSOR_PIN); // Đọc trạng thái cảm biến hồng ngoại
  
  distance = measureDistance();             // Đo khoảng cách
  
  // Hiển thị thông tin lên terminal
  
  Serial.print("IR State: ");
 
  Serial.println(irState == LOW ? "Có vật cản" : "Không có vật cản");
 
  Serial.print("Khoảng cách: ");
 
  Serial.print(distance);
 
  Serial.println(" cm");
  
  // Kiểm tra điều kiện: Có vật cản và khoảng cách từ 4m đến 10m
  
  if (irState == LOW && distance >= 400 && distance <= 1000) {
  
    digitalWrite(LED_PIN, HIGH); // Bật LED
 
    Serial.println("Phát hiện xe trong khoảng 4m - 7m! LED sáng.");
 
  } else {
   
    digitalWrite(LED_PIN, LOW);  // Tắt LED
   
    Serial.println("Không phát hiện xe hoặc ngoài khoảng 4m - 7m. LED tắt.");
  
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

✅ Hệ thống nhận diện vị trí trống chính xác ...%.

✅ Giao diện web giúp giám sát dễ dàng từ xa.

✅ Cổng tự động mở/đóng ổn định và nhanh chóng.

🔧 Cần cải thiện khả năng lọc nhiễu của cảm biến siêu âm.


💡 Nếu bạn thích dự án này, hãy ⭐️ trên GitHub nhé!

📩 Liên hệ: tranminhthuong08082003@gmail.com | 📌 Tác giả: Nhom 1 - PTIT

🚀 Nhom 1 - PTIT! 🚀
