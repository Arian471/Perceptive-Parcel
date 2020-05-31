
#include <MPU6050_tockn.h>
#include <Wire.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include "SSD1306.h" 
#include <WifiLocation.h>

const char* googleApiKey = "AIzaSyD598bfaL-Cy6gRQGwXCzBAOn3EUBRQKZM";

const char* ssid = "Send Nudes";
const char* password = "totallysecurepassword3000";

const char* host = "b7e1ae648528.ngrok.io";
const char* resource = "/sensors";
const char* getAdr = "/sensors/test";

WifiLocation location(googleApiKey);

float flippedDegree = 140;
float maxAcc = 2;

String configurationData;
float configurationDataArr[2];

int timerDelayPost = 5000;


String alertAcc = "";
String alertFlip = "";

SSD1306  display(0x3c, 25, 26);

WiFiClient client;

MPU6050 mpu6050(Wire);

long lastTimePost = 0;
long lastTimeGet = 0;

float lastAcceleration;

float maxFound = 0;

float tempMaxAcc = 0;

void setup() {
  Serial.begin(115200);
  display.init();
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

  lastAcceleration = mpu6050.getAccX()+ mpu6050.getAccY()+ mpu6050.getAccZ();
}

void trackAcceleration(){
  lastAcceleration = abs(mpu6050.getAccX()) + abs(mpu6050.getAccY()) + abs(mpu6050.getAccZ());
  
  if(maxFound < lastAcceleration) {
    maxFound = lastAcceleration;
  }
  if(tempMaxAcc < lastAcceleration) {
    tempMaxAcc = lastAcceleration;
  }
}

void printGyro(){
  Serial.print(mpu6050.getAngleX());
  Serial.print(mpu6050.getAngleY());
  Serial.println(mpu6050.getAngleZ());
}

void checkRestrictions(){
  if(maxFound > maxAcc){
    alertAcc = "MAY BE DAMAGED \n";
  }
  if(abs(mpu6050.getAccAngleX()) > 160 && abs(mpu6050.getAccAngleY()) > 160){
    alertFlip = "MAY HAVE BEEN FLIPPED \n";
  }
}

bool ledBlinking = false;
bool ledState = false;
int lastChangedLed = 0;
bool begun = false;
String reply = "";
void loop() {
  display.clear();
  mpu6050.update();
  trackAcceleration();
  //printGyro();
  checkRestrictions();
  display.drawString(0, 0, String("Acc: ") + maxFound + String(" / ") + maxAcc +
  String("\n") + alertAcc + mpu6050.getTemp() + String("C* \n") + alertFlip);
  
  display.display();
  delay(200);
  reply = "";
  while (client.available()) {
    char c = client.read();
    if(c == '{'){
      begun = true;
    } else if(c == '}'){
      begun = false;
    }
    if(begun){
      if(c == '"'){
        reply += "\"";
      } else {
        reply += c;
      }
    }

  }
  if(reply != ""){
    
    DynamicJsonDocument temp(200);
    deserializeJson(temp, reply);
    JsonObject obj = temp.as<JsonObject>();
    serializeJson(obj, Serial);
    maxAcc = obj["test1"];
    flippedDegree = obj["test2"];
    timerDelayPost = obj["test3"];
    ledBlinking = obj["test4"];
  }
  if (ledBlinking) {
    if ((millis() - lastChangedLed) > 400) {
      if(ledState){
        digitalWrite(32, HIGH);  // turn the LED on (HIGH is the voltage level)
      } else {
        digitalWrite(32, LOW);  // turn the LED off by making the voltage LOW
      }
      ledState = !ledState;
    }
  } else {
    digitalWrite(32,LOW);// turn the LED off by making the voltage LOW
  }

  if ((millis() - lastTimePost) > timerDelayPost) {
    if(connect(host, 80)) {
      DynamicJsonDocument doc(1024);
      doc["Temperature"] = mpu6050.getTemp();
      doc["Acceleration"] = tempMaxAcc;
      doc["Flipped"] = (abs(mpu6050.getAccAngleX()) > 160 && abs(mpu6050.getAccAngleY()) > 160);





      location_t loc = location.getGeoFromWiFi();
      
      doc["Lat"] = String(loc.lat, 7);
      doc["Long"] = String(loc.lon, 7);
      doc["Accuracy"] = String(loc.accuracy);

      //The rest of this data is not used, but is sent for future features
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

      tempMaxAcc = 0;

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
  
      serializeJson(doc, Serial);
      serializeJson(doc, client);


      httpGetRequest();
      
    }
    lastTimePost = millis();
  }
}


void httpGetRequest() {
  // close any connection before send a new request.
  // This will free the socket on the WiFi shield
  client.stop();

  // if there's a successful connection:
  if (client.connect(host, 80)) {
    Serial.println("connecting...");
    // send the HTTP PUT request:
    client.println((String)"GET " + getAdr + " HTTP/1.1");
    client.println((String)"Host: " + host);
    client.println("Connection: close");
    client.println();
  } else {
    Serial.println("connection failed");
  }
}
