#include "HX711.h"

const int LOADCELL_DOUT_PIN = 5;
const int LOADCELL_SCK_PIN = 18;

HX711 scale;
float calibration_factor = 417.5; // Giá trị hiệu chuẩn, bạn cần tinh chỉnh thủ công nếu kết quả chưa chính xác

void setup() {
    Serial.begin(115200);
    scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);

    Serial.println("Đang hiệu chuẩn cân...");
    scale.tare(); // Cân bằng lại cảm biến, đảm bảo không có vật nào trên cân
    delay(1000);
    
    scale.set_scale(calibration_factor); // Cài đặt hệ số hiệu chuẩn
    Serial.println("Hiệu chuẩn hoàn tất!");

    Serial.println("Cân đã sẵn sàng để đo!");
}

void loop() {
    if (scale.is_ready()) {
        float reading = scale.get_units(10); // Lấy trung bình 10 lần đo để giảm nhiễu
        if (abs(reading) < 2) reading = 0; // Nếu giá trị đo được nhỏ hơn 2g, coi như không có vật nào

        Serial.print("Khối lượng đo được: ");
        Serial.print(reading);
        Serial.println(" g");
    } else {
        Serial.println("Cảm biến chưa sẵn sàng!");
    }

    delay(500); // Giảm tần suất đo để tránh nhiễu
}
