
#include <MPU6050_tockn.h>
#include <Wire.h>
#include <WiFi.h>

const char* ssid = "SSID"
const char* pass = "PASSWORD";

int status = WL_IDLE_STATUS;
IPAddress server(192,168,1,28);
int port = 8000;

WiFiClient client;

MPU6050 mpu6050(Wire);

long timer = 0;

void setup() {
  Serial.begin(115200);
  Wire.begin(25, 26);
  mpu6050.begin();
  mpu6050.calcGyroOffsets(true);
  
  pinMode(32, OUTPUT);
  connectToWiFi();
}

void connectToWiFi(){
    WiFi.begin(ssid, pass);
    Serial.print("Establishing WiFi connection.");
    while (WiFi.status() != WL_CONNECTED) { 
        Serial.print(".");
        delay(500);
    }
    Serial.println("");
    Serial.println("Device connected to the Internet.");
}

void connectClient(){
    while (!client.connected()){
        client.connect(server, port);
        Serial.print(".");
        delay(500);
    }
    Serial.println("");
    Serial.println("Connected to server.");
}

void scanNetworks() {
 
  int numberOfNetworks = WiFi.scanNetworks();
 
  Serial.print("Number of networks found: ");
  Serial.println(numberOfNetworks);
 
  for (int i = 0; i < numberOfNetworks; i++) {
 
    Serial.print("Network name: ");
    Serial.println(WiFi.SSID(i));
 
    Serial.print("Signal strength: ");
    Serial.println(WiFi.RSSI(i));
 
    Serial.print("MAC address: ");
    Serial.println(WiFi.BSSIDstr(i));
 
    Serial.print("Encryption type: ");
    String encryptionTypeDescription = translateEncryptionType(WiFi.encryptionType(i));
    Serial.println(encryptionTypeDescription);
    Serial.println("-----------------------");
 
  }
}

void loop() {
  mpu6050.update();

  if(millis() - timer > 1000){
    
    Serial.println("=======================================================");
    Serial.print("temp : ");Serial.println(mpu6050.getTemp());
    Serial.print("accX : ");Serial.print(mpu6050.getAccX());
    Serial.print("\taccY : ");Serial.print(mpu6050.getAccY());
    Serial.print("\taccZ : ");Serial.println(mpu6050.getAccZ());
  
    Serial.print("gyroX : ");Serial.print(mpu6050.getGyroX());
    Serial.print("\tgyroY : ");Serial.print(mpu6050.getGyroY());
    Serial.print("\tgyroZ : ");Serial.println(mpu6050.getGyroZ());
  
    Serial.print("accAngleX : ");Serial.print(mpu6050.getAccAngleX());
    Serial.print("\taccAngleY : ");Serial.println(mpu6050.getAccAngleY());
  
    Serial.print("gyroAngleX : ");Serial.print(mpu6050.getGyroAngleX());
    Serial.print("\tgyroAngleY : ");Serial.print(mpu6050.getGyroAngleY());
    Serial.print("\tgyroAngleZ : ");Serial.println(mpu6050.getGyroAngleZ());
    
    Serial.print("angleX : ");Serial.print(mpu6050.getAngleX());
    Serial.print("\tangleY : ");Serial.print(mpu6050.getAngleY());
    Serial.print("\tangleZ : ");Serial.println(mpu6050.getAngleZ());
    Serial.println("=======================================================\n");
    timer = millis();
  }






  if(client.connected()){
    client.write_P(mpu6050.getTemp() + ":" + mpu6050.getAccX() + ":" + mpu6050.getAccY() + 
    ":" + mpu6050.getAccZ() + ":" + mpu6050.getGyroX() + ":" + mpu6050.getGyroY() + ":" + 
    mpu6050.getGyroZ() + ":" + mpu6050.getAccAngleX() + ":" + mpu6050.getAccAngleY() +
    ":" + mpu6050.getGyroAngleX() + ":" + mpu6050.getGyroAngleY() + ":" + mpu6050.getGyroAngleZ() +
    ":" + mpu6050.getAngleX() + ":" + mpu6050.getAngleY() + ":" + mpu6050.getAngleZ());
    Serial.println("Transmitted data to server.");
  } else {
    Serial.print("Connecting client.");
    connectClient();
  }
  delay (2000)
}
