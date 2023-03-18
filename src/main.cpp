
#include <Arduino.h>
#include <SPI.h>
#include <ESP8266WiFi.h> 
#include <PubSubClient.h>  
#include <ArduinoOTA.h>
#include <PCF8574.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "PCF8591.h" // https://github.com/overbog/PCF8591

// *************************************************************** Funktionen deklarieren
void callback                 (char* topic, byte* payload, unsigned int length);
void reconnect                ();
void printLongRight           (byte, long);
void setup                    ();
void temperartur_schreiben    ();
void wasserdruck_messen       ();
void lichtsensor_ost          ();


// *************************************************************** PCF8574 Adresse
PCF8574 pcf8574(0x20);
//PCF8574 pcf8574_2(0x21);
// SCL - D1
// SDA - D2
// pcf8574.digitalWrite(P, !LOW);


// *************************************************************** pcf8591 Adresse A/D Wandler
// A/D Wandler
// -> https://bre.is/96fuhDtF
//ARDUINO AND PCF8591 EXAMPLE
// -> https://bre.is/qsNvgSUp 
PCF8591 pcf8591(0x48,0);
// PCF8591 pcf8591(0x48,0);
// SCL - D1
// SDA - D2

// *************************************************************** Variablen für Temp Aufbereitung


// *************************************************************** Kartendaten 
const char* kartenID = "WW-Steuerung-Keller";


///////////////////////////////////////////////////////////////////// WIRE Bus
#define ONE_WIRE_BUS D5
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// Temp. Sensor HEX zuweisen
DeviceAddress temp_keller         = { 0x28, 0xAA, 0x6C, 0x90, 0x16, 0x13, 0x02, 0xC2 }; 
const char* topic_keller          = "Temperatur/Raum_WW_Keller";

DeviceAddress temp_kollektor      = { 0x28, 0xAA, 0x2A, 0x9E, 0x16, 0x13, 0x02, 0xA2 }; 
const char* topic_kollektor       = "Temperatur/Kollektor_oben";

DeviceAddress temp_aussen_osten   = { 0x28, 0xFF, 0x26, 0x95, 0xA1, 0x16, 0x04, 0x36 };
const char* topic_aussen_ost      = "Temperatur/Haus/Ost";

DeviceAddress temp_ww_puffer_oben = { 0x28, 0xFF, 0x3B, 0x91, 0xA1, 0x16, 0x05, 0xF6 };
const char* topic_puffer_oben     = "Temperatur/WW_Puffer_oben";

//DeviceAddress temp_vl_ph          = { 0x28, 0xAA, 0x2A, 0x9E, 0x16, 0x13, 0x02, 0xA2 };
//const char* topic_vl_ph           = "Temperatur/vl_ph";

DeviceAddress temp_rl_ph          = { 0x28, 0x8F, 0x51, 0x64, 0x1B, 0x13, 0x01, 0x7A };
const char* topic_rl_ph           = "Temperatur/vl_ph";

DeviceAddress temp_wasser_hahn    = { 0x28, 0x9F, 0xEB, 0x70, 0x1B, 0x13, 0x01, 0xE5 };
const char* topic_wasser_hahn     = "Temperatur/Wasserhahn";

DeviceAddress temp_vl_ww_puffer   = { 0x28, 0x8F, 0x55, 0x88, 0x1B, 0x13, 0x01, 0x3E };
const char* topic_vl_ww_puffer    = "Temperatur/vl_ww_puffer";
// *************************************************************** Temp. Sensor Messwert Variablen
char stgFromFloat[10];
char msgToPublish[60];
char textTOtopic[60];

// ***************************************************************///////////// Intervall der Steuerung
unsigned long letzteMillis_Temperatur = 0;
unsigned long interval_Temperatur = 5000; 

unsigned long letzteMillis_Wasserdruck= 0;
unsigned long interval_Wasserdruck= 1000; 

unsigned long letzteMillis_lichtstaerke= 0;
unsigned long interval_lichtstaerke = 1000; 
// ***************************************************************/////////////


WiFiClient espClient;
PubSubClient client(espClient);

// Connect to the WiFi
const char* ssid = "GuggenbergerLinux";
const char* password = "Isabelle2014samira";
const char* mqtt_server = "192.168.150.1";


void setup() {

 //OTA Setup für Firmware
  ArduinoOTA.setHostname("warmwasser");
  ArduinoOTA.setPassword("84H53N9wTKPk5aCR");
  ArduinoOTA.begin();

// pcf8591 start
 pcf8591.begin();

//*************************************************************** Temp Sensor auslesen
sensors.begin();
sensors.setResolution(temp_keller, 9);
sensors.setResolution(temp_kollektor, 9);
sensors.setResolution(temp_aussen_osten, 9);
sensors.setResolution(temp_ww_puffer_oben, 9);
//sensors.setResolution(temp_vl_ph, 9);
sensors.setResolution(temp_rl_ph, 9);
sensors.setResolution(temp_wasser_hahn, 9);
sensors.setResolution(temp_vl_ww_puffer, 9); 

// MQTT Broker
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

// Serielle Kommunikation starten
  Serial.begin(115200);

  // Verbindung zum WiFI aufbauen

  Serial.print("Verbindung zu SSID -> ");
  Serial.println(ssid);

  // WiFi 
  IPAddress ip(192, 168, 4, 25);
	IPAddress dns(192, 168, 1, 1);  
	IPAddress subnet(255, 255, 0, 0);
	IPAddress gateway(192, 168, 1, 1);

	WiFi.config(ip, dns, gateway, subnet);
  
  WiFi.begin(ssid, password);

  // Wifi AP deaktivieren
  WiFi.mode(WIFI_STA);

  while (WiFi.status() != WL_CONNECTED) {
    delay(200);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("Erfolgreich mit dem WiFi verbunden!");
  Serial.print("IP Warmwasser : ");
  Serial.println(WiFi.localIP());
  Serial.print("ID : ");
  Serial.println(kartenID);


// *************************************************************** Messrelais
  pinMode(D8,OUTPUT);

// pcf8574 Expander konfigurieren
// *************************************************************** Konfig Relais
  pcf8574.pinMode(P0, OUTPUT);
  pcf8574.pinMode(P1, OUTPUT);
  pcf8574.pinMode(P2, OUTPUT);
  pcf8574.pinMode(P3, OUTPUT);     
  pcf8574.pinMode(P4, OUTPUT);
  pcf8574.pinMode(P5, OUTPUT);
  pcf8574.pinMode(P6, OUTPUT);
  pcf8574.pinMode(P7, OUTPUT);
  pcf8574.begin();


// *************************************************************** Relais OFF
pcf8574.digitalWrite(P0, !LOW);
pcf8574.digitalWrite(P1, !LOW);
pcf8574.digitalWrite(P2, !LOW);
pcf8574.digitalWrite(P3, !LOW);
pcf8574.digitalWrite(P4, !LOW);
pcf8574.digitalWrite(P5, !LOW);
pcf8574.digitalWrite(P6, !LOW);
pcf8574.digitalWrite(P7, !LOW);




}

void temperartur_schreiben() {

sensors.requestTemperatures();

// //*************************************************************** Transistor schalten
   //digitalWrite(D8,HIGH);
   //Serial.println("----- AKTOR ------ Transistor AN");
  // delay(150);

//*************************************************************** wert_temp_kollektor
  int currentTemp2 = sensors.getTempC(temp_kollektor);
  dtostrf(currentTemp2, 4, 2, stgFromFloat);

   if ((currentTemp2 == -127)||(currentTemp2 == 85))  { 
     } 
    else 
        {     
        
        sprintf(msgToPublish, "%s", stgFromFloat);
        sprintf(textTOtopic, "%s", topic_kollektor);
        client.publish(textTOtopic, msgToPublish);
        }

//*************************************************************** temp_aussen_osten
  int currentTemp3 = sensors.getTempC(temp_aussen_osten);
  dtostrf(currentTemp3, 4, 2, stgFromFloat);

   if ((currentTemp3 == -127)||(currentTemp3 == 85))  { 
     } 
    else 
        {     
        
        sprintf(msgToPublish, "%s", stgFromFloat);
        sprintf(textTOtopic, "%s", topic_aussen_ost);
        client.publish(textTOtopic, msgToPublish);
        }

//*************************************************************** temp_aussen_osten
  int currentTemp4 = sensors.getTempC(temp_ww_puffer_oben);
  dtostrf(currentTemp4, 4, 2, stgFromFloat);

   if ((currentTemp4 == -127)||(currentTemp4 == 85))  { 
     } 
    else 
        {     
        
        sprintf(msgToPublish, "%s", stgFromFloat);
        sprintf(textTOtopic, "%s", topic_puffer_oben);
        client.publish(textTOtopic, msgToPublish);
        }        

/*
DeviceAddress temp_ww_puffer_oben = { 0x28, 0xFF, 0x3B, 0x91, 0xA1, 0x16, 0x05, 0xF6 };
const char* topic_puffer_oben     = "Temperatur/WW_Puffer_oben";

DeviceAddress temp_vl_ph          = { 0x28, 0xAA, 0x2A, 0x9E, 0x16, 0x13, 0x02, 0xA2 };
const char* topic_vl_ph           = "Temperatur/vl_ph";

DeviceAddress temp_rl_ph          = { 0x28, 0x8F, 0x51, 0x64, 0x1B, 0x13, 0x01, 0x7A };
const char* topic_rl_ph           = "Temperatur/vl_ph";

DeviceAddress temp_wasser_hahn    = { 0x28, 0x9F, 0xEB, 0x70, 0x1B, 0x13, 0x01, 0xE5 };
const char* topic_wasser_hahn     = "Temperatur/Wasserhahn";

DeviceAddress temp_vl_ww_puffer   = { 0x28, 0x8F, 0x55, 0x88, 0x1B, 0x13, 0x01, 0x3E };
const char* topic_vl_ww_puffer    = "Temperatur/vl_ww_puffer";


*/
  

}

void wasserdruck_messen() {

  // ADC Pin für Wasserdruck ansprechen 
  int ana0V = pcf8591.adc_raw_read(0);
  dtostrf(ana0V, 4, 2, stgFromFloat);
 
        
    sprintf(msgToPublish, "%s", stgFromFloat);
    client.publish("Wasserdruck/LeitungBad", msgToPublish);

}

void lichtsensor_ost() {

  // ADC Pin für Wasserdruck ansprechen 
  int ana1V = pcf8591.adc_raw_read(1);
  dtostrf(ana1V, 4, 2, stgFromFloat);
 
        
    sprintf(msgToPublish, "%s", stgFromFloat);
    client.publish("Lichtsensor/Ost", msgToPublish);

}

void loop() {

  // OTA Handle starten
  ArduinoOTA.handle();  

// *************************************************************** pcf8591
  /*
   int ana0V = pcf8591.adc_raw_read(0);
   int ana1V = pcf8591.adc_raw_read(1);
   int ana2V = pcf8591.adc_raw_read(2);
   int ana3V = pcf8591.adc_raw_read(3);

   Serial.print("A0 BRIGHT ");  printLongRight(6,ana0V);
   Serial.print(" A1 TEMP ");   printLongRight(6,ana1V);
   Serial.print(" A2 ANA  ");   printLongRight(6,ana2V);
   Serial.print(" A3 POT ");    printLongRight(6,ana3V);
   Serial.println();
   delay(50);
   */
// ***************************************************************   

  if (!client.connected()) {
    reconnect();
  }
  client.loop();

//*************************************************************** Tempertur Messen
  if (millis() - letzteMillis_Temperatur > interval_Temperatur) {

    letzteMillis_Temperatur = millis();   // aktuelle Zeit abspeichern
    
    temperartur_schreiben();
    
  }  


//*************************************************************** Tempertur Messen
  if (millis() - letzteMillis_lichtstaerke > interval_Wasserdruck) {

    letzteMillis_lichtstaerke = millis();   // aktuelle Zeit abspeichern
    
    wasserdruck_messen();
    
  }   


//*************************************************************** Lichtstärke
  if (millis() - letzteMillis_Wasserdruck > interval_lichtstaerke) {

    letzteMillis_Wasserdruck = millis();   // aktuelle Zeit abspeichern
    
    lichtsensor_ost() ;
    
  }   




delay(200);



}

//**************************************************************** VOID mqtt callback
void callback(char* topic, byte* payload, unsigned int length) {
// *************************************************************** Relais A - Kollerktor Pumpen
// handle message arrived
  String content="";
  char character;

  Serial.print("mqtt : ");
  Serial.println(content);

    for (int num=0;num<length;num++) {
        character = payload[num];
        content.concat(character);
      }

    // *********************************************************** Relais 0
    if (strcmp(topic,"Warmwasser/ww_relais_0")==0) { // IF strcmp

        if (content == "on") {
            Serial.println("Relais 0 -> AN");
            pcf8574.digitalWrite(P0, !HIGH);
        }
      
        if (content == "off") {
            Serial.println("Relais 0 -> AUS");
            pcf8574.digitalWrite(P0, !LOW);
        }

    } // IF strcmp        

    // *********************************************************** Relais 1
    if (strcmp(topic,"Warmwasser/ww_relais_1")==0) { // IF strcmp

        if (content == "on") {
            Serial.println("Relais 1 -> AN");
            pcf8574.digitalWrite(P1, !HIGH);
        }
      
        if (content == "off") {
            Serial.println("Relais 1 -> AUS");
            pcf8574.digitalWrite(P1, !LOW);
        }        

    } // IF strcmp

    // *********************************************************** Relais 2
    if (strcmp(topic,"Warmwasser/ww_relais_2")==0) { // IF strcmp

        if (content == "on") {
            Serial.println("Relais 2 -> AN");
            pcf8574.digitalWrite(P2, !HIGH);
        }
      
        if (content == "off") {
            Serial.println("Relais 2 -> AUS");
            pcf8574.digitalWrite(P2, !LOW);
        }        

    } // IF strcmp

    // *********************************************************** Relais 3
    if (strcmp(topic,"Warmwasser/ww_relais_3")==0) { // IF strcmp

        if (content == "on") {
            Serial.println("Relais 3 -> AN");
            pcf8574.digitalWrite(P3, !HIGH);
        }
      
        if (content == "off") {
            Serial.println("Relais 3 -> AUS");
            pcf8574.digitalWrite(P3, !LOW);
        }        

    } // IF strcmp

    // *********************************************************** Relais 4
    if (strcmp(topic,"Warmwasser/ww_relais_4")==0) { // IF strcmp

        if (content == "on") {
            Serial.println("Relais 4 -> AN");
            pcf8574.digitalWrite(P4, !HIGH);
        }
      
        if (content == "off") {
            Serial.println("Relais 4 -> AUS");
            pcf8574.digitalWrite(P4, !LOW);
        }        

    } // IF strcmp

    // *********************************************************** Relais 5
    if (strcmp(topic,"Warmwasser/ww_relais_5")==0) { // IF strcmp

        if (content == "on") {
            Serial.println("Relais 5 -> AN");
            pcf8574.digitalWrite(P5, !HIGH);
        }
      
        if (content == "off") {
            Serial.println("Relais 5 -> AUS");
            pcf8574.digitalWrite(P5, !LOW);
        }        

    } // IF strcmp

    // *********************************************************** Relais 6
    if (strcmp(topic,"Warmwasser/ww_relais_6")==0) { // IF strcmp

        if (content == "on") {
            Serial.println("Relais 6 -> AN");
            pcf8574.digitalWrite(P6, !HIGH);
        }
      
        if (content == "off") {
            Serial.println("Relais 6 -> AUS");
            pcf8574.digitalWrite(P6, !LOW);
        }        

    } // IF strcmp

    // *********************************************************** Relais 7
    if (strcmp(topic,"Warmwasser/ww_relais_7")==0) { // IF strcmp

        if (content == "on") {
            Serial.println("Relais 7 -> AN");
            pcf8574.digitalWrite(P7, !HIGH);
        }
      
        if (content == "off") {
            Serial.println("Relais 7 -> AUS");
            pcf8574.digitalWrite(P7, !LOW);
        }        

    } // IF strcmp



}


//************************************************************** VOID mqtt reconnected
void reconnect() {
  // Loop until we're reconnected
    while (!client.connected()) {
      Serial.print("Baue Verbindung zum mqtt Server auf. IP: ");
      // Attempt to connect
        if (client.connect(kartenID,"zugang1","43b4134735")) {
          Serial.println("connected");
    // *************************************************************** SUBSCRIBE Eintraege
          client.subscribe ("Warmwasser/ww_relais_0");
          client.subscribe ("Warmwasser/ww_relais_1");
          client.subscribe ("Warmwasser/ww_relais_2");
          client.subscribe ("Warmwasser/ww_relais_3");
          client.subscribe ("Warmwasser/ww_relais_4");
          client.subscribe ("Warmwasser/ww_relais_5");
          client.subscribe ("Warmwasser/ww_relais_6");
          client.subscribe ("Warmwasser/ww_relais_7");

        } else {
          Serial.print("failed, rc=");
          Serial.print(client.state());
          Serial.println(" try again in 5 seconds");
          // Wait 5 seconds before retrying
          delay(5000);
        }
    }
}

// ***************************************************************//
// Right align a value (type long) for column algined text output.
// For ADC can get large numbers and int too small.
void printLongRight(byte fieldSize, long v) {
char str[10];
   ltoa(v,str,10);
   int len = strlen(str);
   if (len>fieldSize) len = fieldSize;
   int spc = fieldSize-len;
   for( int i=0;i<spc;i++) Serial.print(' ');
   for( int i=0;i<len;i++) {
      Serial.print(str[i]);
   }
}