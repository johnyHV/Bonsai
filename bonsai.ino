#include <SPI.h>
#include <Ethernet.h>
#include <DHT.h>
#include <EthernetUdp.h>
#include <SD.h>

// Ethernet config
//---------------------------------------------------------------------------------
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress ip(158,193,86,96);
EthernetServer server(80);
IPAddress brana( 158,193,86,126);
IPAddress maska(255,255,255,192);
//---------------------------------------------------------------------------------
// http://startingelectronics.com/tutorials/arduino/ethernet-shield-web-server-tutorial/SD-card-gauge/

// NTP config
//---------------------------------------------------------------------------------
IPAddress timeServer(132, 163, 4, 101); // time-a.timefreq.bldrdoc.gov NTP server
// IPAddress timeServer(132, 163, 4, 102); // time-b.timefreq.bldrdoc.gov NTP server
//IPAddress timeServer(194, 160, 23, 2); // time-c.timefreq.bldrdoc.gov NTP server
const int NTP_PACKET_SIZE= 48; // NTP time stamp is in the first 48 bytes of the message
byte packetBuffer[ NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets 
unsigned int localPort = 8888;
EthernetUDP Udp;
//---------------------------------------------------------------------------------

// variable
//---------------------------------------------------------------------------------
struct cas {
  uint8_t hour;
  uint8_t min;
  uint8_t sec;
};
cas time;

char c = 0;                   // citanie  retazcov poziadavky
String readString;            // ukladanie retazcov poziadavky
#define DHTTYPE DHT22         // senzor teploty a vlhkosti
#define DHTPIN 6 
DHT dht(DHTPIN, DHTTYPE);
const uint8_t relay = 13;          // I/O rele
const uint8_t buffer[4] = {2,3,6,5}; // piny monitoringu stavu kvapaliny

String buffer_value[4] = {" >80%"," >50%"," >25%"," >10% - Critical !"};
bool relay_status = 0;

#define REQ_BUF_SZ   50
File webFile;               // the web page file on the SD card
char HTTP_req[REQ_BUF_SZ] = {0}; // buffered HTTP request stored as null terminated string
char req_index = 0;              // index into HTTP_req buffer
//---------------------------------------------------------------------------------

// function
//---------------------------------------------------------------------------------
void get_time(EthernetUDP *Udp,cas *time);
//---------------------------------------------------------------------------------



void setup() {
 // Open serial communications and wait for port to open:
  Serial.begin(9600);
  
  // DHCP getting IP address
  Serial.println("Getting IP adres wia DHCP");
  //Ethernet.begin(mac, ip, brana, maska);
  if (Ethernet.begin(mac) == 0) {
     Serial.println("Failed to configure Ethernet using DHCP");
     for(;;)
       ;
   }
  
  // Configuration digital I/O
  pinMode(relay,OUTPUT);
  digitalWrite(relay,LOW);
  for (uint8_t i=0;i<5;i++)
    pinMode(buffer[i],INPUT);
  
  // Starting NTP,WHTTP server, DHT sensor
  dht.begin();
  Udp.begin(localPort);
  server.begin();
  Serial.print("server is at ");
  Serial.println(Ethernet.localIP());
  
  /*
  // initialize SD card
    Serial.println("Initializing SD card...");
    if (!SD.begin(4)) {
        Serial.println("ERROR - SD card initialization failed!");
        return;    // init failed
    }
    Serial.println("SUCCESS - SD card initialized.");
    // check for index.htm file
    if (!SD.exists("index.htm")) {
        Serial.println("ERROR - Can't find index.htm file!");
        return;  // can't find index file
    }
    Serial.println("SUCCESS - Found index.htm file.");
*/
}


void loop() {
  //teplota
  float tempC = (((((analogRead(A0) / 1023.0) * 5) * 100.0) - 273.15) ); 
  float tempD = dht.readTemperature();

  //DHT11
  float vlhkostP = analogRead(A1) * (5.0 / 1023.0);
  float vlhkostV = dht.readHumidity();  
  
  get_time(&Udp,&time);
  // listen for incoming clients
  EthernetClient client = server.available();
  if (client) {
    Serial.println("new client");
    // an http request ends with a blank line
    boolean currentLineIsBlank = true;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        
        //read char by char HTTP request
        if (readString.length() < 100) {
          //store characters to string
          readString += c;
          //Serial.print(c);
        }
        
        Serial.write(c);
        // if you've gotten to the end of the line (received a newline
        // character) and the line is blank, the http request has ended,
        // so you can send a reply
        if (c == '\n' && currentLineIsBlank) {
          Serial.println(readString); //print to serial monitor for debuging
          
          // send a standard http response header
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println("Connection: close");  // the connection will be closed after completion of the response
	  client.println("Refresh: 15");  // refresh the page automatically every 10 sec
          client.println();
          client.println("<!DOCTYPE HTML>");
          client.println("<html>");
          
          client.println("<HEAD>");
          client.println("<meta name='Bonsai - Carmona Microphylla' content='yes' />");
          client.println("<TITLE>Bonsai</TITLE>");
          
          client.println("    <style> #visualization{width: 500px; height: 500px; } </style>");         
          client.println("    <script type='text/javascript' src='http://www.google.com/jsapi'></script>");
          client.println("    <script type='text/javascript'>");
          client.println("      google.load('visualization', '1', {packages:['AnnotatedTimeLine']});");
          client.println("      google.setOnLoadCallback(drawChart);");

          client.println("      function drawChart() {");
          client.println("var data = new google.visualization.DataTable();");
          client.println("	data.addColumn('date', 'Date');");
          client.println("	data.addColumn('number', 'Sold Pencils');");
          client.println("	data.addColumn('string', 'title1');");
          client.println("	data.addColumn('string', 'text1');");
          client.println("	data.addColumn('number', 'Sold Pens');");
          client.println("	data.addColumn('string', 'title2');");
          client.println("	data.addColumn('string', 'text2');");
          client.println("	data.addRows([");
          client.println("		[new Date(2008, 1 ,1), 30000, null, null, 40645, null, null],");
          client.println("		[new Date(2008, 1 ,2), 14045, null, null, 20374, null, null],");
          client.println("		[new Date(2008, 1 ,3), 55022, null, null, 50766, null, null],");
          client.println("		[new Date(2008, 1 ,4), 75284, null, null, 14334, 'Out of Stock', 'Ran out of stock on pens at 4pm'],");
          client.println("		[new Date(2008, 1 ,5), 41476, 'Bought Pens', 'Bought 200k pens', 66467, null, null],");
          client.println("		[new Date(2008, 1 ,6), 33322, null, null, 39463, null, null]");
          client.println("	]);");
          client.println("	var annotatedtimeline = new google.visualization.AnnotatedTimeLine("); //AnnotatedTimeLine
          client.println("		document.getElementById('visualization'));");
          client.println("        var options = {width: 160, height: 160, 'displayAnnotations': true}");
          client.println("	annotatedtimeline.draw(data, options);");
          client.println("      }");
          client.println("    </script>");     
          client.println("</HEAD>");
          
          client.println("<body>");
          client.println("<center><font color=\"#43BFC7\">");
          client.println("<H1>Bonsai - Carmona Microphylla</H1>");
          client.println("</font><hr /><br />");


          // aktualny cas
          client.print("Aktualny cas: ");
          client.print(time.hour);    
          client.print(":");
          client.print(time.min);
          client.print(":");
          client.print(time.sec);
          client.print("<br /><br />");
          
          //teplota
          client.print("Teplota LM335: ");
          client.print(tempC);
          client.print(" *C");
          client.println("<br />");
          client.print("Teplota DHT11: ");
          client.print(tempD);
          client.print(" *C ");
          client.println("<br /><br />");
          
          //vlhkost
          client.print("Vlhkost pody: ");          
          client.print(100 - ((vlhkostP / 5) * 100));
          client.print(" %");
          client.println("<br />"); 
          client.print("Vlhkost vzduchu: "); 
          client.print(vlhkostV);
          client.print(" %"); 
          client.println("<br />");
/*
          // stav zasobnika s vodou
          client.println("<br />");
          client.print("Stav zasobnika s vodou: ");
          for (int i=0;i<5;i++) 
            if (digitalRead(buffer[i]) == true)
              client.print(buffer_value[i]);
          client.println("<br />");
    */      
          // stav cerpadla
          //client.print("Stav zavlazovania: ");
          //client.print((relay_status ? "Zapnuty" : "Vypnuty"));          
          //client.println("<br />");

          // tlacidla
          client.print((relay_status ? "<br /><br /><a href=\"/?relayoff\"\">Vypnut zavlazovanie</a>" : "<br /><br /><a href=\"/?relayon\"\">Zapnut zavlazovanie</a>"));               
          
          client.print("<br /><br />");
          client.print((relay_status ? "<a href='http://158.193.86.96?relayoff'><button style='background:lightgreen;width:10%;height:40px'>Vypnut zavlazovanie</button></a>" :
                                       "<a href='http://158.193.86.96?relayon'><button style='background:red;width:10%;height:40px'>Zapnut zavlazovanie</button></a>"));
          
          /*
          client.print("<FORM action='http://158.193.86.96' method='GET'>");
          client.print("<P> <INPUT type='radio' name='led1' value='1'>ON");
          client.print("<P> <INPUT type='radio' name='led1' value='0'>OFF");
          client.print("<P> <INPUT type='submit' value='Odeslat'> </FORM>"); 
          */
          

          client.println("    <div id='visualization'></div>");


           
           
          client.println("</center></body></HTML>");
       
                 client.println("Connection: close");  // the connection will be closed after completion of the response


       
          if(readString.indexOf("relayon") >0)//checks for on
          {
            relay_status = true;
            digitalWrite(relay, HIGH);
            Serial.println("Led On");
          }
          else
          {
            if(readString.indexOf("relayoff") >0)//checks for off
            {
              relay_status = false;
              digitalWrite(relay, LOW);
              Serial.println("Led Off");
            }
          }
          
          /*
          client.print(F("<body>"));
          client.print(F("<canvas id=\"myCanvas\" width=\"1200\" height=\"650\" style=\"border:1px solid #d3d3d3;\">"));
          client.print(F("Your browser does not support the HTML5 canvas tag.</canvas>"));
          client.print(F("<script>"));
          
          client.print(F("var c=document.getElementById(\"myCanvas\");"));
          client.print(F("var ctx=c.getContext(\"2d\");"));
          client.print(F("ctx.rect(80,50,1000,500);"));
          client.print(F("ctx.font=\"20px Arial\";"));
          client.print(F("ctx.fillText(\"Sensor\",5,50);"));
          client.print(F("ctx.fillText(\"Reading\",5,70);"));
          client.print(F("ctx.fillText(\"Time\",1030,580);"));
                    
          //client.print(F("ctx.fillStyle=\"#FF0000\";"));
          //client.print(F("ctx.fillRect(0,0,150,75);"));
          
          //client.print(F("ctx.moveTo(0,0);"));
          
          int pointX = 50;
          int pointY = 550;
          client.print(F("ctx.moveTo("));
          client.print(pointX);
          client.print(",");
          client.print(pointY);
          client.print(F(");"));
          
          for(double i = 0; i < 1000; i++)
          {
            pointX = i + 80;
            //pointY = 550 - (i/2);
            pointY = 550 - ((i/45)*(i/45));
            client.print(F("ctx.lineTo("));
            client.print(pointX);
            client.print(",");
            client.print(pointY);
            client.print(F(");"));
            client.print(F("ctx.stroke();"));            
          }
          
          client.print(F("</script>"));
          client.print(F("</body>"));
          client.print(F("</html>"));          
          */
          
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
    delay(1);
    client.stop();
    Serial.println("client disconnected");
  }
}



void get_time(EthernetUDP *Udp,cas *time)
{
   sendNTPpacket(timeServer); // send an NTP packet to a time server

     // wait to see if a reply is available
   delay(1000);  

   if ( Udp->parsePacket() ) {  
     // We've received a packet, read the data from it
     Udp->read(packetBuffer,NTP_PACKET_SIZE);  // read the packet into the buffer

     //the timestamp starts at byte 40 of the received packet and is four bytes,
     // or two words, long. First, esxtract the two words:

     unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
     unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);  
     // combine the four bytes (two words) into a long integer
     // this is NTP time (seconds since Jan 1 1900):
     unsigned long secsSince1900 = highWord << 16 | lowWord;  
     //Serial.print("Seconds since Jan 1 1900 = " );
     //Serial.println(secsSince1900);               

     // now convert NTP time into everyday time:
     Serial.print("Unix time = ");
     // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
     const unsigned long seventyYears = 2208988800UL;     
     // subtract seventy years:
     unsigned long epoch = secsSince1900 - seventyYears;  
     // print Unix time:
     //Serial.println(epoch);                               


     // print the hour, minute and second:
     Serial.print("The UTC time is ");       // UTC is the time at Greenwich Meridian (GMT)
     Serial.print((epoch  % 86400L) / 3600); // print the hour (86400 equals secs per day)
     time->hour = (epoch  % 86400L) / 3600 +2;
     Serial.print(':');  
     if ( ((epoch % 3600) / 60) < 10 ) {
       // In the first 10 minutes of each hour, we'll want a leading '0'
       Serial.print('0');
     }
     Serial.print((epoch  % 3600) / 60); // print the minute (3600 equals secs per minute)
     time->min = (epoch  % 3600) / 60;
     Serial.print(':'); 
     if ( (epoch % 60) < 10 ) {
       // In the first 10 seconds of each minute, we'll want a leading '0'
       Serial.print('0');
     }
     Serial.println(epoch %60); // print the second
     time->sec = epoch %60;
   }
   // wait ten seconds before asking for the time again
   //delay(10000);
}

unsigned long sendNTPpacket(IPAddress& address)
{
   // set all bytes in the buffer to 0
   memset(packetBuffer, 0, NTP_PACKET_SIZE); 
   // Initialize values needed to form NTP request
   // (see URL above for details on the packets)
   packetBuffer[0] = 0b11100011;   // LI, Version, Mode
   packetBuffer[1] = 0;     // Stratum, or type of clock
   packetBuffer[2] = 6;     // Polling Interval
   packetBuffer[3] = 0xEC;  // Peer Clock Precision
   // 8 bytes of zero for Root Delay & Root Dispersion
   packetBuffer[12]  = 49; 
   packetBuffer[13]  = 0x4E;
   packetBuffer[14]  = 49;
   packetBuffer[15]  = 52;

   // all NTP fields have been given values, now
   // you can send a packet requesting a timestamp:         
   Udp.beginPacket(address, 123); //NTP requests are to port 123
   Udp.write(packetBuffer,NTP_PACKET_SIZE);
   Udp.endPacket(); 
}

