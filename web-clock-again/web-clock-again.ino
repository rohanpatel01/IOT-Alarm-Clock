/*
  TODO:

  get multiple devices to connect
    - allow unit board to recieve messages via esp now during pairing phase in first few seconds of powering on
      - issue with esp_not not being initialized
      - issue with WS? but WS not needed for esp now communication
      - app peer function should send message from front end to mother board that requested it
        then mother board should send message via esp now signalling that it is trying to pair with it / send it a message

      - rather than playing game with all boards just make it so mother board and units share their index in mac address table
        so we only send to paired boards
        - need to change the sendDataToAllPeers() function

  reconfigure so wifi and esp_now setup are in setup and not loop - if this is possible

  
*/

#include <Arduino.h>
#include <WebSocketsClient.h>
#include <WiFiMulti.h>
#include <esp_now.h>
#include <TM1637.h>
#include <ArduinoJson.h>

#define WIFI_SSID "Ronnie" // MRK
#define WIFI_PASSWORD "Something Simple" // MRK9004# // Flimsy-Hound-21@

#define WS_HOST "75kun89lml.execute-api.us-east-2.amazonaws.com"
#define WS_PORT 443
#define WS_URL "/dev"

#define JSON_DOC_SIZE 2048
#define MSG_SIZE 256

WiFiMulti wifiMulti;
WebSocketsClient wsClient;

// variables for keeping local time and alarm time
uint8_t currentHour = 99;
uint8_t currentMinute = 99;
bool isCurrentAm = false;

uint8_t alarmHour = 0;
uint8_t alarmMinute = 0;
bool isAlarmAm = false;
uint32_t wsInit = true;

bool isMother = true;

uint32_t oldtime = millis();
uint32_t count = 0;

int CLK = 22;
int DIO = 4;
TM1637 tm(CLK,DIO);

// variables for alarm game

typedef struct struct_message {
    bool alarmTrue; // true if currentTime = alarmTime
    bool incoming;
    int hour;
    int minute;
    int numAlarmPressedInfo;
} struct_message;

int thisMacIndex = 0;
bool gameStarted = false;
struct_message incomingReadings;
struct_message ESP_NOW_STRUCT;
esp_now_peer_info_t peerInfo;

// Define variables to store incoming readings
bool incoming = false;
bool incomingAlarmTrue;
int incomingHour = 0;
int incomingMinute = 0;
int numAlarmPressed = 0;
String success;

int numESPSTest = 2; // change this 

uint8_t allAddresses[3][6] = { {0x78, 0x21, 0x84, 0x9D, 0x40, 0xD0}, 
                          {0x78, 0x21, 0x84, 0x9C, 0xFA, 0x84},
                          {0x78, 0x21, 0x84, 0x9D, 0x0D, 0x7C}
                          };

uint8_t addressToSendTo[6];

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
  
  if (status ==0){
    success = "Delivery Success :)";
  }
  else{
    success = "Delivery Fail :(";
  }

}

void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpy(&incomingReadings, incomingData, sizeof(incomingReadings));
  incoming = incomingReadings.incoming;
  incomingAlarmTrue = incomingReadings.alarmTrue;
  incomingHour = incomingReadings.hour;
  incomingMinute = incomingReadings.minute;
  numAlarmPressed = incomingReadings.numAlarmPressedInfo;

  Serial.println(incomingReadings.numAlarmPressedInfo);

  if ((incomingHour != 0) || (incomingMinute != 0))
  {
    displayTime(incomingHour, incomingMinute);
  }

  Serial.println("Incoming!!");
  digitalWrite(13, 1);

}

// assuming time is already changed to normal time (not military) as input
// but can display otherwise but just recommended as such for formatting / consistency
void displayTime(int hour, int minute){

  Serial.println("Displaying Time");

  // % 10 takes the last digit
  // / 10 takes the front digit
  tm.point(1);
  tm.display(1, hour % 10);       // hour ones place
  tm.display(0, hour / 10 % 10);    // hour tens place
  tm.display(3, minute % 10);      // minute ones place
  tm.display(2, minute / 10 % 10); // minute tens place 
}

void alarm() {
  if ( (currentHour == alarmHour) && (currentMinute == alarmMinute) && (isCurrentAm == isAlarmAm)) {

    Serial.println("Alarm");
    // send message to other board
    // while(true){
    //   // if (numAlarmPressed >= 6){
    //   //   return;
    //   // }
    // }
  }

}

void sendDataToAllPeers() {
  for (int i = 0; i < numESPSTest; i++)
  {
    if (thisMacIndex == i)
    {
      continue;
    }
    Serial.println("Contacted ");
    Serial.println(i);

    esp_err_t result = esp_now_send(allAddresses[i], (uint8_t *) &ESP_NOW_STRUCT, sizeof(ESP_NOW_STRUCT));
  }
}

int findMacAddress() {
  String macAddress = WiFi.macAddress();
  int sLength = macAddress.length();

  // first digit
  char lastHexMacAddressFirstDigit = macAddress[sLength-2];
  int lastHexMacAddressFirstDigitInt = convertCharToHEX(lastHexMacAddressFirstDigit);
  
  // last digit
  char lastHexMacAddressSecondDigit = macAddress[sLength-1];
  int lastHexMacAddressSecondDigitInt = convertCharToHEX(lastHexMacAddressSecondDigit);
  
  // convert Hex value to decimal
  int thisEsp_TotalHexValue = (lastHexMacAddressFirstDigitInt * 16) + (lastHexMacAddressSecondDigitInt);
  Serial.println(thisEsp_TotalHexValue);

  // find our address in list of addresses
  for (int i = 0; i < 3; i++)
  {
    // returns decimal value of last hex from current address we're viewing in 2D array
    int currentLoopAddress = allAddresses[i][5]; // decimal value from hex
    
    if (currentLoopAddress == thisEsp_TotalHexValue)
    {
      Serial.print("Home MAC FOUND!!!");
      return i;
    }
  }
 
  Serial.print("Bad Comparison; Home MAC not found");
  return 0;  
}

int convertCharToHEX(char inputChar) {
    switch (inputChar) 
    {
      case 'A':
        return 10;
        
      case 'B':
        return 11;
        
      case 'C':
        return 12;
        
      case 'D':
        return 13;
        
      case 'E':
        return 14;
        
      case 'F':
        return 15;
        
      default:
        return inputChar - '0'; // will return int value for char
    }
}

void sendErrorMessage(const char *error) {
  char msg[MSG_SIZE];

  sprintf(msg, "{\"action\":\"msg\",\"type\":\"error\",\"body\":\"%s\"}",
          error);

  wsClient.sendTXT(msg);
}

void sendConnectionMessage(){
  wsClient.sendTXT("{\"action\":\"msg\",\"type\":\"status\",\"body\":\"connectedESP\"}");
}

void sendOkMessage() {
  wsClient.sendTXT("{\"action\":\"msg\",\"type\":\"status\",\"body\":\"ok\"}");
}

uint8_t toMode(const char *val) {
  if (strcmp(val, "output") == 0) {
    return OUTPUT;
  }

  if (strcmp(val, "input_pullup") == 0) {
    return INPUT_PULLUP;
  }

  return INPUT;
}

void handleMessage(uint8_t *payload) {
  StaticJsonDocument<JSON_DOC_SIZE> doc;

  DeserializationError error = deserializeJson(doc, payload);

  // Test if parsing succeeds.
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    // sendErrorMessage(error.c_str());
    return;
  }

  if (!doc["type"].is<const char *>()) {
    // sendErrorMessage("invalid message type format");
    return;
  }
 
  // process current time and alarm time
  if (strcmp(doc["type"], "info") == 0) {                      // set current time for display
    if (strcmp(doc["body"]["type"], "currentTime") == 0) {

      JsonObject timeDoc = doc["body"];
      long hour = timeDoc["bodyCurrentHour"];
      long minute = timeDoc["bodyCurrentMinute"];
      currentMinute = minute;

      if (hour > 12) { 
        isCurrentAm = false;
        currentHour = (hour - 12); // need to convert hour so not in milirary time
        
      } else if (hour >= 12) {
        isCurrentAm = false; // time is PM if 12 or later during day
        currentHour = hour; 
      } else {
        isCurrentAm = true; // hour is before 12 thus in the am
        currentHour = hour; 
      }
      
      displayTime(currentHour, currentMinute); // display current time
      Serial.println("Displayed time from server");

      // send time data to other units
      ESP_NOW_STRUCT.alarmTrue = false;
      ESP_NOW_STRUCT.hour = currentHour;
      ESP_NOW_STRUCT.minute = currentMinute;
      // ESP_NOW_STRUCT.numAlarmPressedInfo = 0; // see if need to send this over yet
      // sendDataToAllPeers(); // uncomment when display time works

      Serial.println("isCurrentAm");
      Serial.println(isCurrentAm);

    } else if (strcmp(doc["body"]["type"], "alarmTime") == 0) {   // set alarm time for alarm
      JsonObject timeDoc = doc["body"];
      long hour = timeDoc["bodyAlarmHour"];
      long minute = timeDoc["bodyAlarmMinute"];
      alarmMinute = minute;

      if (hour > 12) { 
        isAlarmAm = false;
        alarmHour = (hour - 12);

      } else if (hour >= 12) {
        isAlarmAm = false;
        alarmHour = hour;
      } else {
        isAlarmAm = true;
        alarmHour = hour;
      }
    
      Serial.println("isAlarmAm");
      Serial.println(isAlarmAm);

    } else if (strcmp(doc["body"]["type"], "addPeer") == 0) {
      ESP_NOW_STRUCT.alarmTrue = false;
      ESP_NOW_STRUCT.incoming = true;
      ESP_NOW_STRUCT.hour = currentHour;
      ESP_NOW_STRUCT.minute = currentMinute;
      // ESP_NOW_STRUCT.numAlarmPressedInfo = 0;

      sendDataToAllPeers();
      Serial.println("add Peer from front end, esp delivered");
       
    }
  }

  if (strcmp(doc["type"], "cmd") == 0) {
    if (!doc["body"].is<JsonObject>()) {
      // sendErrorMessage("invalid command body");
      return;
    }

    if (strcmp(doc["body"]["type"], "pinMode") == 0) {
      // comment here was for better validation for pin mode - just visit his repo if need this
      pinMode(doc["body"]["pin"], toMode(doc["body"]["mode"]));
      sendConnectionMessage();
      // sendOkMessage();
      return;
      
    }

    if (strcmp(doc["body"]["type"], "digitalWrite") == 0) {
      digitalWrite(doc["body"]["pin"], doc["body"]["value"]);
      wsClient.sendTXT("{\"action\":\"msg\",\"type\":\"status\",\"body\":\"ok\"}");
      String espID = "ESP" + String(thisMacIndex);
      wsClient.sendTXT(espID); // forbidden

      sendOkMessage();
      return;
    }
    // respond to WS command from react for digitalRead
    if (strcmp(doc["body"]["type"], "digitalRead") == 0) {
      auto value = digitalRead(doc["body"]["pin"]);

      char msg[MSG_SIZE];
      // send message back to react front end for digital read
      sprintf(msg, "{\"action\":\"msg\",\"type\":\"output\",\"body\":%d}",
              value);

      wsClient.sendTXT(msg);
      return;
    }

    // sendErrorMessage("unsupported command type");
    return;
  }

  // sendErrorMessage("unsupported message type");
  return;
}

void onWSEvent(WStype_t type, uint8_t *payload, size_t length) {
  switch (type) {
  case WStype_CONNECTED:
    Serial.println("WS Connected");
    break;
  case WStype_DISCONNECTED:
    Serial.println("WS Disconnected");
    break;
  case WStype_TEXT:
    Serial.printf("WS Message: %s\n", payload);

    handleMessage(payload);

    break;
  }
}

void ESP_INIT() {
  Serial.println("Setting up ESP_NOW");
  WiFi.mode(WIFI_STA); 
  thisMacIndex = findMacAddress();

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK)
  {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Register Peers
  for (int i = 0; i < numESPSTest; i++)
  {
    // skip registering peer if i = thisBoard
    if (i == thisMacIndex)
      continue;

    // register peer
    memcpy(peerInfo.peer_addr, allAddresses[i], 6); 
    peerInfo.channel = 0;  
    peerInfo.encrypt = false;
    
    // Add peer success      
    if (esp_now_add_peer(&peerInfo) != ESP_OK)
    {
      Serial.println("Failed to add peer");
      return;
    }    
  }
  
  esp_now_register_send_cb(OnDataSent);
  esp_now_register_recv_cb(OnDataRecv);

  Serial.println("Done with ESP_NOW setup");

}

void setup() {
  Serial.begin(9600);
  pinMode(LED_BUILTIN, OUTPUT);
  tm.init();
  tm.set(5);

  // setup ESP_NOW communication
  ESP_INIT();
  
}

void waitForConnection() {

  Serial.println("waiting for connection");

  while (count < 5)  {

    if ( (millis() - oldtime) > 1000) {
      oldtime = millis();
      count++;
      Serial.println(count);
    }

    if (incoming) {
      isMother = false;
      Serial.println("is not mother");
      Serial.println("is not mother");
      Serial.println("is not mother");
      Serial.println("is not mother");
      Serial.println("is not mother");

    }
  }

  // while (true) {
  //   if (incoming) {
  //     Serial.println("is not mother");
  //     Serial.println("is not mother");
  //     Serial.println("is not mother");
  //     Serial.println("is not mother");
  //     Serial.println("is not mother");
  //   }
  // }

}

void loop() {

  // wait to recieve incoming message saying this is a unit
  // if ( (count < 5) && (isMother) ) {
    // waitForConnection();
  // }

  if (isMother) {

    // initialize mother unit to connect to WS
    if (wsInit){
      wifiMulti.addAP(WIFI_SSID, WIFI_PASSWORD);

      while (wifiMulti.run() != WL_CONNECTED) {
        delay(100);
        Serial.println("attempting WS connection");
      }

      Serial.println("WIFI Connected");
      wsClient.beginSSL(WS_HOST, WS_PORT, WS_URL, "", "wss");
      wsClient.onEvent(onWSEvent);

      wsInit = false;
      Serial.println("wsINIT");
      sendConnectionMessage();
    }

    // handle everything
    digitalWrite(LED_BUILTIN, WiFi.status() == WL_CONNECTED);
    wsClient.loop();
    alarm();

  } else {

    // units should only recieve time and worry about the game
    Serial.println("unit");
    delay(1000);

    // ESP_NOW_STRUCT.incoming = true;
    // esp_err_t result = esp_now_send(allAddresses[1], (uint8_t *) &ESP_NOW_STRUCT, sizeof(ESP_NOW_STRUCT));
    // delay(5000);


  }
}