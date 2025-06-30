#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <ArduinoJson.h> // Diperlukan untuk membuat payload JSON

// ===== WiFi credentials =====
const char* ssid = "WLC_CNAP";
const char* password = "s4y4b1s4";

// ===== MQTT Broker settings =====
const char* mqtt_broker = "10.2.22.192";
const int mqtt_port = 1883;
// mqtt_client_id akan digenerate dari MAC Address
String mqtt_client_id_str; // Gunakan String untuk ID yang dinamis
const char* mqtt_topic = "fog/iot/city"; // Topik yang sama dengan pub-smartcity.py

// ===== Sensor configuration =====
#define DHT_PIN 13
#define DHT_TYPE DHT22 // DHT 22 (AM2302)
const int mq135Pin = 34; // GPIO 34 (analog input)
#define RED_LED_PIN 27   // GPIO 27 untuk LED indikator polusi

// Inisialisasi sensor DHT
DHT dht(DHT_PIN, DHT_TYPE);

// Inisialisasi WiFiClient dan PubSubClient
WiFiClient espClient;
PubSubClient client(espClient);

// Variabel untuk waktu pengiriman data
long lastMsg = 0;
const int PUBLISH_INTERVAL_SECONDS = 5; // Mengirim data setiap 5 detik

// ===== Fungsi koneksi WiFi (Statis) =====
void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  int retries = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    retries++;
    if (retries > 20) { // Coba 20 kali (10 detik total), lalu restart jika gagal
      Serial.println("\nFailed to connect to WiFi. Restarting...");
      ESP.restart();
    }
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

// ===== Fungsi callback MQTT (jika diperlukan untuk subscribe) =====
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

// ===== Fungsi reconnect MQTT =====
void reconnect() {
  // Loop hingga terkoneksi
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection as ");
    Serial.print(mqtt_client_id_str.c_str()); // Gunakan c_str() untuk konversi String ke const char*
    Serial.print("...");
    
    // Coba koneksi menggunakan ID klien dinamis
    if (client.connect(mqtt_client_id_str.c_str())) {
      Serial.println("connected");
      // Jika perlu subscribe topik lain
      // client.subscribe("your/topic");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Tunggu 5 detik sebelum mencoba lagi
      delay(5000);
    }
  }
}

// ===== Setup utama =====
void setup() {
  Serial.begin(115200);
  pinMode(RED_LED_PIN, OUTPUT); // Set pin LED sebagai output
  digitalWrite(RED_LED_PIN, LOW); // Pastikan LED mati di awal

  // Panggil fungsi setup_wifi yang statis
  setup_wifi();

  // --- Generate Device ID dari MAC Address ---
  byte mac[6];
  WiFi.macAddress(mac); // Dapatkan MAC Address
  char macStr[18];
  sprintf(macStr, "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  
  // Ambil 5 karakter terakhir
  String lastFiveMac = String(macStr).substring(String(macStr).length() - 5);
  // Ubah ke huruf kapital
  lastFiveMac.toUpperCase(); 
  
  // Bentuk device ID baru
  mqtt_client_id_str = "CITY-SENSOR-" + lastFiveMac; 

  Serial.print("Device ID: ");
  Serial.println(mqtt_client_id_str);

  client.setServer(mqtt_broker, mqtt_port);
  client.setCallback(callback); // Meskipun tidak subscribe, tetap set callback
  dht.begin(); // Inisialisasi sensor DHT
}

// ===== Loop utama =====
void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop(); // Pertahankan koneksi MQTT

  long now = millis();
  if (now - lastMsg > (PUBLISH_INTERVAL_SECONDS * 1000)) {
    lastMsg = now;

    // --- Baca data dari sensor ---
    float temperature = dht.readTemperature();
    float humidity = dht.readHumidity();
    int mq135Analog = analogRead(mq135Pin); // Baca nilai ADC dari MQ-135

    // ===== Konversi MQ-135 ke PPM (Perkiraan Sederhana) =====
    // Ini adalah konversi SANGAT sederhana dari nilai ADC mentah.
    // Untuk mendapatkan nilai PPM yang akurat dari MQ-135, diperlukan kalibrasi
    // yang lebih kompleks melibatkan pembakaran awal (pre-heating), suhu, kelembaban,
    // dan kurva kalibrasi spesifik dari datasheet sensor untuk berbagai gas.
    // Asumsi: rentang ADC ESP32 0-4095. Kita petakan ke rentang PPM 0-1000.
    // Sesuaikan faktor ini (1000.0/4095.0) berdasarkan eksperimen dan kalibrasi Anda.
    float ppm = (float)mq135Analog * (1000.0 / 4095.0); 
    ppm = constrain(ppm, 0.0, 2000.0); // Batasi nilai agar tidak negatif atau terlalu tinggi jika kalibrasi belum pas

    // --- Tentukan kondisi dan kontrol LED ---
    String kondisi = "Aman";
    bool polusiTinggi = false;
    if (ppm > 500) { // Jika PPM di atas 500 dianggap polusi tinggi
        kondisi = "Polusi Tinggi";
        polusiTinggi = true;
    } else if (ppm > 350) { // Contoh ambang batas lain
        kondisi = "Sedang";
    }

    if (polusiTinggi) {
      digitalWrite(RED_LED_PIN, HIGH); // Nyalakan LED
    } else {
      digitalWrite(RED_LED_PIN, LOW); // Matikan LED
    }

    // Cek apakah pembacaan sensor valid
    if (isnan(temperature) || isnan(humidity)) {
      Serial.println("Failed to read from DHT sensor! Skipping data publish.");
      return; 
    }

    // --- Buat payload JSON ---
    StaticJsonDocument<256> doc; // Ukuran dokumen JSON, sesuaikan jika data lebih besar

    doc["timestamp"] = time(nullptr); // Menggunakan waktu UNIX timestamp (pastikan NTP aktif untuk akurasi)
    doc["device_id"] = mqtt_client_id_str; // Gunakan ID yang digenerate dari MAC
    doc["sensor"] = "MQ-135 & DHT22"; // Sesuai dengan dummy pub
    // Pastikan semua nilai float dikirim sebagai float dengan menggunakan 100.0 (float)
    doc["temperature"] = round(temperature * 100.0) / 100.0; // Batasi 2 desimal, pastikan float
    doc["humidity"] = round(humidity * 100.0) / 100.0;     // Batasi 2 desimal, pastikan float
    doc["ppm"] = round(ppm * 100.0) / 100.0;             // Batasi 2 desimal, pastikan float
    // Pastikan power_usage adalah float dengan desimal untuk konsistensi di InfluxDB
    doc["power_usage"] = round((float)(esp_random() % 46 + 5) * 100.0) / 100.0; // Mengubah hasil integer menjadi float
    doc["kondisi"] = kondisi;

    char jsonBuffer[256];
    serializeJson(doc, jsonBuffer);

    // --- Kirim payload melalui MQTT ---
    Serial.print("Publishing message to topic ");
    Serial.print(mqtt_topic);
    Serial.print(": ");
    Serial.println(jsonBuffer);
    client.publish(mqtt_topic, jsonBuffer);
  }
}

// Catatan: Fungsi random() di Arduino berbeda dengan Python.
// esp_random() adalah fungsi yang lebih baik untuk ESP32 untuk mendapatkan angka acak.
// (int)esp_random() % (max - min + 1) + min akan memberikan angka acak dalam rentang [min, max]
int random(int minVal, int maxVal) {
    return esp_random() % (maxVal - minVal + 1) + minVal;
}