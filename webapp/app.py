from flask import Flask, render_template, request, jsonify
from influxdb_client import InfluxDBClient

app = Flask(__name__)

# ===== Konfigurasi InfluxDB =====
INFLUXDB_URL = "http://10.2.22.192:8086"
INFLUXDB_TOKEN = "y5rUZo5y2u0pl6NE8OtPCYD-N8BK4KRF5fQtYjokYjIYh_9DP4dQUoKDpTGcft4bbD7IvHaeOaiSldeuBwvgtw=="
INFLUXDB_ORG = "klp2"
INFLUXDB_BUCKET = "IoT"

client = InfluxDBClient(url=INFLUXDB_URL, token=INFLUXDB_TOKEN, org=INFLUXDB_ORG)
query_api = client.query_api()

# ===== Mapping Sistem Berdasarkan Prefix Device ID =====
SYSTEM_CONFIG = {
    "AGRI": {"measurement": "Agriculture", "name": "Smart Agriculture", "icon": "eco", "color": "#4caf50"},
    "HOME": {"measurement": "Home", "name": "Smart Home", "icon": "home", "color": "#ff9800"},
    "CITY": {"measurement": "City", "name": "Smart City", "icon": "location_city", "color": "#3f51b5"},
    "HEALTH": {"measurement": "HealthCare", "name": "Smart Healthcare", "icon": "local_hospital", "color": "#f44336"},
    "ENERGY": {"measurement": "Energy", "name": "Smart Energy", "icon": "bolt", "color": "#2196f3"},
}

def get_system_by_device_id(device_id):
    """Mendapatkan konfigurasi sistem berdasarkan Device ID"""
    if not isinstance(device_id, str) or '-' not in device_id:
        return None
    prefix = device_id.split('-')[0].upper()
    return SYSTEM_CONFIG.get(prefix)

def get_latest_data(device_id):
    """Mengambil data terbaru dari InfluxDB untuk device tertentu"""
    system = get_system_by_device_id(device_id)
    if not system:
        return None, None

    query = f'''
    from(bucket: "{INFLUXDB_BUCKET}")
      |> range(start: -12h)
      |> filter(fn: (r) => r["_measurement"] == "{system['measurement']}")
      |> filter(fn: (r) => r["device_id"] == "{device_id}")
      |> last()
    '''

    try:
        result = query_api.query(org=INFLUXDB_ORG, query=query)
        
        data = {}
        sensor_info = ""
        kondisi = ""
        
        for table in result:
            for record in table.records:
                # Ambil field dan value
                field_name = record.get_field()
                field_value = record.get_value()
                
                # Simpan data field yang relevan
                if field_name and field_name not in ['value', '_value']:
                    data[field_name] = field_value
                
                # Ambil tags dan field lainnya
                for key, value in record.values.items():
                    if key not in ["_start", "_stop", "_time", "_measurement", "_field", "_value", "result", "table"]:
                        if key == "sensor":
                            sensor_info = value
                        elif key == "kondisi":
                            kondisi = value
                        elif key not in ['value', '_value'] and value is not None:
                            data[key] = value
        
        # Tambahkan informasi sensor dan kondisi ke data
        if sensor_info:
            data["sensor"] = sensor_info
        if kondisi:
            data["kondisi"] = kondisi
            
        # Filter data untuk hanya menampilkan sensor dan kondisi yang relevan
        filtered_data = {}
        for key, value in data.items():
            # Skip field yang tidak diperlukan
            if key not in ['value', '_value', 'result', 'table'] and value is not None:
                filtered_data[key] = value
        
        return system, filtered_data if filtered_data else None
        
    except Exception as e:
        print(f"Error querying InfluxDB: {e}")
        return system, None

@app.route("/", methods=["GET", "POST"])
def index():
    if request.method == "POST":
        device_id = request.form.get("device_id", "").strip()
        
        if not device_id:
            return render_template("index.html", error="Device ID tidak boleh kosong.")
        
        system, data = get_latest_data(device_id)
        
        if not system:
            return render_template("index.html", error="Device ID tidak valid. Pastikan menggunakan format yang benar (contoh: ENERGY-SENSOR-92810).")
        
        if not data:
            return render_template("index.html", error="Device ID tidak ditemukan atau tidak ada data terbaru.")
        
        # Return success response with redirect flag
        return render_template("index.html", 
                             success=True, 
                             device_id=device_id, 
                             system=system, 
                             data=data)
    
    return render_template("index.html")

@app.route("/api/device/<device_id>")
def api_device_data(device_id):
    """API endpoint untuk mendapatkan data device dalam format JSON"""
    system, data = get_latest_data(device_id)
    
    if not system or not data:
        return jsonify({"error": "Device not found or no data available"}), 404
    
    return jsonify({
        "device_id": device_id,
        "system": system,
        "data": data,
        "status": "success"
    })

if __name__ == "__main__":
    app.run(debug=True, host="0.0.0.0", port=5000)