#include <WiFi.h>
#include <PubSubClient.h>
#include <WiFiManager.h> // Tambahkan library WiFiManager

// MQTT
const char* mqtt_server = "10.2.22.192";
const int mqtt_port = 1883;
const char* mqtt_topic = "fog/iot/home";
const char* mqtt_client_id = "esp32_pub_home";

// Pins
const int mq135Pin = 34;
const int buzzerPin = 26;
const float gasThreshold = 600.0;

// ==========================================================
// ✅ PERUBAHAN 1: Tambahkan variabel global untuk device ID
// ==========================================================
String deviceId = "";


WiFiClient espClient;
PubSubClient client(espClient);

void reconnect() {
 while (!client.connected()) {
  Serial.print("Menghubungkan ke MQTT... ");
  if (client.connect(mqtt_client_id)) {
   Serial.println("✅ Terhubung ke MQTT Broker!");
  } else {
   Serial.print("Gagal, rc=");
   Serial.print(client.state());
   Serial.println(" mencoba lagi dalam 5 detik...");
   delay(5000);
  }
 }
}

void setup() {
 Serial.begin(115200);
 pinMode(mq135Pin, INPUT);
 pinMode(buzzerPin, OUTPUT);
 digitalWrite(buzzerPin, LOW);

 // ======= Koneksi WiFi via WiFiManager =======
 WiFiManager wm;
  // wm.resetSettings(); // Aktifkan jika ingin reset paksa kredensial WiFi setiap kali restart
 bool res = wm.autoConnect("ESP32-Hasma", "12345678");
 
 if (!res) {
  Serial.println("❌ Gagal terhubung ke WiFi! Restarting...");
  ESP.restart();
 }

 Serial.println("✅ WiFi terhubung");
 Serial.print("IP: ");
 Serial.println(WiFi.localIP());

  // ====================================================================
  // ✅ PERUBAHAN 2: Membuat Device ID unik berdasarkan MAC Address
  // ====================================================================
  String mac = WiFi.macAddress(); // Dapatkan MAC Address (contoh: AA:BB:CC:DD:EE:FF)
  mac.replace(":", ""); // Hapus tanda ':' menjadi AABBCCDDEEFF
  String macSuffix = mac.substring(mac.length() - 5); // Ambil 5 karakter terakhir
  
  deviceId = "HOME-SENSOR-" + macSuffix; // Gabungkan menjadi ID unik
  
  Serial.print("MAC Address: ");
  Serial.println(mac);
  Serial.print("Device ID Unik: ");
  Serial.println(deviceId);
  // ====================================================================

 client.setServer(mqtt_server, mqtt_port);
}

void loop() {
 if (!client.connected()) {
  reconnect();
 }
 client.loop();

 int analogValue = analogRead(mq135Pin); // 0 - 4095
 float ppm = analogValue * (1000.0 / 4095.0); // estimasi sederhana

 float power_usage = random(5, 50) + random(0, 100) / 100.0;

 String kondisi = (ppm > gasThreshold) ? "Berbahaya" : "Normal";
 digitalWrite(buzzerPin, (ppm > gasThreshold) ? HIGH : LOW);

 unsigned long timestamp = millis() / 1000;
 String payload = "{";
 payload += "\"timestamp\":" + String(timestamp) + ",";
  // ==================================================================
  // ✅ PERUBAHAN 3: Gunakan variabel deviceId yang sudah dibuat
  // ==================================================================
 payload += "\"device_id\":\"" + deviceId + "\",";
 payload += "\"sensor\":\"MQ-135\",";
 payload += "\"ppm\":" + String(ppm, 2) + ",";
 payload += "\"power_usage\":" + String(power_usage, 2) + ",";
 payload += "\"kondisi\":\"" + kondisi + "\"";
 payload += "}";

 if (client.publish(mqtt_topic, payload.c_str())) {
  Serial.println("✅ Data terkirim:");
  Serial.println(payload);
 } else {
  Serial.println("❌ Gagal mengirim data.");
 }

 delay(3000);
}