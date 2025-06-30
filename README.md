# IoT Monitoring Dashboard dengan MQTT, InfluxDB, dan Flask

<p align="center">
  <img src="https://raw.githubusercontent.com/fadilikrm/IoT-Application/main/docs/screenshot.png" alt="Tampilan Dashboard" width="700"/>
</p>

<p align="center">
  Sebuah sistem monitoring IoT lengkap untuk menerima, menyimpan, dan memvisualisasikan data sensor secara real-time.
  <br />
  <a href="#arsitektur-sistem"><strong>Lihat Arsitektur »</strong></a>
  ·
  <a href="https://github.com/fadilikrm/IoT-Application/issues">Laporkan Bug</a>
</p>

<p align="center">
  <img src="https://img.shields.io/badge/Python-3.9%2B-blue?style=for-the-badge&logo=python" alt="Python Version">
  <img src="https://img.shields.io/badge/Flask-2.x-black?style=for-the-badge&logo=flask" alt="Flask Version">
  <img src="https://img.shields.io/badge/InfluxDB-2.x-purple?style=for-the-badge&logo=influxdb" alt="InfluxDB Version">
  <img src="https://img.shields.io/badge/License-MIT-green.svg?style=for-the-badge" alt="License">
</p>


## Tentang Proyek

Proyek ini adalah sebuah implementasi sistem *Internet of Things* (IoT) yang dirancang untuk memonitor berbagai jenis data dari sensor. Data dikirim melalui protokol **MQTT**, diproses oleh sebuah *subscriber* Python, lalu disimpan dalam database time-series **InfluxDB**.

Sebuah aplikasi web yang dibangun menggunakan **Flask** menyediakan antarmuka bagi pengguna untuk melihat data terbaru dari perangkat manapun hanya dengan memasukkan ID perangkat yang unik.

---

## Arsitektur Sistem

Alur kerja data dalam sistem ini dirancang agar efisien dan modular, dari perangkat hingga ke pengguna akhir.


[Perangkat IoT] ----(JSON via MQTT)----> [MQTT Broker] ----> [sub-all.py] ----> [InfluxDB] <---- [app.py] <---- [Browser Pengguna]


1.  **Perangkat IoT**: Mengirimkan data sensor dalam format JSON ke topik MQTT tertentu (misal: `fog/iot/agri`).
2.  **MQTT Broker**: Bertindak sebagai perantara pesan antara perangkat dan subscriber.
3.  **`sub-all.py` (Subscriber)**: Skrip ini berlangganan ke semua topik yang relevan. Saat pesan masuk, skrip ini akan:
    * Menghitung latensi dan throughput.
    * Menyusun data ke dalam format yang sesuai untuk InfluxDB.
    * Menyimpan data tersebut ke dalam bucket InfluxDB.
4.  **InfluxDB**: Menyimpan semua data time-series secara persisten.
5.  **`app.py` (Aplikasi Web Flask)**: Menyediakan UI dan API. Saat pengguna meminta data untuk sebuah `device_id`:
    * Aplikasi akan mengirimkan kueri (query) ke InfluxDB.
    * Menampilkan data terbaru yang diterima dari database ke dalam halaman web.
6.  **Browser Pengguna**: Antarmuka tempat pengguna berinteraksi dengan sistem.

---

## Struktur Kode

Struktur folder dan file di dalam repositori ini diatur untuk memisahkan antara logika backend dan antarmuka web.


/IoT-Application
│
├── app.py              # Aplikasi utama Flask (backend web & API)
├── sub-all.py          # Skrip subscriber MQTT untuk menyimpan data ke InfluxDB
├── requirements.txt    # Daftar semua dependensi Python yang dibutuhkan
│
├── /templates/
│   └── index.html      # File HTML untuk antarmuka dashboard
│
├── /docs/
│   └── screenshot.png  # Gambar cuplikan layar untuk README
│
└── README.md           # File dokumentasi ini


### Penjelasan File:

* **`app.py`**: Berisi semua logika untuk web server. Menggunakan Flask untuk merender halaman `index.html`, menangani permintaan dari pengguna (form POST), dan menyediakan endpoint API (`/api/device/<device_id>`) untuk mengambil data dalam format JSON.
* **`sub-all.py`**: Merupakan service *listener* yang berjalan terus-menerus. Menggunakan Paho-MQTT untuk terhubung ke broker dan InfluxDB-Client untuk menulis setiap pesan yang masuk ke database.
* **`requirements.txt`**: Mendefinisikan semua pustaka Python yang diperlukan proyek ini, memungkinkan instalasi yang mudah dan terprediksi dengan `pip`.
* **`templates/index.html`**: Halaman antarmuka yang dilihat oleh pengguna. Menggunakan HTML, CSS, dan sedikit JavaScript untuk menampilkan data dan menangani interaksi pengguna.

---

## Fitur Utama

* **Ingesti Data Multi-Domain**: Mampu menerima data dari 5 domain IoT berbeda (Agriculture, HealthCare, City, Home, Energy).
* **Kalkulasi Metrik**: Menghitung **latensi** (ms) dan **throughput** (pesan/menit) secara otomatis saat data diterima.
* **Dashboard Interaktif**: Antarmuka web yang responsif untuk mencari dan menampilkan status terbaru dari perangkat berdasarkan ID.
* **API Endpoint**: Menyediakan endpoint JSON di `/api/device/<device_id>` untuk kemudahan integrasi dengan sistem lain.
* **Pemisahan Logika**: Logika penerimaan data (`sub-all.py`) terpisah dari logika penyajian data (`app.py`), memungkinkan skalabilitas yang lebih baik.

---

## Teknologi yang Digunakan

* **Backend**: Python
* **Web Framework**: Flask
* **Database**: InfluxDB
* **Protokol Pesan**: MQTT
* **Pustaka Python**:
    * `flask`
    * `paho-mqtt`
    * `influxdb-client`

---

## Memulai

Untuk menjalankan proyek ini di lingkungan lokal Anda, ikuti langkah-langkah berikut.

### Prasyarat

* **Python 3.9** atau versi lebih baru.
* **MQTT Broker** yang sedang berjalan (contoh: Mosquitto, EMQ X).
* **InfluxDB 2.x** yang sudah terinstal dan dikonfigurasi.

### Instalasi & Konfigurasi

1.  **Clone Repositori**
    ```sh
    git clone [https://github.com/fadilikrm/IoT-Application.git](https://github.com/fadilikrm/IoT-Application.git)
    cd IoT-Application
    ```

2.  **Buat Virtual Environment (Sangat Direkomendasikan)**
    ```sh
    python -m venv venv
    # Windows
    venv\Scripts\activate
    # macOS / Linux
    source venv/bin/activate
    ```

3.  **Instal Dependensi**
    ```sh
    pip install -r requirements.txt
    ```

4.  **Konfigurasi InfluxDB**
    * Pastikan InfluxDB sudah berjalan.
    * Buat sebuah **Bucket** (contoh: `IoT`).
    * Dapatkan **Organization Name** dan **API Token** Anda.

5.  **Atur Variabel Konfigurasi**
    Buka file `app.py` dan `sub-all.py` lalu sesuaikan variabel di bagian atas file dengan konfigurasi Anda:
    ```python
    MQTT_BROKER = "ALAMAT_IP_BROKER_ANDA"
    INFLUXDB_URL = "http://ALAMAT_IP_INFLUXDB_ANDA:8086"
    INFLUXDB_TOKEN = "TOKEN_INFLUXDB_ANDA"
    INFLUXDB_ORG = "NAMA_ORGANISASI_ANDA"
    INFLUXDB_BUCKET = "NAMA_BUCKET_ANDA"
    ```

---

## Cara Menjalankan

Anda perlu menjalankan kedua skrip utama (`sub-all.py` dan `app.py`) secara bersamaan di dua terminal yang berbeda.

1.  **Terminal 1: Jalankan Subscriber MQTT**
    Skrip ini harus dijalankan terlebih dahulu untuk memastikan tidak ada data yang hilang.
    ```sh
    python sub-all.py
    ```
    Anda akan melihat output setiap kali ada data yang berhasil disimpan ke InfluxDB.

2.  **Terminal 2: Jalankan Aplikasi Web Flask**
    ```sh
    python app.py
    ```
    Aplikasi web akan berjalan secara default di port `5000`.

3.  **Akses Dashboard**
    Buka browser Anda dan navigasi ke `http://localhost:5000`. Masukkan ID Perangkat yang valid untuk melihat data terbarunya.

## Lisensi

Didistribusikan di bawah Lisensi MIT.

## Kontak

Fadhil Ikram - [GitHub](https://github.com/fadilikrm)
