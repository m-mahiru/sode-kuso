#include <TinyGPS++.h>
#include <HardwareSerial.h>
#include <WiFi.h>
#include <HTTPClient.h>

// Wi-Fi設定
const char* ssid = "ベリーのiphone";
const char* password = "1234567890abec";

// サーバー設定
const char* serverUrl = "http://your-server.com/location";
const char* alertUrl = "http://your-server.com/alert"; // 警告送信用URL

// GPS設定
HardwareSerial gpsSerial(1); // UART1を使用
TinyGPSPlus gps;

// センサーのGPIOピン
const int pirPin = 4; // 人感センサーのピン
const int lightSensorPin = 5; // 感光性センサーのピン

// サーバ位置情報（例: 仮の位置情報を設定）
double serverLatitude = 35.6895; // サーバの緯度
double serverLongitude = 139.6917; // サーバの経度

// 閾値（距離）を設定（例: 100m以内）
const double locationThreshold = 0.1; // 単位: km

void setup() {
  Serial.begin(9600); // デバッグ用
  gpsSerial.begin(19200, SERIAL_8N1, 16, 17); // RX=16, TX=17でGPSモジュールを接続

  // センサー初期化
  pinMode(pirPin, INPUT);
  pinMode(lightSensorPin, INPUT);

  // Wi-Fi接続
  WiFi.begin(ssid, password);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\\nConnected to Wi-Fi");
}

void loop() {
  while (gpsSerial.available() > 0) {
    char c = gpsSerial.read();
    if (gps.encode(c)) { // GPSデータが正しくデコードされた場合
      if (gps.location.isUpdated()) {
        double latitude = gps.location.lat();
        double longitude = gps.location.lng();
        Serial.printf("Location: %.6f, %.6f\\n", latitude, longitude);

        // サーバの位置情報と比較
        double distance = calculateDistance(latitude, longitude, serverLatitude, serverLongitude);
        Serial.printf("Distance to server: %.2f km\\n", distance);

        if (distance > locationThreshold) {
          // センサーの状態を確認
          if (digitalRead(pirPin) == HIGH || digitalRead(lightSensorPin) == HIGH) {
            Serial.println("Alert: Sensor triggered and location mismatch!");
            sendAlertToServer();
          }
        }
      }
    }
  }
}

double calculateDistance(double lat1, double lon1, double lat2, double lon2) {
  // 簡易的な距離計算（ハバースイン数式を使用）
  const double R = 6371.0; // 地球の半径 (km)
  double dLat = radians(lat2 - lat1);
  double dLon = radians(lon2 - lon1);
  double a = sin(dLat / 2) * sin(dLat / 2) +
             cos(radians(lat1)) * cos(radians(lat2)) * sin(dLon / 2) * sin(dLon / 2);
  double c = 2 * atan2(sqrt(a), sqrt(1 - a));
  return R * c;
}

void sendAlertToServer() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(alertUrl);
    http.addHeader("Content-Type", "application/json");

    // JSONデータを作成
    String jsonPayload = "{\"alert\": \"Sensor triggered\", \"details\": \"Location mismatch detected.\"}";

    // POSTリクエストを送信
    int httpResponseCode = http.POST(jsonPayload);

    if (httpResponseCode > 0) {
      Serial.printf("Alert sent. Server response: %d\\n", httpResponseCode);
    } else {
      Serial.printf("Error sending alert: %s\\n", http.errorToString(httpResponseCode).c_str());
    }

    http.end();
  } else {
    Serial.println("Wi-Fi not connected, alert not sent");
  }
}

