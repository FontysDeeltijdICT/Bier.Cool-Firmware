/*********
  Present pictures on oled during boot and show ambient and object temperature
  API json formatting, hand made json
*********/
// Importing libraries

#include <OneWire.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <DallasTemperature.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <Adafruit_I2CDevice.h>

// GPIO where the DS18B20 is connected to
const int oneWireBus = 4;          
// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(oneWireBus);
// Pass our oneWire reference to Dallas Temperature sensor 
DallasTemperature sensors(&oneWire);
// Define addresses for sensors, add or comment out if needed. Use template to gather IDs.
DeviceAddress sensor1 = { 0x28, 0xE2, 0x1B, 0x77, 0x91, 0x6, 0x2, 0xEE };
// DeviceAddress sensor2 = { 0x28, 0x17, 0x24, 0x77, 0x91, 0x4, 0x2, 0xD };
// DeviceAddress sensor3= { 0x28, 0xFF, 0xA0, 0x11, 0x33, 0x17, 0x3, 0x96 };

// MQTT server
const char* mqtt_server = "192.168.2.10";
const char* topic = "beers/v1";    // this is the [root topic]

// WiFi settings
const char* ssid = "<Wi-Fi-Network-Here";
const char* pswd = "<Really-Hard-To-Guess_Password-Here!";

// Variable used for timing of messages, 10.000 = 10 seconds
long timeBetweenMessages = 10000;

// Create objects
WiFiClient espClient;
ESP8266WebServer server(80);
PubSubClient client(espClient);

// Additional variables
long lastMsg = 0;
int value = 0;
int status = WL_IDLE_STATUS;     // the starting Wifi radio's status
float temp0;
float temp1;
//String something;

// Method for setting up Wi-Fi
void setup_wifi() {
  delay(10);

  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, pswd);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

// 
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // Switch on the LED if an 1 was received as first character
  if ((char)payload[0] == '1') {
    digitalWrite(BUILTIN_LED, LOW);   // Turn the LED on (Note that LOW is the voltage level
    // but actually the LED is on; this is because
    // it is acive low on the ESP-01)
  } else {
    digitalWrite(BUILTIN_LED, HIGH);  // Turn the LED off by making the voltage HIGH
  }
}

// Method for getting MAC address and convert to string
String macToStr(const uint8_t* mac)
{
  String result;
  for (int i = 0; i < 6; ++i) {
    result += String(mac[i], 16);
    if (i < 5)
      result += ':';
  }
  return result;
}

// Method for composing hostname
String composeClientID() {
  //uint8_t mac[6];
  uint8_t* mac[6];
  String something;
  
  something = WiFi.macAddress();
  String clientId;
  //clientId += "esp-";
 // clientId += macToStr(mac);
  return something;
  
}

// Method for reconnecting to MQTT
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");

    String clientId = composeClientID() ;
    clientId += "-";
    clientId += String(micros() & 0xff, 16); // to randomise. sort of

    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish(topic, ("connected " + composeClientID()).c_str() , true );
      // ... and resubscribe
      // topic + clientID + in
      String subscription;
      subscription += topic;
      subscription += "/";
      subscription += composeClientID() ;
      subscription += "/in";
      client.subscribe(subscription.c_str() );
      Serial.print("subscribed to : ");
      Serial.println(subscription);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.print(" wifi=");
      Serial.print(WiFi.status());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}


// Methods for API calls
// Method for /helloWorld GET
void getHelloWord() {
    DynamicJsonDocument doc(512);
    doc["name"] = "Hello world";

    Serial.print(F("Stream..."));
    String buf;
    serializeJson(doc, buf);
    server.send(200, "application/json", buf);
    Serial.println(F("done."));
}

// Method for /getTemperatures GET
void getTemperatures() {
    DynamicJsonDocument doc(512);
    String ambientTemp;
    String objectTemp;
    
    ambientTemp.concat(sensors.getTempCByIndex(0));
    objectTemp.concat(sensors.getTempCByIndex(1));
    
    doc["ambientTemperature"] = ambientTemp;
    doc["objectTemperature"] = objectTemp;
    
    Serial.print(F("Stream..."));
    String buf;
    serializeJson(doc, buf);
    server.send(200, "application/json", buf);
    Serial.println(F("done."));
}

// Method for serving settings
// Call url: http://192.168.2.79/settings?signalStrength=true&chipInfo=true&freeHeap=true
void getSettings() {
      // Allocate a temporary JsonDocument
      // StaticJsonDocument<512> doc;
      DynamicJsonDocument doc(512);
 
      doc["ip"] = WiFi.localIP().toString();
      doc["gw"] = WiFi.gatewayIP().toString();
      doc["nm"] = WiFi.subnetMask().toString();
 
      if (server.arg("signalStrength")== "true"){
          doc["signalStrengh"] = WiFi.RSSI();
      }
 
      if (server.arg("chipInfo")== "true"){
          doc["chipId"] = ESP.getChipId();
          doc["flashChipId"] = ESP.getFlashChipId();
          doc["flashChipSize"] = ESP.getFlashChipSize();
          doc["flashChipRealSize"] = ESP.getFlashChipRealSize();
      }
      if (server.arg("freeHeap")== "true"){
          doc["freeHeap"] = ESP.getFreeHeap();
      }
 
      Serial.print(F("Stream..."));
      String buf;
      serializeJson(doc, buf);
      server.send(200, F("application/json"), buf);
      Serial.println(F("done."));
}


void setRoom() {
    String postBody = server.arg("plain");
    Serial.println(postBody);
 
    DynamicJsonDocument doc(512);
    DeserializationError error = deserializeJson(doc, postBody);
    if (error) {
        // if the file didn't open, print an error:
        Serial.print(F("Error parsing JSON "));
        Serial.println(error.c_str());
 
        String msg = error.c_str();
 
        server.send(400, F("text/html"),
                "Error in parsin json body! <br>" + msg);
 
    } else {
        JsonObject postObj = doc.as<JsonObject>();
 
        Serial.print(F("HTTP Method: "));
        Serial.println(server.method());
 
        if (server.method() == HTTP_POST) {
            if (postObj.containsKey("name") && postObj.containsKey("type")) {
 
                Serial.println(F("done."));
 
                // Here store data or doing operation

                // Create the response
                DynamicJsonDocument doc(512);
                doc["status"] = "OK";
 
                Serial.print(F("Stream..."));
                String buf;
                serializeJson(doc, buf);
 
                server.send(201, F("application/json"), buf);
                Serial.println(F("done."));
 
            }else {
                DynamicJsonDocument doc(512);
                doc["status"] = "KO";
                doc["message"] = F("No data found, or incorrect!");
 
                Serial.print(F("Stream..."));
                String buf;
                serializeJson(doc, buf);
 
                server.send(400, F("application/json"), buf);
                Serial.println(F("done."));
            }
        }
    }
}
 
// Set routing
void restServerRouting() {
    server.on("/", HTTP_GET, []() {
        server.send(200, F("text/html"),
            F("Welcome to the REST Web Server"));
    });
    // Handle get requests
    server.on(F("/helloWorld"), HTTP_GET, getHelloWord);
    server.on(F("/settings"), HTTP_GET, getSettings);
    server.on(F("/getTemperatures"), HTTP_GET, getTemperatures);

    // Handle post requests
    server.on(F("/setRoom"), HTTP_POST, setRoom);
}

// Manage not found URL
void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

String IpAddress2String(const IPAddress& ipAddress)
{
    return String(ipAddress[0]) + String(".") +
           String(ipAddress[1]) + String(".") +
           String(ipAddress[2]) + String(".") +
           String(ipAddress[3]);
}

void setup() {
  pinMode(BUILTIN_LED, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  // Turn of blue led
  digitalWrite(BUILTIN_LED, HIGH);

  Serial.begin(115200);

  // Initialize temp sensor
  sensors.begin();


  // Setting up Wi-FI
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pswd);
  Serial.println("");
 
    // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  
   // Activate mDNS this is used to be able to connect to the server
  // with local DNS hostmane esp8266.local
  if (MDNS.begin("esp8266")) {
    Serial.println("MDNS responder started");
    delay(1000);
  }
 
  // Set server routing
  restServerRouting();
  
  // Set not found response
  server.onNotFound(handleNotFound);

  // Start server
  server.begin();
  Serial.println("HTTP server started");
  
  
  // Connecting to MQTT server
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback); 
  delay(1000);

}


// Counter to track loops
int i = 0;
void loop() {
  // Server ready
  server.handleClient();

  // confirm still connected to mqtt server
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  
  // Display ambient and object temperture
 i++;
  
  sensors.requestTemperatures(); 
  // Temperature in Celsius degrees
  temp0 = sensors.getTempCByIndex(0); 
  temp1 = sensors.getTempCByIndex(1); 
  Serial.print("Ambient = "); 
  Serial.print(temp0); 
  Serial.print("*C\tObject = "); 
  Serial.print(temp1); 
  Serial.println("*C");

  long now = millis();
  if (now - lastMsg > timeBetweenMessages ) {
    lastMsg = now;
    ++value;


    // Composing MQTT message
    String payload = "[{\"sensor\":";
    payload += "1";
    payload += ",\"temperature\":";
    payload += sensors.getTempCByIndex(0);
    payload += ",\"presence\":";
    payload += "true";
    payload += "}";
    payload += ",{\"sensor\":";
    payload += "2";
    payload += ",\"temperature\":";
    payload += "4.5";
    payload += ",\"presence\":";
    payload += "true";

    
    
    payload += "}]";

    // Composing published topic
    String pubTopic;
    pubTopic += topic ;
    pubTopic += "/";
    pubTopic += composeClientID();
    //pubTopic += "/out";

    Serial.print("Publish topic: ");
    Serial.println(pubTopic);
    Serial.print("Publish message: ");
    Serial.println(payload);

    // Publish data to MQTT server
    client.publish( (char*) pubTopic.c_str() , (char*) payload.c_str(), true );
  }
  delay(1000);
}