#include <SPI.h>
#include <Ethernet.h>
#include "DHT.h"
#include <SD.h>
#include <avr/wdt.h>

// Ethernet config
//---------------------------------------------------------------------------------
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xAD };
EthernetServer server(80);

// variable
//---------------------------------------------------------------------------------
char c = 0;                   // citanie  retazcov poziadavky
String readString;            // ukladanie retazcov poziadavky
#define DHTTYPE DHT11         // senzor teploty a vlhkosti
#define DHTPIN 6 
DHT dht(DHTPIN, DHTTYPE);
const uint8_t relay = 8;          // I/O rele
const uint8_t buffer[4] = {3,5,6,7}; // piny monitoringu stavu kvapaliny

float min_temp = 100;
float max_temp = 0;
bool relay_status = 0;

File webFile;               // the web page file on the SD card
//---------------------------------------------------------------------------------

void setup() {
    Serial.begin(9600);

    // disable Ethernet chip
    pinMode(10, OUTPUT);
    digitalWrite(10, HIGH);
    
    // initialize SD card
    Serial.println(F("Initializing SD card..."));
    if (!SD.begin(4)) {
        Serial.println(F("ERROR - SD card initialization failed!"));
        return;    // init failed
    }
   
    if (Ethernet.begin(mac) == 0) {
       Serial.println(F("Failed to configure Ethernet using DHCP"));
       for(;;)
         ;
     }
     Serial.println(Ethernet.localIP());
    // Configuration digital I/O
    pinMode(relay,OUTPUT);
    digitalWrite(relay,LOW);
    for (uint8_t i=0;i<5;i++)
      pinMode(buffer[i],INPUT);
  
    // Starting ,HTTP server, DHT sensor
    dht.begin();
    server.begin();
  Serial.println(Ethernet.localIP());
    wdt_enable (WDTO_8S);  // reset after one second, if no "pat the dog" received

}


void loop() {
    wdt_reset();

  // listen for incoming clients
  EthernetClient client = server.available();
  if (client) {

    boolean currentLineIsBlank = true;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        
        if (readString.length() < 100) {
          readString += c;
          Serial.print(c);
        }
        
        if (c == '\n' && currentLineIsBlank) {
          
          client.println(F("HTTP/1.1 200 OK"));
          if(readString.indexOf("json_input") >0)
          {
            client.println(F("Content-Type: application/json"));
            client.println(F("Connection: close"));
            client.println();
            json_response(client);
          }
          else {
            client.println(F("Content-Type: text/html"));
            client.println(F("Connection: close"));
            client.println();
          
            webFile = SD.open("index.htm");        // open web page file
            if (webFile) {
              while(webFile.available()) {
              client.write(webFile.read()); // send web page to client
              }
            webFile.close();
            }
          }
       
          if(readString.indexOf(F("relayon")) >0)//checks for on
          {
            relay_status = true;
            digitalWrite(relay, HIGH);
          }
          else
          {
            if(readString.indexOf(F("relayoff")) >0)//checks for off
            {
              relay_status = false;
              digitalWrite(relay, LOW);
            }
          }
          
       
          readString="";
          break;
        }
        if (c == '\n') {
          currentLineIsBlank = true;
        } 
        else if (c != '\r') {
          currentLineIsBlank = false;
        }
      }
    }
    
      float tempD = dht.readTemperature();
  
  if (tempD > max_temp)
    max_temp = tempD;
  else if ( tempD < min_temp)
    min_temp = tempD;
    
    delay(5);
    client.stop();
  }
}


void json_response(EthernetClient client)
{
    wdt_reset();

  String buffer_value[4] = {F(">80%"),F(">50%"),F(">25%"),F(">10%")};

  float tempD = dht.readTemperature();
  
  if (tempD > max_temp)
    max_temp = tempD;
  else if ( tempD < min_temp)
    min_temp = tempD;

  float vlhkostP = analogRead(A2) * (5.0 / 1023.0);
  float vlhkostV = dht.readHumidity(); 
  
  //teplota
  client.print(F("{\"temp\":\""));
  client.print(tempD);
  client.print(F("\",\"max_temp\":\""));
  client.print(max_temp);
  client.print(F("\",\"min_temp\":\""));
  client.print(min_temp);
          
  //vlhkost
  client.print(F("\",\"humidity_p\":\""));        
  client.print(vlhkostP);
  
  client.print(F("\",\"humidity_v\":\""));  
  client.print(vlhkostV);

  // stav zasobnika s vodou
  client.print(F("\",\"zasobnik\":\""));
    for (int i=0;i<5;i++) 
    {
      if (digitalRead(buffer[i]) == true)
      {
        client.print(buffer_value[i]);
        break;
      }
      if (i == 5)
        client.print(F("Critical!"));  
    }

          
  // stav cerpadla
  client.print(F("\",\"relatko\":\""));
  client.print((relay_status ? F("on") : F("off")));          
  client.print(F("\"}"));
}
