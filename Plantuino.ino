#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
#include <DHT.h>

//Builtin
const short int BUILTIN_LED1 = 2; //GPIO2

//Pins
const short int TRIG_PIN = D1;
const short int ECHO_PIN = D2;
const short int DHT_SENSOR = D3;
const short int RELAY = D5;
const short int LDR = A0;
const short int SHS = A0;
const short int SHS_LOW_HIGH = D0;

DHT dht(DHT_SENSOR, DHT11);

int tempState = 0;
int humidState = 0;
int soilHumidState = 0;
int lightExposure = 0;
int distance = 0;

//NETWORK INFORMATION
const char* ssid = "SSID";
const char* password = "PASSWORD";

//TCP SERVERSOCKET
const int port = 8085;

WiFiServer wifiServer(port);

void setup() {
  Serial.begin(115200);
 
  pinMode(BUILTIN_LED1, OUTPUT);
  pinMode(RELAY, OUTPUT);
  pinMode(SHS_LOW_HIGH, OUTPUT);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
 

  //Station
  WiFi.mode(WIFI_STA);

  //Initialize the wifi
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(400);
    Serial.print("Connecting to WiFi: ");
    Serial.println(ssid);
    blink();
  }

  Serial.println("WiFi Connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  wifiServer.begin();
  dht.begin();
}

void loop() {
  //Try connecting to wifi
  while (WiFi.status() != WL_CONNECTED) {
    Serial.println("Reconnecting to WiFi");
    delay(400);
    blink();
  }

  WiFiClient client = wifiServer.available();

  if (client) {
    Serial.println("Valind Client Detected");
    while (client.connected()) {
      //returns 0 until something is read
      while (client.available() > 0) {
        Serial.println("Message from Hub available");
        String request = client.readStringUntil('\n');
        Serial.print("Message: ");
        Serial.println(request);

        //create a state json file
        StaticJsonDocument<256> doc;
        //Deserialze the json file with the recieved string
        DeserializationError error = deserializeJson(doc, request);
        if (error) {
          Serial.print("ERROR WITH JSON: ");
          Serial.println(error.c_str());
        } else {
          //Create new json object with the string
          JsonObject obj = doc.as<JsonObject>();
          //extract the varaibles from jsonObj
          int command = obj[String("command")];
          double openWaterFlowSec = obj[String("openWaterFlowSec")];

          if (command == 1003) {
            //Hub polls the plant, requesting all the data.
            client.println("1004::" + getAllPlantData());
          } else if (command == 1005) {
            //Hub sends command to plant that it needs to be watered.
            waterPlant(openWaterFlowSec);
            client.println("1004::" + getAllPlantData());          
          }
        }
      }
    }
  }
}

void blink() {
  digitalWrite(BUILTIN_LED1, LOW);
  delay(200);
  digitalWrite(BUILTIN_LED1, HIGH);
}

String getAllPlantData() {
  humidState = dht.readHumidity();
  tempState = dht.readTemperature();
  soilHumidState = getSoilHumidity();
  distance = calculateDistance();
  
  String s = (soilHumidState + String("::") + humidState + String("::") + tempState + String("::") + lightExposure + String("::") + distance);
  return s;
}

//Will help to save the lifespan of the soilHumiditySensor by using a pin low/high.
int getSoilHumidity() {
  digitalWrite(SHS_LOW_HIGH, HIGH);
  int state = analogRead(SHS);
  digitalWrite(SHS_LOW_HIGH, LOW);
  return state;
}

void waterPlant(double waterSec) {
  Serial.println("WATERING PLANT");
  digitalWrite(RELAY, HIGH);
  delay(waterSec * 1000);
  digitalWrite(RELAY, LOW);
}

int calculateDistance(){
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  

 const unsigned long duration= pulseIn(ECHO_PIN, HIGH);
 int stateDistance = duration/29/2;
 Serial.print("Distance: ");
 Serial.println(stateDistance);
 delay(50);
 return stateDistance;
}
