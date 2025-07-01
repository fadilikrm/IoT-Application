#include <WiFi.h>
#include <PubSubClient.h>
#include <WiFiManager.h> // pastikan versi untuk ESP32

// ===== MQTT Configuration =====
const char* mqtt_server = "10.2.22.150";
const int mqtt_port = 1883;
const char* mqtt_topic = "fog/iot/agri";

String deviceId = ""; // Variabel untuk menyimpan Device ID unik kita

WiFiClient espClient;
PubSubClient client(espClient);

// ===== Sensor Pins =====
#define TRIG_PIN 27
#define ECHO_PIN 14
#define WATER_FLOW_SENSOR_PIN 26

// ===== Flow Sensor Calculation =====
volatile int pulseCount = 0;
float flowRate = 0.0;
float flowSpeed = 0.0;
unsigned long previousMillis = 0;

const float calibrationFactor = 4.5;
const float pipeDiameter = 0.05;
const float litersToCubicMeters = 0.001;

// ===== Interrupt Handler =====
void IRAM_ATTR pulseCounter() {
 pulseCount++;
}

// ===== MQTT Reconnect =====
void reconnect() {
 while (!client.connected()) {
  Serial.print("🔁 Menghubungkan ke MQTT Broker...");

    // =================================================================
    // ✅ PERBAIKAN: Gunakan deviceId langsung sebagai MQTT Client ID
    //    Ini lebih bersih dan sudah dijamin unik.
    // =================================================================
  if (client.connect(deviceId.c_str())) {
   Serial.println("✅ Terhubung ke MQTT!");
  } else {
   Serial.print("❌ Gagal, rc=");
   Serial.print(client.state());
   Serial.println(" coba lagi dalam 5 detik");
   delay(5000);
  }
 }
}

// ===== Setup =====
void setup() {
 Serial.begin(115200);
 delay(1000);

 // ==== WiFiManager Setup ====
 WiFiManager wm;
 wm.setTimeout(180);
 if (!wm.autoConnect("ESP32-Uchank", "123456")) {
  Serial.println("❌ Gagal konek. Restart...");
  ESP.restart();
 }
 Serial.println("✅ Terhubung ke WiFi!");
 Serial.print("IP: ");
 Serial.println(WiFi.localIP());

  // ==========================================================================
  // ✅ PERUBAHAN UTAMA: Membuat Device ID unik dari 5 karakter MAC Address
  // ==========================================================================
  String mac = WiFi.macAddress();
  mac.replace(":", "");
  String macSuffix = mac.substring(mac.length() - 5);
  
  deviceId = "AGRI-SENSOR-" + macSuffix;
  
  Serial.print("🆔 Device ID Unik Dibuat: ");
  Serial.println(deviceId);
  // ==========================================================================

 // ==== MQTT Setup ====
 client.setServer(mqtt_server, mqtt_port);

 // ==== Sensor Setup ====
 pinMode(TRIG_PIN, OUTPUT);
 pinMode(ECHO_PIN, INPUT);
 pinMode(WATER_FLOW_SENSOR_PIN, INPUT_PULLUP);
 attachInterrupt(digitalPinToInterrupt(WATER_FLOW_SENSOR_PIN), pulseCounter, RISING);
}

void loop() {
 if (!client.connected()) {
  reconnect();
 }
 client.loop();

 unsigned long currentMillis = millis();
 if (currentMillis - previousMillis >= 5000) {
  previousMillis = currentMillis;


  flowRate = (pulseCount / calibrationFactor); // L/min
  flowSpeed = (flowRate * litersToCubicMeters) / (3.14159 * pow(pipeDiameter / 2, 2)) / 60; // m/s
  pulseCount = 0;


  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 30000);
  float distance = (duration == 0) ? 8.0 : duration * 0.0344 / 2.0; // cm
  float waterHeight = max(0.0, 8.0 - distance); // asumsi 8 cm tinggi pipa

// Tentukan kondisi
  String kondisi = (flowSpeed < 1.0 || waterHeight < 15.0) ? "Kekurangan Air" : "Cukup";

// Buat payload JSON menggunakan deviceId yang sudah dibuat
  String payload = "{";
  payload += "\"device_id\": \"" + deviceId + "\",";
  payload += "\"sensor\": \"Water Flow & Ultrasonic\",";
  payload += "\"flow_speed\": " + String(flowSpeed, 2) + ",";
  payload += "\"water_height\": " + String(waterHeight, 2) + ",";
  payload += "\"kondisi\": \"" + kondisi + "\"";
  payload += "}";

  Serial.print("📤 Kirim MQTT: ");
  Serial.println(payload);

  client.publish(mqtt_topic, payload.c_str());
 }
}
