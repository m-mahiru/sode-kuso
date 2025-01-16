#include <TinyGPS++.h>
#include <HardwareSerial.h>
#include <WiFi.h>
#include <HTTPClient.h>

// Wi-Fi設定
const char* ssid = "ベリーのiphone";
const char* password = "1234567890abec";

// サーバー設定
const char* serverUrl = "http://163.43.208.237/"; // センサーデータ送信用URL

// GPS設定
HardwareSerial gpsSerial(1); // UART1を使用
TinyGPSPlus gps;

// センサーのGPIOピン
const int humanSensorPin = 18; // 人感センサーのピン
const int lightSensorPin = 4; // 感光性センサーのピン

void setup() {
  Serial.begin(9600); // デバッグ用
  gpsSerial.begin(19200, SERIAL_8N1, 16, 17); // RX=16, TX=17でGPSモジュールを接続

  // センサー初期化
  pinMode(humanSensorPin, INPUT);
  pinMode(lightSensorPin, INPUT);

  // Wi-Fi接続
  WiFi.begin(ssid, password);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to Wi-Fi");
}

void loop() {
  while (gpsSerial.available() > 0) {
    char c = gpsSerial.read();
    if (gps.encode(c)) { // GPSデータが正しくデコードされた場合
      if (gps.location.isUpdated()) {
        double latitude = gps.location.lat();
        double longitude = gps.location.lng();
        Serial.printf("Location: %.6f, %.6f\n", latitude, longitude);

        // センサー値を取得
        bool humanSensorState = digitalRead(humanSensorPin) == HIGH;
        bool lightSensorState = digitalRead(lightSensorPin) == HIGH;

        // データをサーバーに送信
        sendDataToServer(latitude, longitude, humanSensorState, lightSensorState);
      }
    }
  }
}

void sendDataToServer(double latitude, double longitude, bool humanSensorState, bool lightSensorState) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(serverUrl);
    http.addHeader("Content-Type", "application/json");

    // JSONデータを作成
    String jsonPayload = "{";
    jsonPayload += "\"latitude\": " + String(latitude, 6) + ", ";
    jsonPayload += "\"longitude\": " + String(longitude, 6) + ", ";
    jsonPayload += "\"human_sensor\": " + String(humanSensorState ? "true" : "false") + ", ";
    jsonPayload += "\"light_sensor\": " + String(lightSensorState ? "true" : "false");
    jsonPayload += "}";

    // POSTリクエストを送信
    int httpResponseCode = http.POST(jsonPayload);

    if (httpResponseCode > 0) {
      Serial.printf("Data sent. Server response: %d\n", httpResponseCode);
    } else {
      Serial.printf("Error sending data: %s\n", http.errorToString(httpResponseCode).c_str());
    }

    http.end();
  } else {
    Serial.println("Wi-Fi not connected, data not sent");
  }
}
