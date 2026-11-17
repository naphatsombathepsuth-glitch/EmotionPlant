// ==================== Blynk IoT Config ====================
#define BLYNK_TEMPLATE_ID   "TMPL6g94f2cpR"
#define BLYNK_TEMPLATE_NAME "Iot Lab"
#define BLYNK_AUTH_TOKEN    "HyPhc-FalxnW4KtHYhe-07Es315o9YSp"

#define BLYNK_PRINT Serial
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>

// ==================== Wi-Fi Config ====================
char ssid[] = "Satit_WiFi";     // แก้ไขชื่อ Wi-Fi แล้ว
char pass[] = "s@tit&2026";     // รหัสผ่าน Wi-Fi

// ==================== Pin Mapping ====================
const int SOIL_PIN   = A0;  // เซนเซอร์ FC-28
const int RELAY_PUMP = D1;  // Relay ควบคุมปั๊มน้ำ (GPIO5)
const int RELAY_AUX  = D2;  // Relay ตัวที่ 2 สำรอง (GPIO4)
const int LED_YELLOW = D5;  // ไฟเตือนดินแห้ง (GPIO14)
const int LED_RED    = D6;  // ไฟสถานะปั๊มทำงาน (GPIO12)

// ==================== Relay Signal Logic ====================
// หากลองแล้วปั๊มทำงานสลับกัน (กด On แต่ปั๊มดับ / กด Off แต่ปั๊มติด) 
// ให้สลับค่าตรงนี้เป็น: #define RELAY_ON HIGH  และ  #define RELAY_OFF LOW
#define RELAY_ON  LOW   // สัญญาณสั่งเปิด Relay
#define RELAY_OFF HIGH  // สัญญาณสั่งปิด Relay

// ==================== Sensor Calibration ====================
const int AIR_VALUE   = 1023; // ค่าเมื่อแห้งสนิท (ในอากาศ)
const int WATER_VALUE = 350;  // ค่าเมื่อชื้นสนิท (ในน้ำ)
const int DRY_LIMIT   = 30;   // ความชื้น < 30% = ดินแห้ง
const int WET_LIMIT   = 70;   // ความชื้น > 70% = ชื้นพอแล้ว

// ==================== System Variables ====================
int moisturePercent = 0;
bool isManualMode   = false;  // false = Auto, true = Manual
bool isPumpActive   = false;

BlynkTimer timer;

// ดึงสถานะปุ่มจาก Blynk มาอัปเดตเมื่อบอร์ดเชื่อมต่อสำเร็จ
BLYNK_CONNECTED() {
  Blynk.syncVirtual(V1, V2);
}

// --- ฟังก์ชันควบคุมเปิด-ปิด ปั๊มน้ำและไฟสถานะสีแดง ---
void setPump(bool turnOn) {
  isPumpActive = turnOn;
  if (turnOn) {
    digitalWrite(RELAY_PUMP, RELAY_ON);  // เปิดปั๊มน้ำ
    digitalWrite(LED_RED, HIGH);        // ติดไฟแดง (แสดงสถานะปั๊มทำงาน)
  } else {
    digitalWrite(RELAY_PUMP, RELAY_OFF); // ปิดปั๊มน้ำ
    digitalWrite(LED_RED, LOW);         // ปิดไฟแดง
  }
}

// ==================== Blynk Mode ====================

BLYNK_WRITE(V2) {

  isManualMode = param.asInt();

  Serial.print("Mode: ");
  Serial.println(isManualMode ? "MANUAL" : "AUTO");

  // ถ้าเปลี่ยนจาก Manual เป็น Auto
  // ให้ระบบ Auto ควบคุมปั๊มเอง
}


// ==================== Blynk Pump Button ====================

BLYNK_WRITE(V1) {

  int state = param.asInt();

  // ต้องอยู่ Manual ถึงจะสั่งปั๊มจากแอปได้
  if (isManualMode) {

    if (state == 1) {
      setPump(true);
    } 
    else {
      setPump(false);
    }

  } else {

    // Auto → ไม่ให้ปุ่ม V1 ควบคุมปั๊ม
    Blynk.virtualWrite(V1, isPumpActive ? 1 : 0);

    Serial.println("AUTO MODE: ปั๊มถูกควบคุมโดยระบบ");
  }
}

// --- ฟังก์ชันอ่านค่าเซนเซอร์และประมวลผล (ทำงานทุก 1 วินาที) ---
void sendSensorData() {
  int rawValue = analogRead(SOIL_PIN);
  moisturePercent = map(rawValue, AIR_VALUE, WATER_VALUE, 0, 100);
  moisturePercent = constrain(moisturePercent, 0, 100);

  // 1. ส่งค่าความชื้นขึ้น Blynk (Virtual Pin V0)
  Blynk.virtualWrite(V0, moisturePercent);

  // 2. ควบคุมไฟ LED สีเหลือง (ติดเมื่อดินแห้ง < 30%)
  if (moisturePercent < DRY_LIMIT) {
    digitalWrite(LED_YELLOW, HIGH); // ดินแห้ง -> ติดไฟเหลืองเตือน
  } else {
    digitalWrite(LED_YELLOW, LOW);  // ดินชื้นพอ -> ปิดไฟเหลือง
  }

  // 3. ควบคุมปั๊มน้ำในโหมด Auto
if (!isManualMode) {

  if (moisturePercent < DRY_LIMIT) {
    // ดินแห้ง → เปิดปั๊ม
    if (!isPumpActive) {
      setPump(true);
      Blynk.virtualWrite(V1, 1);
      digitalWrite(LED_YELLOW, HIGH);
      digitalWrite(LED_RED, HIGH);
    }
  }
  else {
    // ดินไม่แห้ง → ปิดปั๊ม
    if (isPumpActive) {
      setPump(false);
      Blynk.virtualWrite(V1, 0);
      digitalWrite(LED_YELLOW, LOW);
      digitalWrite(LED_RED, HIGH);
    }
  }
}

  // 4. แสดงผลข้อมูลผ่าน Serial Monitor
  Serial.print("ความชื้น: ");
  Serial.print(moisturePercent);
  Serial.print("% | โหมด: ");
  Serial.print(isManualMode ? "Manual" : "Auto");
  Serial.print(" | ปั๊มน้ำ: ");
  Serial.print(isPumpActive ? "ON [ติด]" : "OFF [ดับ]");
  Serial.print(" | LED เหลือง: ");
  Serial.println(moisturePercent < DRY_LIMIT ? "ON (ดินแห้ง)" : "OFF");
}

void setup() {
  Serial.begin(115200);

  pinMode(RELAY_PUMP, OUTPUT);
  pinMode(RELAY_AUX, OUTPUT);
  pinMode(LED_YELLOW, OUTPUT);
  pinMode(LED_RED, OUTPUT);

  // ตั้งสถานะเริ่มต้น
  digitalWrite(RELAY_PUMP, RELAY_OFF);
  digitalWrite(RELAY_AUX, RELAY_OFF);
  digitalWrite(LED_YELLOW, LOW);
  digitalWrite(LED_RED, LOW);

  Serial.println("\n--- กำลังเชื่อมต่อ Wi-Fi และ Blynk... ---");
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  // ตั้งเวลาอ่านค่าเซนเซอร์ทุกๆ 1,000 ms (1 วินาที)
  timer.setInterval(1000L, sendSensorData);
}

void loop() {
  Blynk.run();
  timer.run();
}
