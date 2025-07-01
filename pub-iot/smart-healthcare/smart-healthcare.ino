#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <Wire.h>
#include <LiquidCrystal_PCF8574.h>

// ===== WiFi credentials =====
const char* ssid = "WLC_CNAP";
const char* password = "s4y4b1s4";

// ===== MQTT Broker Configuration =====
const char* mqtt_server = "10.2.22.150"; // Ganti dengan IP broker MQTT kamu
const int mqtt_port = 1883;
const char* mqtt_topic = "fog/iot/healthcare";

// ===== Sensor configuration =====
#define DHT_PIN 32
#define DHT_TYPE DHT22
#define MQ135_PIN 35

DHT dht(DHT_PIN, DHT_TYPE);
LiquidCrystal_PCF8574 lcd(0x27);  // Alamat I2C bisa disesuaikan jika perlu

WiFiClient espClient;
PubSubClient client(espClient);

// Ambil 5 karakter terakhir dari MAC address
String getDeviceID() {
  String mac = WiFi.macAddress();
  mac.replace(":", "");
  return "ESP32-" + mac.substring(mac.length() - 5);
}

void setup_wifi() {
  delay(100);
  Serial.print("🔌 Menghubungkan ke WiFi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }

  Serial.println("\n📡 Terhubung ke WiFi!");
  Serial.print("IP Lokal: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("🔄 Mencoba koneksi ke MQTT broker...");
    String clientId = getDeviceID();

    if (client.connect(clientId.c_str())) {
      Serial.println("✅ Terhubung ke MQTT!");
    } else {
      Serial.print("❌ Gagal, rc=");
      Serial.print(client.state());
      Serial.println(". Coba lagi dalam 3 detik.");
      delay(3000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  dht.begin();
  lcd.begin(20, 4);          // LCD 20x4
  lcd.setBacklight(255);     // Nyalakan backlight LCD

  lcd.setCursor(0, 0);
  lcd.print("Smart Healthcare");
  lcd.setCursor(0, 1);
  lcd.print("Connecting WiFi...");
  delay(2000);

  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);

  lcd.clear();
  lcd.print("WiFi Connected!");
  delay(2000);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // Baca sensor
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();
  int ppm = analogRead(MQ135_PIN);

  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("❌ Gagal membaca DHT22.");
    lcd.clear();
    lcd.print("Sensor Error!");
    delay(2000);
    return;
  }

  // Status udara
  String kondisi = (ppm > 600 || temperature > 30 || humidity > 70) ? "Buruk" : "Baik";

  // Tampilkan ke Serial
  Serial.printf("🌡 Suhu: %.2f C\n", temperature);
  Serial.printf("💧 Kelembaban: %.2f %%\n", humidity);
  Serial.printf("🛢 PPM: %d\n", ppm);
  Serial.printf("📉 Kondisi: %s\n", kondisi.c_str());

  // Tampilkan ke LCD
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Suhu: ");
  lcd.print(temperature, 1);
  lcd.print((char)223);
  lcd.print("C");

  lcd.setCursor(0, 1);
  lcd.print("Kelembaban: ");
  lcd.print(humidity, 1);
  lcd.print("%");

  lcd.setCursor(0, 2);
  lcd.print("PPM: ");
  lcd.print(ppm);

  lcd.setCursor(0, 3);
  lcd.print("Status: ");
  lcd.print(kondisi);

  // Kirim data ke MQTT
  String payload = "{";
  payload += "\"device_id\":\"" + getDeviceID() + "\",";
  payload += "\"sensor\":\"DHT22_MQ135\",";
  payload += "\"temperature\":" + String(temperature, 2) + ",";
  payload += "\"humidity\":" + String(humidity, 2) + ",";
  payload += "\"ppm\":" + String(ppm);
  payload += "}";

  Serial.println("📤 Kirim data MQTT:");
  Serial.println(payload);
  client.publish(mqtt_topic, payload.c_str());

  delay(5000); // Kirim setiap 5 detik
}
