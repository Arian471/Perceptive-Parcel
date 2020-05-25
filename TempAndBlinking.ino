
#include <MPU6050_tockn.h>
#include <Wire.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>

const char* ssid = "Send Nudes";
const char* password = "totallysecurepassword3000";

const char* host = "15671838.ngrok.io";
const char* resource = "/sensors";

const char* serverNameTemp = "http://192.168.1.28:1880/temperature";
const char* serverNameHumi = "http://192.168.1.28:1880/gyroscope";
const char* serverNamePres = "http://192.168.1.28:1880/accelerometer";

WiFiClient client;

MPU6050 mpu6050(Wire);

long timer = 0;

void setup() {
  Serial.begin(115200);
  Wire.begin(25, 26);
  mpu6050.begin();
  mpu6050.calcGyroOffsets(true);

  pinMode(32, OUTPUT);
   
  WiFi.begin(ssid, password);
  Serial.println("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());
}

bool connect(const char* hostName, int portNumber) {
  Serial.print("Connect to ");
  Serial.println(hostName);

  bool ok = client.connect(hostName, portNumber);

  Serial.println(ok ? "Connected" : "Connection Failed!");
  return ok;
}

void loop() {
  if(connect(host, 80)) {
    mpu6050.update();
  
    DynamicJsonDocument doc(1024);
    
    doc["Temperature"] = mpu6050.getTemp();
    doc["AccX"] = mpu6050.getAccX();
    doc["AccY()"] = mpu6050.getAccY();
    doc["AccZ"] = mpu6050.getAccZ();
    doc["GyroX"] = mpu6050.getGyroX();
    doc["GyroY"] = mpu6050.getGyroY();
    doc["GyroZ"] = mpu6050.getGyroZ();
    doc["AccAngleX"] = mpu6050.getAccAngleX();
    doc["AccAngleY"] = mpu6050.getAccAngleY();
    doc["GyroAngleX"] = mpu6050.getGyroAngleX();
    doc["GyroAngleY"] = mpu6050.getGyroAngleY();
    doc["GyroAngleZ"] = mpu6050.getGyroAngleZ();
    doc["AngleX"] = mpu6050.getAngleX();
    doc["AngleY"] = mpu6050.getAngleY();
    doc["AngleZ"] = mpu6050.getAngleZ();
    serializeJson(doc, Serial);
    
    client.print("POST ");
    client.print(resource);
    client.println(" HTTP/1.1");
    client.print("Host: ");
    client.println(host);
    client.println("Connection: close\r\nContent-Type: application/json");
    client.print("Content-Length: ");  
    client.print(measureJson(doc));
    client.print("\r\n");
    client.println();
    serializeJson(doc, client);
  
    int n = WiFi.scanNetworks();
    Serial.println("scan done");
    if (n == 0) {
      doc["network"] = "";
    } else {
      doc["network"] = "";
      DynamicJsonDocument networks(1024);
      for (int i = 0; i < n; ++i) {
        // Print SSID and RSSI for each network found
        networks[i]["SSID"] = WiFi.SSID(i);
        networks[i]["RSSI"] = WiFi.RSSI(i);
      }
      doc["network"] = networks;
    }
    serializeJson(doc, Serial);
  }
}
