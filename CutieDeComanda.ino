//************************************************************

//************************************************************

#include "IPAddress.h"
#include "painlessMesh.h"

#ifdef ESP8266
#include "Hash.h"
#include <ESPAsyncTCP.h>
#else
#include <AsyncTCP.h>
#endif
#include <ESPAsyncWebServer.h>

const int button_up = D2;
int container = 0;

const int CHrelay = D5;
const int relay1 = D6;
const int relay2 = D7;
const int relay3 = D4;
const int red = D1;
const int green = D3;
const int blue = D8;

bool a = false, b = false, c = false;


const double VCC = 3.3;             // NodeMCU on board 3.3v vcc
const double R2 = 10000;            // 10k ohm series resistor
const double adc_resolution = 1023; // 10-bit adc

const double A = 0.001129148;   // thermistor equation parameters
const double B = 0.000234125;
const double C = 0.0000000876741; 
double temperature;

#define   MESH_PREFIX     "whateverYouLike"
#define   MESH_PASSWORD   "somethingSneaky"
#define   MESH_PORT       5555

#define   STATION_SSID     "TP-Link_56C8"
#define   STATION_PASSWORD "62337540"

#define HOSTNAME "HTTP_BRIDGE"

// Prototype
void receivedCallback( const uint32_t &from, const String &msg );
IPAddress getlocalIP();

painlessMesh  mesh;
AsyncWebServer server(80);
IPAddress myIP(0,0,0,0);
IPAddress myAPIP(0,0,0,0);



void readTemp()
{
 double Vout, Rth, adc_value; 

  adc_value = analogRead(A0);
  Vout = (adc_value * VCC) / adc_resolution;
  Rth = (VCC * R2 / Vout) - R2;

/*  Steinhart-Hart Thermistor Equation:
 *  Temperature in Kelvin = 1 / (A + B[ln(R)] + C[ln(R)]^3)
 *  where A = 0.001129148, B = 0.000234125 and C = 8.76741*10^-8  */
  temperature = (1 / (A + (B * log(Rth)) + (C * pow((log(Rth)),3))));   // Temperature in kelvin

  temperature = temperature - 273.15;  // Temperature in degree celsius
  Serial.print("\nTemperature: ");
  Serial.print(temperature);
  Serial.println(" degree celsius");
       }

void setup() {
  Serial.begin(115200);

  mesh.setDebugMsgTypes( ERROR | STARTUP | CONNECTION );  // set before init() so that you can see startup messages

  // Channel set to 6. Make sure to use the same channel for your mesh and for you other
  // network (STATION_SSID)
  mesh.init( MESH_PREFIX, MESH_PASSWORD, MESH_PORT, WIFI_AP_STA, 6 );
  mesh.onReceive(&receivedCallback);

  mesh.stationManual(STATION_SSID, STATION_PASSWORD);
  mesh.setHostname(HOSTNAME);

  mesh.setRoot(true);
  // This node and all other nodes should ideally know the mesh contains a root, so call this on all nodes
  mesh.setContainsRoot(true);

  myAPIP = IPAddress(mesh.getAPIP());
  Serial.println("My AP IP is " + myAPIP.toString());

  //Async webserver
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/html", "<center><form>Dormitor<br><input type='text' name='BROADCAST'><br><br><input type='submit' value='Submit'></form></center><center><form>Dormitor 2<br><input type='text' name='BROADCAST'><br><br><input type='submit' value='Submit'></center><center></form><form>Dormitor 2<br><input type='text' name='BROADCAST'><br><br><input type='submit' value='Submit'></form></center>");
    
    if (request->hasArg("BROADCAST")){
      String msg = request->arg("BROADCAST");
      mesh.sendBroadcast(msg);
     
    }
  });
  server.begin();

  pinMode(button_up, INPUT_PULLUP);
  pinMode(CHrelay, OUTPUT); 
  pinMode(relay1, OUTPUT); 
  pinMode(relay2, OUTPUT);
  pinMode(relay3, OUTPUT);
  pinMode(red, OUTPUT); 
  pinMode(green, OUTPUT);
  pinMode(blue, OUTPUT);
  digitalWrite(CHrelay, HIGH);
  digitalWrite(relay1, HIGH);
  digitalWrite(relay2, HIGH);
  digitalWrite(relay3, HIGH);
  digitalWrite(green,LOW);
  digitalWrite(red, LOW);
  digitalWrite(blue, HIGH);

  readTemp();

}

unsigned long lastMillis, startMillis, buttonMillis, currentMillis, lastUpdateDisplay, lastUpdateDht, critical, half_an_hour, tto;
volatile bool flag = false;

void preventiveStop()
{
  digitalWrite(relay1, HIGH);
  digitalWrite(relay2, HIGH);
  digitalWrite(relay3, HIGH);
  flag = false;
  a=b=c=false;
}
//************************************************************************************************************************************************


void loop() {
    
unsigned long currentMillis = millis(); // grab current time. 

 while(digitalRead(button_up) == 0)
  {

    if(container == 0)
    {
      container = 1;
      Serial.printf("container = %i", container);
    }
    else
    {
      container = 0;
      Serial.printf("container = %i", container);
    }
  }
 
 
 switch (container){

  case 0:
   mesh.update();
  if(myIP != getlocalIP()){
    myIP = getlocalIP();
    Serial.println("My IP is " + myIP.toString());
  }

 if (flag == true) //boiler delayed start 
 {
  if (millis() - critical  > 60000)
  {
    digitalWrite(CHrelay, LOW);
    
  }
  if (millis() - half_an_hour > 1.8e+6) ////boiler and everything stops every 30 min in case there is a lost connection and boiler remained on
{
  preventiveStop();
  half_an_hour = millis();
}
 }else {
    digitalWrite(CHrelay, HIGH);
    critical = millis();
  }
  break;
 case 1: 
  if (millis() - lastUpdateDht > 300000) // 300 seconds wait in this Loop before next temperature read cycle. 
 { 
    
     readTemp();
     lastUpdateDht = millis();
 }
 /* Here i should implement modem sleep */
   digitalWrite(green,LOW);
   digitalWrite(red, HIGH);
   digitalWrite(blue, LOW);
   digitalWrite(CHrelay, LOW);
   digitalWrite (relay1, LOW);
   digitalWrite (relay2, LOW);
   digitalWrite (relay3, LOW);
   break;
 }
}

void receivedCallback( const uint32_t &from, const String &msg ) {
  Serial.printf("bridge: Received from %u msg=%s\n", from, msg.c_str());
 String thOn1 = "1 1";
 String thOff1 = "1 0";
 String thOn2 = "2 1";
 String thOff2 = "2 0";
  String thOn3 = "3 1";
 String thOff3 = "3 0";
 
 if (msg == thOn1){
  digitalWrite (relay1, LOW);
  a = true;
 }else if (msg == thOff1){
  digitalWrite (relay1, HIGH);
  a = false;
 }

 if (msg == thOn2) {
  digitalWrite (relay2, LOW);
  digitalWrite (relay3, LOW);
  b = true;
 }else if(msg == thOff2){
  digitalWrite (relay2, HIGH);
  digitalWrite (relay3, HIGH);
  b = false;
 }

 if (msg == thOn3){
  digitalWrite (relay3, LOW);
  c = true;
 }else if (msg == thOff3){
  digitalWrite (relay3, HIGH);
  c = false;
  }
  
  if (a || b || c){
    flag = true;
    Serial.printf("\nFlag = %s\n", flag ? "TRUE" : "FALSE");
  }else{
    flag = false;
    Serial.printf("\nFlag = %s\n", flag ? "TRUE" : "FALSE");
  } 
}


IPAddress getlocalIP() {
  return IPAddress(mesh.getStationIP()); 
}
