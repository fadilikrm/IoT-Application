#include <WiFi.h>
#include <PubSubClient.h> // Library untuk MQTT
#include <ArduinoJson.h>  // Library untuk membuat format data JSON
#include <LiquidCrystal_I2C.h>

// ===== Konfigurasi WiFi =====
const char* ssid = "WLC_CNAP";       // SSID WiFi Anda
const char* password = "s4y4b1s4";   // Password WiFi Anda

// ===== Konfigurasi MQTT Broker =====
const char* mqtt_server = "10.2.22.192"; // Alamat IP MQTT Broker Anda
const int   mqtt_port = 1883;
const char* mqtt_topic = "fog/iot/energy";
const char* mqtt_client_id = "mqtt_pub_energy"; // ID unik untuk perangkat ini (bisa juga dibuat dinamis)

// ===== Definisi Pin =====
const int pirPin = 13;      // Pin untuk sensor PIR
const int ledPin = 15;      // Pin untuk LED
const int ldrPin = 34;      // Pin untuk sensor cahaya (LDR)

// ===== Variabel Global =====
String full_device_id; // Variabel untuk menyimpan device_id yang sudah dinamis

// ===== Inisialisasi Library =====
LiquidCrystal_I2C lcd(0x27, 16, 2);
WiFiClient espClient;
PubSubClient client(espClient);

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  lcd.clear();
  lcd.print("Connecting WiFi");

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    lcd.setCursor(0,1);
    lcd.print(".");
  }

  Serial.println("\nWiFi Connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  lcd.clear();
  lcd.print("WiFi Connected!");
  lcd.setCursor(0,1);
  lcd.print(WiFi.localIP());
  delay(2000);
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    lcd.clear();
    lcd.print("MQTT Connecting");
    
    if (client.connect(mqtt_client_id)) {
      Serial.println("connected");
      lcd.clear();
      lcd.print("MQTT Connected!");
      delay(1000);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      lcd.clear();
      lcd.print("MQTT Failed!");
      lcd.setCursor(0,1);
      lcd.print("Retrying...");
      delay(5000);
    }
  }
}

void setup() {
  pinMode(pirPin, INPUT);
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);

  Serial.begin(115200);

  lcd.init();
  lcd.backlight();
  
  setup_wifi();
  
  // ==================== PERUBAHAN DIMULAI DI SINI ====================
  // Membuat device_id dinamis berdasarkan MAC Address
  String mac = WiFi.macAddress(); // Dapatkan MAC address (contoh: 1A:2B:3C:4D:5E:6F)
  mac.replace(":", ""); // Hapus tanda ":" -> 1A2B3C4D5E6F
  String mac_suffix = mac.substring(mac.length() - 5); // Ambil 5 karakter terakhir
  
  full_device_id = "ENERGY-SENSOR-" + mac_suffix; // Gabungkan -> ENERGY-SENSOR-D5E6F

  Serial.println("=============================");
  Serial.print("Device ID terdaftar: ");
  Serial.println(full_device_id);
  Serial.println("=============================");
  lcd.clear();
  lcd.print("Device ID:");
  lcd.setCursor(0,1);
  lcd.print(full_device_id);
  delay(3000);
  // ===================== PERUBAHAN SELESAI =====================

  client.setServer(mqtt_server, mqtt_port);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  int gerak_terdeteksi = digitalRead(pirPin);
  int intensitas_cahaya = analogRead(ldrPin);

  String status_lampu;
  String kondisi;
  float power_usage;

  int lightThreshold = 1500;

  if (gerak_terdeteksi == 1 && intensitas_cahaya < lightThreshold) {
    status_lampu = "LAMPU ON";
    kondisi = "Aktif";
    power_usage = random(30, 100);
    digitalWrite(ledPin, HIGH);
  } else {
    status_lampu = "LAMPU OFF";
    kondisi = "Hemat Energi";
    power_usage = random(1, 5);
    digitalWrite(ledPin, LOW);
  }

  StaticJsonDocument<256> doc;

  doc["timestamp"] = millis();
  // ==================== PERUBAHAN DI PAYLOAD ====================
  doc["device_id"] = full_device_id; // Gunakan variabel yang sudah dibuat
  // ============================================================
  doc["sensor"] = "PIR & LDR";
  doc["gerak"] = gerak_terdeteksi;
  doc["cahaya"] = intensitas_cahaya;
  doc["lampu"] = status_lampu;
  doc["power_usage"] = power_usage;
  doc["kondisi"] = kondisi;

  char payload[256];
  serializeJson(doc, payload);

  client.publish(mqtt_topic, payload);

  Serial.print("Pesan terkirim: ");
  Serial.println(payload);
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Gerak: " + String(gerak_terdeteksi) + " | " + kondisi);
  lcd.setCursor(0, 1);
  lcd.print("Cahaya: " + String(intensitas_cahaya));

  delay(5000);
}