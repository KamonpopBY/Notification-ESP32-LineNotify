#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <TridentTD_LineNotify.h>

/***************************************************
 * WiFi Configuration 
 ***************************************************/
const char* ssid = "";     // ชื่อ WiFi ของคุณ
const char* password = "";  // รหัสผ่าน WiFi ของคุณ
const char* LINE_TOKEN = ""; // ใส่ Line Notify Token

/***************************************************
 * GearMind API Configuration 
 ***************************************************/
const char* apiEndpoint = "http://gearmind.geworn.cloud/api/v1/chat";                      // GearMind API URL
const char* bearerToken = "sk-...";  // API Token ของคุณ

/***************************************************
 * Setup Function (รันครั้งเดียวตอนเริ่มทำงาน)
 ***************************************************/
void setup() {
  Serial.begin(115200);  // เริ่มการสื่อสาร Serial

  Serial.println(LINE.getVersion());

  // เชื่อมต่อ WiFi
  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {  // รอจนกว่า WiFi จะเชื่อมต่อสำเร็จ
    delay(1000);
    Serial.print(".");  // แสดง "." ทุก 1 วินาที ระหว่างรอ
  }
  Serial.println("\nConnected to WiFi!");  // แสดงข้อความเมื่อเชื่อมต่อสำเร็จ

  LINE.setToken(LINE_TOKEN);
}

/***************************************************
 * Loop Function (รันซ้ำไปเรื่อยๆ) 
 ***************************************************/
void loop() {
  // ตรวจสอบ WiFi Connection
  if (WiFi.status() == WL_CONNECTED) {

    // สร้าง HTTP Client
    WiFiClient client;
    HTTPClient http;

    // เริ่มต้น HTTP Request ไปยัง GearMind API
    if (http.begin(client, apiEndpoint)) {

      // ตั้งค่า Timeout สำหรับ API Request
      http.setTimeout(60000);  // รอ Response สูงสุด 60 วินาที (GearMind อาจใช้เวลาคิดนาน)

      // ตั้งค่า Header สำหรับ API Request
      http.addHeader("Content-Type", "application/json");                // ระบุ Content เป็น JSON
      http.addHeader("Authorization", String("Bearer ") + bearerToken);  // ส่ง API Token

      // สร้าง JSON Payload (คำถามที่จะส่งไปถาม GearMind)
      StaticJsonDocument<200> doc;
      doc["messages"][0]["role"] = "user";                                                                 // ระบุ Role เป็น "user"
      doc["messages"][0]["content"] = "สถานีชาร์จรถยนต์ ABB ขึ้น Error Code Isolation Test Failed แก้ไขอย่างไร";  // คำถาม

      // แปลง JSON Payload เป็น String
      String requestBody;
      serializeJson(doc, requestBody);

      // ส่ง HTTP POST Request ไปยัง GearMind API
      int httpCode = http.POST(requestBody);

      // ประมวลผล Response จาก GearMind API
      if (httpCode > 0) {                   // Request สำเร็จ
        String payload = http.getString();  // อ่าน Response Body
        Serial.println("Status code: " + String(httpCode));
        Serial.println("Response from GearMind: " + payload);

        StaticJsonDocument<1024> responseDoc;
        DeserializationError error = deserializeJson(responseDoc, payload);

        if (error) {
          Serial.print("deserializeJson() failed:");
          Serial.println(error.c_str());
          return;
        }

        const char* content = responseDoc["content"];

        if (content) {
          Serial.println("Content: " + String(content));
          LINE.notify(content);  // ส่งเฉพาะ content ไปยัง LINE Notify
        } else {
          Serial.println("Content not found in the response.");
        }

      } else {  // Request ล้มเหลว
        Serial.println("Error on HTTP request: " + http.errorToString(httpCode));
      }

      // ปิด HTTP Connection
      http.end();
    } else {
      Serial.println("Unable to connect to GearMind API");
    }
  }

  delay(60000);  // รอ 1 นาที ก่อนส่ง Request ถัดไป
}
