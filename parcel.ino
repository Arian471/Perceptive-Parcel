
#include <MPU6050_tockn.h>
#include <Wire.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include "SSD1306.h" 
#include <WifiLocation.h>


const char* ssid = "SSID";
const char* password = "PASSWORD";

const char* host = "Server URL";
const char* resource = "/sensors";
const char* getAdr = "/sensors/test";

WiFiClient client;

const char* googleApiKey = "KEY";
WifiLocation location(googleApiKey);

SSD1306  display(0x3c, 25, 26);

String configurationData;
float configurationDataArr[2];

int timerDelayPost = 5000;
long lastTime = 0;

MPU6050 mpu6050(Wire);
float maxAcc = 2;
float flippedDegree = 110;
String alertAcc = "";
String alertFlip = "";
float lastAcceleration;
float maxFound = 0;
float tempMaxAcc = 0;


bool ledBlinking = false;
bool ledState = false;
int lastChangedLed = 0;
bool begun = false;

String reply = "";

void setup() {
  Serial.begin(115200);
  display.init();
  Wire.begin(25, 26);//The I2C connection on which the MPU6050 is conected
  mpu6050.begin();
  mpu6050.calcGyroOffsets(true); //calibrates the gyroscope to minimize drift

  pinMode(32, OUTPUT);//this is the led pin
   
  WiFi.begin(ssid, password);
  Serial.println("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to WiFi network");
}

bool connect(const char* hostName, int portNumber) {
  bool ok = client.connect(hostName, portNumber);//connects to the server
  Serial.println(ok ? "Connected" : "Connection Failed!");
  return ok;
}

void trackAcceleration(){
  lastAcceleration = abs(mpu6050.getAccX()) + abs(mpu6050.getAccY()) + abs(mpu6050.getAccZ());
  //the sum of the absolute acceleration in all 3 axis
  
  if(maxFound < lastAcceleration) {
    maxFound = lastAcceleration;
  }
  if(tempMaxAcc < lastAcceleration) {//stores the largest acceleration since the data was last shared with the server
    tempMaxAcc = lastAcceleration;
  }
}

void checkRestrictions(){
  if(maxFound > maxAcc){
    alertAcc = "MAY BE DAMAGED \n";
  }
  if(abs(mpu6050.getAccAngleX()) > flippedDegree && abs(mpu6050.getAccAngleY()) > flippedDegree){
    alertFlip = "MAY HAVE BEEN FLIPPED \n";
  }
}

void loop() {
  display.clear();
  mpu6050.update();
  trackAcceleration();
  checkRestrictions();
  display.drawString(0, 0, String("Acc: ") + maxFound + String(" / ") + maxAcc +
  String("\n") + alertAcc + mpu6050.getTemp() + String("C* \n") + alertFlip);
  
  display.display();
  
  //converts the slowly incomming message from the sercer to a string
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
  //convert that string to a JSON object with all the data that is needed
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
  

  if ((millis() - lastTime) > timerDelayPost) {
    if(connect(host, 80)) {
      DynamicJsonDocument doc(1024);
      doc["Temperature"] = mpu6050.getTemp();
      doc["Acceleration"] = tempMaxAcc;
      doc["Flipped"] = (abs(mpu6050.getAccAngleX()) > flippedDegree && abs(mpu6050.getAccAngleY()) > flippedDegree);

      location_t loc = location.getGeoFromWiFi();//get the location using the WiFi networks nearby and Google's Geolocation API
      doc["Lat"] = String(loc.lat, 7);
      doc["Long"] = String(loc.lon, 7);
      doc["Accuracy"] = String(loc.accuracy);

      //The rest of this data is not used, but is sent in case it is needed in the future
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
      
      //send a post request with all the data we want to share from the parcel
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
    lastTime = millis();
  }
}


void httpGetRequest() {
  //make sure there are no other connections
  client.stop();

  // if there's a successful connection:
  if (client.connect(host, 80)) {
    //send the GET request
    client.println((String)"GET " + getAdr + " HTTP/1.1");
    client.println((String)"Host: " + host);
    client.println("Connection: close");
    client.println();
  } else {
    Serial.println("connection failed for GET request");
  }
}
