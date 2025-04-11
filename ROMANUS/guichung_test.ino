#include <SparkFun_VL53L1X.h>

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <Keypad.h>
#include <SoftwareSerial.h>

SoftwareSerial mySerial(4, 5);  //RX, TX
// D4 RX - PA9
// D5 TX - PA10
// (Send and Receive)

// 📌 Thông tin WiFi
const char* ssid = "NHA 35 TANG 2";
const char* password = "minhha368";

// 📌 Twilio API thông tin
const char* twilio_account_sid = "ACee0e83fb8290876077350f587cf3465e";
const char* twilio_auth_token = "6c17105c7cf79002ac07efac47ac60df";
const char* twilio_phone_number = "+18147872751";  // Số Twilio
const char* owner_phone_number = "+84364031645";   // Số điện thoại nhận tin nhắn

// 📌 Nội dung tin nhắn
static const char* message = "Sent from my ESP32";


// 📌 Mật khẩu & OTP
String password1 = "1234";
String password2 = "5678";
//String otpCode = "";
String inputPassword = "";
bool firstLayerPassed = false;
bool waitingForOTP = false;
unsigned long lastActionTime = 0;
const unsigned long resetTime = 30000;  // 30 giây

// 📌 Hàm mã hóa Base64 (cho Authentication)
String base64_encode(const String& input) {
  static const char base64_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  String encoded;
  int val = 0, valb = -6;
  for (size_t i = 0; i < input.length(); i++) {
    char c = input[i];
    val = (val << 8) + c;
    valb += 8;
    while (valb >= 0) {
      encoded += base64_table[(val >> valb) & 0x3F];
      valb -= 6;
    }
  }
  if (valb > -6) encoded += base64_table[((val << 8) >> (valb + 8)) & 0x3F];
  while (encoded.length() % 4) encoded += '=';
  return encoded;
}

// 📌 Gửi tin nhắn SMS qua Twilio
void sendSMS(const String& otpCode) {
  //Serial.print("OTP nhận được từ STM32: ");
  //Serial.println(otpCode);
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi chưa kết nối! Kiểm tra lại.");
    return;
  }

  Serial.println("🔄 Đang gửi tin nhắn SMS...");

  // Thiết lập kết nối bảo mật
  WiFiClientSecure client;
  client.setInsecure();  // Không kiểm tra SSL (có thể thay bằng setCACert())

  HTTPClient http;
  String url = "https://api.twilio.com/2010-04-01/Accounts/" + String(twilio_account_sid) + "/Messages.json";
  String postData = "To=" + String(owner_phone_number) + "&From=" + String(twilio_phone_number) + "&Body=Ma mo khoa: " + otpCode;

  // Mã hóa Authentication bằng Base64
  String authString = String(twilio_account_sid) + ":" + String(twilio_auth_token);
  String encodedAuth = base64_encode(authString);

  // Gửi HTTP POST request
  http.begin(client, url);
  http.addHeader("Authorization", "Basic " + encodedAuth);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");

  int httpResponseCode = http.POST(postData);

  // Kiểm tra phản hồi từ server
  if (httpResponseCode > 0) {
    String response = http.getString();
    Serial.println("✅ Tin nhắn đã gửi thành công!");
    Serial.println(response);
  } else {
    Serial.print("❌ Gửi thất bại. Mã lỗi: ");
    Serial.println(httpResponseCode);
  }

  http.end();
}

void setup() {
  Serial.begin(115200);
  mySerial.begin(115200);
  // Kết nối WiFi
  Serial.print("Đang kết nối WiFi...");
  WiFi.begin(ssid, password);
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(1000);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ WiFi đã kết nối!");
  } else {
    Serial.println("\n❌ Không thể kết nối WiFi!");
  }
}

void loop() {
  // Đọc dữ liệu từ STM32 qua UART
  if (mySerial.available() > 0) {
    String input = mySerial.readString();
    input.trim();  // Loại bỏ ký tự xuống dòng hoặc khoảng trắng thừa
    Serial.println("Nhận từ STM32: " + input);
    // Trích xuất OTP từ dữ liệu nhận được
    String otpCode = input.substring(20);
    otpCode.trim();  // Loại bỏ khoảng trắng thừa
    // Gửi SMS với OTP nhận được
    sendSMS(otpCode);
  }
  // Đọc dữ liệu từ Serial Monitor và gửi đến STM32
  if (Serial.available() > 0) {
    String input = Serial.readString();
    mySerial.println(input);
  }
}
