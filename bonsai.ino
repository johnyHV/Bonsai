#include <SPI.h>
#include <Ethernet.h>
#include <DHT.h>
#include <SD.h>
#include <avr/wdt.h>

// Ethernet config
//---------------------------------------------------------------------------------
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xAD };
//IPAddress ip(158,193,86,96);
EthernetServer server(80);
//IPAddress brana( 158,193,86,126);
//IPAddress maska(255,255,255,192);
//---------------------------------------------------------------------------------
// http://startingelectronics.com/tutorials/arduino/ethernet-shield-web-server-tutorial/SD-card-gauge/

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
    // Open serial communications and wait for port to open:
    Serial.begin(9600);

    // disable Ethernet chip
    pinMode(10, OUTPUT);
    digitalWrite(10, HIGH);
    
    // initialize SD card
    Serial.println("Initializing SD card...");
    if (!SD.begin(4)) {
        Serial.println("ERROR - SD card initialization failed!");
        return;    // init failed
    }
    //Serial.println("SUCCESS - SD card initialized.");
    // check for index.htm file
//    if (!SD.exists("index.htm")) {
        //Serial.println("ERROR - Can't find index.htm file!");
//        return;  // can't find index file
//   }
    //Serial.println("SUCCESS - Found index.htm file.");
    
    // DHCP getting IP address
    //Serial.println("Getting IP adres wia DHCP");
    //Ethernet.begin(mac, ip, brana, maska);
    if (Ethernet.begin(mac) == 0) {
       Serial.println("Failed to configure Ethernet using DHCP");
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
//  Serial.println(Ethernet.localIP());
    wdt_enable (WDTO_8S);  // reset after one second, if no "pat the dog" received

}


void loop() {
    wdt_reset();

  // listen for incoming clients
  EthernetClient client = server.available();
  if (client) {
    //Serial.println("new client");
    // an http request ends with a blank line
    boolean currentLineIsBlank = true;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        
        //read char by char HTTP request
        if (readString.length() < 100) {
          //store characters to string
          readString += c;
          Serial.print(c);
        }
        
        //Serial.write(c);
        // if you've gotten to the end of the line (received a newline
        // character) and the line is blank, the http request has ended,
        // so you can send a reply
        if (c == '\n' && currentLineIsBlank) {
          //Serial.println(readString); //print to serial monitor for debuging
          
          // send a standard http response header
          client.println("HTTP/1.1 200 OK");
          if(readString.indexOf("json_input") >0)
          {
            client.println("Content-Type: application/json");
            client.println("Connection: close");  // the connection will be closed after completion of the response
	    //client.println("Refresh: 15");  // refresh the page automatically every 10 sec
            client.println();
            json_response(client);
          }
          else {
            client.println("Content-Type: text/html");
            client.println("Connection: close");  // the connection will be closed after completion of the response
	    //client.println("Refresh: 15");  // refresh the page automatically every 10 sec
            client.println();
          
            webFile = SD.open("index.htm");        // open web page file
            if (webFile) {
              while(webFile.available()) {
              client.write(webFile.read()); // send web page to client
              }
            webFile.close();
            }
          }
       

          // nacitanie stavu tlacidiel
          if(readString.indexOf("relayon") >0)//checks for on
          {
            relay_status = true;
            digitalWrite(relay, HIGH);
            //Serial.println("Led On");
          }
          else
          {
            if(readString.indexOf("relayoff") >0)//checks for off
            {
              relay_status = false;
              digitalWrite(relay, LOW);
              //Serial.println("Led Off");
            }
          }
          
       
          readString="";
          break;
        }
        if (c == '\n') {
          // you're starting a new line
          currentLineIsBlank = true;
        } 
        else if (c != '\r') {
          // you've gotten a character on the current line
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
    //Serial.println("client disconnected");
  }
}


// send the XML file containing analog value
void json_response(EthernetClient client)
{
    wdt_reset();

  String buffer_value[4] = {">80%",">50%",">25%",">10%"};

  //float tempC = (((((analogRead(A0) / 1023.0) * 5) * 100.0) - 273.15) ); 
  float tempD = dht.readTemperature();
  
  if (tempD > max_temp)
    max_temp = tempD;
  else if ( tempD < min_temp)
    min_temp = tempD;

  //DHT11
  float vlhkostP = analogRead(A2) * (5.0 / 1023.0);
  float vlhkostV = dht.readHumidity(); 
  
  //teplota
  client.print("{\"temp\":\"");
  client.print(tempD);
  client.print("\",\"max_temp\":\"");
  client.print(max_temp);
  client.print("\",\"min_temp\":\"");
  client.print(min_temp);
          
  //vlhkost
  client.print("\",\"humidity_p\":\"");        
  //client.print(100 - ((vlhkostP) * 5/1023));
  client.print(vlhkostP);
  
  client.print("\",\"humidity_v\":\"");  
  client.print(vlhkostV);


  // stav zasobnika s vodou
  client.print("\",\"zasobnik\":\"");
    for (int i=0;i<5;i++) 
    {
      if (digitalRead(buffer[i]) == true)
      {
        client.print(buffer_value[i]);
        break;
      }
      if (i == 5)
        client.print("Critical!");  
    }

          
  // stav cerpadla
  client.print("\",\"relatko\":\"");
  client.print((relay_status ? "on" : "off"));          
  client.print("\"}");
}
