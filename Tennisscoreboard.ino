
#include <GxEPD2_BW.h>
#include <GxEPD2_3C.h>
#include <Fonts/FreeMonoBold9pt7b.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "config.h"
#include "digits.h"

#define ENABLE_GxEPD2_GFX 0
#define RST_PIN 16

//Displays
GxEPD2_BW<GxEPD2_290_BS, GxEPD2_290_BS::HEIGHT> heimSet1(GxEPD2_290_BS(/*CS=5*/ 25, /*DC=*/ 17, /*RST=*/ -1, /*BUSY=*/ 2)); // DEPG0290BS 128x296, SSD1680
GxEPD2_BW<GxEPD2_290_BS, GxEPD2_290_BS::HEIGHT> gastSet1(GxEPD2_290_BS(/*CS=5*/ 26, /*DC=*/ 17, /*RST=*/ -1, /*BUSY=*/ 0)); // DEPG0290BS 128x296, SSD1680
GxEPD2_BW<GxEPD2_290_BS, GxEPD2_290_BS::HEIGHT> heimSet2(GxEPD2_290_BS(/*CS=5*/ 27, /*DC=*/ 17, /*RST=*/ -1, /*BUSY=*/ 4)); // DEPG0290BS 128x296, SSD1680
GxEPD2_BW<GxEPD2_290_BS, GxEPD2_290_BS::HEIGHT> gastSet2(GxEPD2_290_BS(/*CS=5*/ 14, /*DC=*/ 17, /*RST=*/ -1, /*BUSY=*/ 5)); // DEPG0290BS 128x296, SSD1680
GxEPD2_BW<GxEPD2_290_BS, GxEPD2_290_BS::HEIGHT> heimSet3(GxEPD2_290_BS(/*CS=5*/ 12, /*DC=*/ 17, /*RST=*/ -1, /*BUSY=*/ 19)); // DEPG0290BS 128x296, SSD1680
GxEPD2_BW<GxEPD2_290_BS, GxEPD2_290_BS::HEIGHT> gastSet3(GxEPD2_290_BS(/*CS=5*/ 13, /*DC=*/ 17, /*RST=*/ -1, /*BUSY=*/ 21)); // DEPG0290BS 128x296, SSD1680


// Buttons Interrupts
const uint8_t interruptPinHeim = 34;
const uint8_t interruptPinGast = 35;
const uint8_t interruptPinSet = 32;
const uint8_t interruptPinReset = 33;

bool interruptHeim = false;
bool interruptGast = false;
bool interruptSet = false;
bool interruptReset = false;

void ICACHE_RAM_ATTR ISRoutine0 ();
void ICACHE_RAM_ATTR ISRoutine1 ();
void ICACHE_RAM_ATTR ISRoutine2 ();
void ICACHE_RAM_ATTR ISRoutine3 ();

// Button, Timehandling
const unsigned long debounceDelay = 180;
volatile unsigned long lastDebounceTime = 0;
bool toSend = false;
const unsigned long timeoutToSend = 10000;
volatile unsigned long lastSetButtonPressed = 0;

// Scoreboard
unsigned int gamesSet1Heim;
unsigned int gamesSet1Gast;
unsigned int gamesSet2Heim;
unsigned int gamesSet2Gast;
unsigned int gamesSet3Heim;
unsigned int gamesSet3Gast;

//Set
unsigned int set;

//Wifi
WiFiClient espClient;
PubSubClient client(espClient);

//MQTT
char mqtt[] = "Tennisscoreboard";
char subscribeChannel[] = "Tennisscoreboard/toCourt1";
char publishChannel[] = "Tennisscoreboard/fromCourt1";


void setup()
{

  Serial.begin(115200);
  Serial.println("Es geht los ...");

  pinMode(interruptPinHeim,INPUT_PULLUP);
  pinMode(interruptPinGast,INPUT_PULLUP);
  pinMode(interruptPinSet,INPUT_PULLUP);
  pinMode(interruptPinReset,INPUT_PULLUP);
  
  attachInterrupt(interruptPinHeim,ISRoutine0,FALLING);
  attachInterrupt(interruptPinGast,ISRoutine1,FALLING);
  attachInterrupt(interruptPinSet,ISRoutine2,FALLING);
  attachInterrupt(interruptPinReset,ISRoutine3,FALLING);

  // one common reset for all displays
  digitalWrite(RST_PIN, HIGH);
  pinMode(RST_PIN, OUTPUT);
  delay(20);
  digitalWrite(RST_PIN, LOW);
  delay(20);
  digitalWrite(RST_PIN, HIGH);
  delay(200);
  
  heimSet1.init();
  gastSet1.init();
  heimSet2.init();
  gastSet2.init();
  heimSet3.init();
  gastSet3.init();  

  reset_scoreboard(false);

  //Wifi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  //MQTT
  client.setServer(mqtt_server, 1883);
  client.setCallback(mqtt_callback);
  reconnect();
  
  /*drawTheDigit(heimSet1,1);
  drawTheDigit(gastSet1,2);
  drawTheDigit(heimSet2,3);
  drawTheDigit(gastSet2,4);
  drawTheDigit(heimSet3,5);
  drawTheDigit(gastSet3,6);*/  

 
}


void reset_display(GxEPD2_BW<GxEPD2_290_BS, GxEPD2_290_BS::HEIGHT>& display) {

  Serial.println("Setze auf Null ...");

  display.setRotation(0);
  display.setFullWindow();
  display.firstPage();

  do
  {
    display.fillScreen(GxEPD_WHITE);
    display.drawInvertedBitmap(0, 0,epd_bitmap_0b, 128, 296, GxEPD_BLACK);
  }
  while (display.nextPage());
  
}

void drawTheDigit(GxEPD2_BW<GxEPD2_290_BS, GxEPD2_290_BS::HEIGHT>& display, int digit) {

  Serial.print("Draw the digit: ");
  Serial.println(digit);

  display.setRotation(0);
  display.setPartialWindow(0, 0, display.width(), display.height());
  display.firstPage();

  do
  {
    display.fillScreen(GxEPD_WHITE);
    display.drawInvertedBitmap(0, 0,epd_bitmap_allArray[digit] , 128, 296, GxEPD_BLACK);
  }
  while (display.nextPage());
 
}

void drawTheDigitFull(GxEPD2_BW<GxEPD2_290_BS, GxEPD2_290_BS::HEIGHT>& display, int digit) {

  display.setRotation(0);
  display.setFullWindow();
  display.firstPage();

  do
  {
    display.fillScreen(GxEPD_WHITE);
    display.drawInvertedBitmap(0, 0,epd_bitmap_allArray[digit] , 128, 296, GxEPD_BLACK);
  }
  while (display.nextPage());
 
}

void show_partial_scoreboard(int heimGast, int set, int games ){
  // 0 = heim 
  // 1 = gast
  if ( heimGast == 0 and set == 0 ) { 
     drawTheDigit(heimSet1,games);
  } else if ( heimGast == 1 and set == 0 ) {
    drawTheDigit(gastSet1,games);
  } else if ( heimGast == 0 and set == 1 ) {
    drawTheDigit(heimSet2,games);
  } else if ( heimGast == 1 and set == 1 ) {
    drawTheDigit(gastSet2,games);
  } else if ( heimGast == 0 and set == 2 ) {
    drawTheDigit(heimSet3,games);
  } else if ( heimGast == 1 and set == 2 ) {
    drawTheDigit(gastSet3,games);
  }
  
}  


void setAndShow_scoreboard(int setHeimSet1, int setGastSet1, int setHeimSet2, int setGastSet2, int setHeimSet3, int setGastSet3) {

  if ( gamesSet1Heim != setHeimSet1 ) {
      drawTheDigit(heimSet1, setHeimSet1);
      gamesSet1Heim = setHeimSet1;   
  }
  if ( gamesSet1Gast != setGastSet1 ) {
      drawTheDigit(gastSet1, setGastSet1);
      gamesSet1Gast = setGastSet1;   
  }
  if ( gamesSet2Heim != setHeimSet2 ) {
      drawTheDigit(heimSet2, setHeimSet2);
      gamesSet2Heim = setHeimSet2;   
  }
  if ( gamesSet2Gast != setGastSet2 ) {
      drawTheDigit(gastSet2, setGastSet2);
      gamesSet2Gast = setGastSet2;   
  }
  if ( gamesSet3Heim != setHeimSet3 ) {
      drawTheDigit(heimSet3, setHeimSet3);
      gamesSet3Heim = setHeimSet3;   
  }
  if ( gamesSet3Gast != setGastSet3 ) {
      drawTheDigit(gastSet3, setGastSet3);
      gamesSet3Gast = setGastSet3;   
  }

  reconnect();

}


void reset_scoreboard(bool reset_mqtt) {

  reset_display(heimSet1);
  reset_display(gastSet1);
  reset_display(heimSet2);
  reset_display(gastSet2);
  reset_display(heimSet3);
  reset_display(gastSet3);
  
  gamesSet1Heim = 0;
  gamesSet1Gast = 0;
  gamesSet2Heim = 0;
  gamesSet2Gast = 0;
  gamesSet3Heim = 0;
  gamesSet3Gast = 0;

  set=0;

  // after reset via mqtt the connection seems to be disrupted
  if (reset_mqtt == true ) {
    reconnect();
  }
  
}


void IRAM_ATTR ISRoutine0 () {

   unsigned long currentMillis = millis();
   if (currentMillis - lastDebounceTime >= debounceDelay) {
       interruptHeim = true;
       lastDebounceTime = currentMillis;
       toSend=true;
       lastSetButtonPressed = millis();
   }
     
}

void IRAM_ATTR ISRoutine1 () {

   unsigned long currentMillis = millis();
   if (currentMillis - lastDebounceTime >= debounceDelay) {
       interruptGast = true;
       lastDebounceTime = currentMillis;
       toSend = true;
       lastSetButtonPressed = millis();
   }

}

void IRAM_ATTR ISRoutine2 () {

   unsigned long currentMillis = millis();
   if (currentMillis - lastDebounceTime >= debounceDelay) {
       interruptSet = true;
       lastDebounceTime = currentMillis;
   }
  
 
}

void IRAM_ATTR ISRoutine3 () {

   unsigned long currentMillis = millis();
   if (currentMillis - lastDebounceTime >= debounceDelay) {
       interruptReset = true;
       lastDebounceTime = currentMillis;
   }
  
 
}

// MQTT
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(mqtt) ) {
      Serial.println("connected");
      client.subscribe(subscribeChannel);
    } else {
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}


//MQTT
void mqtt_callback(char* topic, byte* payload, unsigned int length) {

  bool jsonMessageOk=true;

  int setHeimSet1;
  int setGastSet1;
  int setHeimSet2;
  int setGastSet2;
  int setHeimSet3;
  int setGastSet3;

  
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i=0;i<length;i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
  StaticJsonDocument<256> doc;
  deserializeJson(doc, payload, length);
  if ( doc["reset"] == "true" ) {
    reset_scoreboard(true);
  } else {
    if ( ! ( doc["Score"]["HeimSet1"] >= 0 and doc["Score"]["HeimSet1"] <= 7 ) ) {
      jsonMessageOk=false;    
    } else {
      Serial.println("Ok,1");
      setHeimSet1 = doc["Score"]["HeimSet1"];
    }
    if ( ! ( doc["Score"]["GastSet1"] >= 0 and doc["Score"]["GastSet1"] <= 7 )  ) {
      jsonMessageOk=false;    
    } else {
      Serial.println("Ok,2");     
      setGastSet1 = doc["Score"]["GastSet1"];
    }
    if ( ! ( doc["Score"]["HeimSet2"] >= 0 and doc["Score"]["HeimSet2"] <= 7 ) ) {
      jsonMessageOk=false;    
    } else {
      Serial.println("Ok,3");
      setHeimSet2 = doc["Score"]["HeimSet2"];
    }
    if ( ! ( doc["Score"]["GastSet2"] >= 0 and doc["Score"]["GastSet2"] <= 7 ) ) {
      jsonMessageOk=false;    
    } else {
      Serial.println("Ok,4");
      setGastSet2 = doc["Score"]["GastSet2"];
    }
    if ( ! ( doc["Score"]["HeimSet3"] >= 0 and doc["Score"]["HeimSet3"] <= 9 ) ) {
      jsonMessageOk=false;    
    } else {
      Serial.println("Ok,5");
      setHeimSet3 = doc["Score"]["HeimSet3"];
    }
    if ( ! ( doc["Score"]["GastSet3"] >= 0 and doc["Score"]["GastSet3"] <= 9 ) ) {
      jsonMessageOk=false;    
    } else {
      Serial.println("Ok,6");
      setGastSet3 = doc["Score"]["GastSet3"];
    }

    Serial.println(jsonMessageOk);
    if ( jsonMessageOk == true )  {
      Serial.println("Json message ok, sending to scoreboard: ");
      setAndShow_scoreboard(setHeimSet1,setGastSet1,setHeimSet2,setGastSet2,setHeimSet3,setGastSet3);
    }
  }

}

void loop() {
 
 if ( interruptHeim == true ) {

  Serial.println("Interupt 1");
  if ( set == 0 ) { 
    gamesSet1Heim++;
    gamesSet1Heim = gamesSet1Heim %8;

    show_partial_scoreboard(0, set,gamesSet1Heim);
    } else if ( set == 1 ) {
    gamesSet2Heim++;
    gamesSet2Heim = gamesSet2Heim %8;

    show_partial_scoreboard(0, set,gamesSet2Heim);
    } else if ( set == 2 ) {
    gamesSet3Heim++;
    gamesSet3Heim = gamesSet3Heim %10;

    show_partial_scoreboard(0, set,gamesSet3Heim);   
    }
    
  interruptHeim = false;

 }

 if ( interruptGast == true ) {

  Serial.println("Interupt 2");

  if ( set == 0 ) { 
    gamesSet1Gast++;
    gamesSet1Gast = gamesSet1Gast %8;

    show_partial_scoreboard(1, set,gamesSet1Gast);
    } else if ( set == 1 ) {
    gamesSet2Gast++;
    gamesSet2Gast = gamesSet2Gast %8;

    show_partial_scoreboard(1, set,gamesSet2Gast);
    } else if ( set == 2 ) {
    gamesSet3Gast++;
    gamesSet3Gast = gamesSet3Gast %10;

    show_partial_scoreboard(1, set,gamesSet3Gast);   
    }
    
  interruptGast = false;

    
 }

 if ( interruptSet == true ) {

  Serial.println("Interupt 3");

  set++;
  set = set%3;
  if ( set == 0 ) {
    drawTheDigitFull(heimSet1,gamesSet1Heim);
  } else if ( set == 1 ) {
    drawTheDigitFull(heimSet2,gamesSet2Heim); 
  } else if ( set == 2 ) {
    drawTheDigitFull(heimSet3,gamesSet3Heim);
  }
  interruptSet = false;
  
 }

 if ( interruptReset == true ) {

  Serial.println("Interupt 4");
  reset_scoreboard(true);
  interruptReset = false;
  
 }

  if ( toSend == true ) {
    if ( ( millis() - lastSetButtonPressed ) > timeoutToSend ) {
    Serial.println("Publishing Score ...");
    toSend=false;
    
    const int capacity = JSON_OBJECT_SIZE(6);
    StaticJsonDocument<capacity> json_to_mqtt;
    json_to_mqtt["HeimSet1"] = gamesSet1Heim;
    json_to_mqtt["GastSet1"] = gamesSet1Gast;    
    json_to_mqtt["HeimSet2"] = gamesSet2Heim;
    json_to_mqtt["GastSet2"] = gamesSet2Gast; 
    json_to_mqtt["HeimSet3"] = gamesSet3Heim;
    json_to_mqtt["GastSet3"] = gamesSet3Gast;
    client.publish(publishChannel, (char *) json_to_mqtt.as<String>().c_str() );
    }
  }
  
  // put your main code here, to run repeatedly:
  client.loop();
}
