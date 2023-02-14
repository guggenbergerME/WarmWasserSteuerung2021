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
const char* kartenID = "WW-Steuerung-Keller2";


// *************************************************************** Temperaturfuehler einrichten
#define ONE_WIRE_BUS D5
#define TEMPERATURE_PRECISION 10
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
DeviceAddress tempDeviceAddress; // Verzeichniss zum Speichern von Sensor Adressen
int numberOfDevices; // Anzahl der gefundenen Sensoren
// Temp. Sensor HEX zuweisen
//DeviceAddress temp_keller         = { 0x28, 0xAA, 0x6C, 0x90, 0x16, 0x13, 0x02, 0xC2 }; 
DeviceAddress temp_keller         = { 0x28, 0xF3, 0xF5, 0x7C, 0x1B, 0x13, 0x01, 0x97 };
DeviceAddress temp_kollektor      = { 0x28, 0xFF, 0x7F, 0x77, 0xA1, 0x16, 0x05, 0x40 }; 
DeviceAddress temp_aussen_osten   = { 0x28, 0xFF, 0x26, 0x95, 0xA1, 0x16, 0x04, 0x36 };
DeviceAddress temp_ww_puffer_oben = { 0x28, 0xFF, 0x3B, 0x91, 0xA1, 0x16, 0x05, 0xF6 };
DeviceAddress temp_vl_ph          = { 0x28, 0xAA, 0x2A, 0x9E, 0x16, 0x13, 0x02, 0xA2 };
DeviceAddress temp_rl_ph          = { 0x28, 0x8F, 0x51, 0x64, 0x1B, 0x13, 0x01, 0x7A };
DeviceAddress temp_wasser_hahn    = { 0x28, 0x9F, 0xEB, 0x70, 0x1B, 0x13, 0x01, 0xE5 };
DeviceAddress temp_vl_ww_puffer   = { 0x28, 0x8F, 0x55, 0x88, 0x1B, 0x13, 0x01, 0x3E };

// *************************************************************** Temp. Sensor Messwert Variablen
char stgFromFloat[10];
char msgToPublish[60];
char textTOtopic[60];

float wert_temp_keller;
const char* topic_keller = "Temperatur/Raum_WW_Keller";

float wert_temp_kollektor;
float wert_temp_aussen_osten;
float wert_temp_ww_puffer_oben;
float wert_temp_vl_ph;
float wert_temp_rl_ph;
float wert_temp_wasser_hahn;
float wert_temp_vl_ww_puffer;

// ***************************************************************///////////// Intervall der Steuerung
unsigned long previousMillis_Temperatur = 0;
unsigned long interval_Temperatur = 30000; 

unsigned long previousMillis_Wasserdruck= 0;
unsigned long interval_Wasserdruck= 5000; 

unsigned long previousMillis_lichtstaerke= 0;
unsigned long interval_lichtstaerke = 60000; 
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

// MQTT Broker
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

// Serielle Kommunikation starten
  Serial.begin(115200);

  // Verbindung zum WiFI aufbauen

  Serial.print("Verbindung zu SSID -> ");
  Serial.println(ssid);

  // WiFi 
  IPAddress ip(192, 168, 3, 10);
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
  pinMode(D3,OUTPUT);

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


// *************************************************************** Relais Check
/*
pcf8574.digitalWrite(P0, !HIGH);
delay(400);
pcf8574.digitalWrite(P0, !LOW);
delay(400);

pcf8574.digitalWrite(P1, !HIGH);
delay(400);
pcf8574.digitalWrite(P1, !LOW);
delay(400);

pcf8574.digitalWrite(P2, !HIGH);
delay(400);
pcf8574.digitalWrite(P2, !LOW);
delay(400);

pcf8574.digitalWrite(P3, !HIGH);
delay(400);
pcf8574.digitalWrite(P3, !LOW);
delay(400);

pcf8574.digitalWrite(P4, !HIGH);
delay(400);
pcf8574.digitalWrite(P4, !LOW);
delay(400);

pcf8574.digitalWrite(P5, !HIGH);
delay(400);
pcf8574.digitalWrite(P5, !LOW);
delay(400);

pcf8574.digitalWrite(P6, !HIGH);
delay(400);
pcf8574.digitalWrite(P6, !LOW);
delay(400);

pcf8574.digitalWrite(P7, !HIGH);
delay(400);
pcf8574.digitalWrite(P7, !LOW);
delay(400);
*/

//*************************************************************** Temp Sensor auslesen
sensors.begin();
sensors.setResolution(temp_keller, 4);
sensors.setResolution(temp_kollektor, 4);
sensors.setResolution(temp_aussen_osten, 4);
sensors.setResolution(temp_ww_puffer_oben, 4);
sensors.setResolution(temp_vl_ph, 4);
sensors.setResolution(temp_rl_ph, 4);
sensors.setResolution(temp_wasser_hahn, 4);
sensors.setResolution(temp_vl_ww_puffer, 4);


}

void temperartur_schreiben() {
// //*************************************************************** Transistor schalten
   digitalWrite(D3,HIGH);
   //Serial.println("Transistor HIGH");
   delay(25);

//*************************************************************** Sensor schreiben
sensors.requestTemperatures();

//*************************************************************** temp_keller
  int currentTemp = sensors.getTempC(temp_keller);

   if ((currentTemp == -127)||(currentTemp == 85))  { 
     } 
    else 
        {     
        dtostrf(currentTemp, 4, 2, stgFromFloat);
        sprintf(msgToPublish, "%s", stgFromFloat);
        sprintf(textTOtopic, "%s", topic_keller);
        client.publish(textTOtopic, msgToPublish);
        }

// //*************************************************************** Transistor schalten
   digitalWrite(D3,LOW);
   //Serial.println("Transistor LOW");
   //delay(5000);

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

//temperartur_schreiben();

//delay(2500);



}

//**************************************************************** VOID mqtt callback
void callback(char* topic, byte* payload, unsigned int length) {
// *************************************************************** Relais A - Kollerktor Pumpen
 /*
    if (strcmp(topic,"Pumpe_Kollektor/IN")==0) {

        // Kanal A
        if ((char)payload[0] == 'o' && (char)payload[1] == 'n') {  
                 Serial.println("relais_A -> AN");
                 pcf8574.digitalWrite(P0, !HIGH);
                 client.publish("Pumpe_Kollektor/OUT","on");
                delay(100);
              }

        if ((char)payload[0] == 'o' && (char)payload[1] == 'f' && (char)payload[2] == 'f') {  
                 Serial.println("relais_A -> AUS");
                 pcf8574.digitalWrite(P0, !LOW);
                 client.publish("Pumpe_Kollektor/OUT","off");
                delay(100);
              }
      } 
*/



// handle message arrived
  String content="";
  char character;
    for (int num=0;num<length;num++) {
        character = payload[num];
        content.concat(character);
      }

      Serial.print("***** MQTT callback - Content : ");
      Serial.print(topic);Serial.print(" | ");Serial.println(content);

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