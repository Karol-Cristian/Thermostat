#include <string>
#include <Arduino.h>
#include <U8g2lib.h>
#include "painlessMesh.h"
//#include <FS.h>
//extern "C" {
//}


Scheduler userScheduler; // to control your personal task
painlessMesh  mesh;
// User stub
void sendMessage() ; // Prototype so PlatformIO doesn't complain

Task taskSendMessage( TASK_SECOND * 1 , TASK_FOREVER, &sendMessage );


#include <Bounce2.h>

#define   MESH_PREFIX     "whateverYouLike"
#define   MESH_PASSWORD   "somethingSneaky"
#define   MESH_PORT       5555

#define U8X8_HAVE_HW_I2C
#include <Wire.h>

#define SDA D1
#define SCL D2



U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R2, /* reset=*/ U8X8_PIN_NONE, /* clock=*/ SCL, /* data=*/ SDA);

Bounce debouncerUp = Bounce();  
Bounce debouncerDown = Bounce();




int buttonPinUp = D6;
int buttonPinDown = D7;
int debaunceTime = 20;

bool relayState;

void sendMessage() {
  
  String msg = "1 ";
  msg += relayState;
  mesh.sendBroadcast( msg );
  taskSendMessage.setInterval( random( TASK_SECOND * 15, TASK_SECOND * 20 ));
}

const double VCC = 3.3;             // NodeMCU on board 3.3v vcc
const double R2 = 10000;            // 10k ohm series resistor
const double adc_resolution = 1023; // 10-bit adc

const double A = 0.001129148;   // thermistor equation parameters
const double B = 0.000234125;
const double C = 0.0000000876741; 
double temperature;
float setTemp = 20.0;
// Needed for painless library
void receivedCallback( uint32_t from, String &msg ) {
  Serial.printf("startHere: Received from %u msg=%s\n", from, msg.c_str());
  switch(from){
    case 3814948839:
     setTemp = msg.toFloat();
   break;
    case 3814948344:
    mesh.sendBroadcast( msg );
    break;
  }
  
  }

String Connect = "Connecting...";
void newConnectionCallback(uint32_t nodeId) {
    Serial.printf("--> startHere: New Connection, nodeId = %u\n", nodeId);
    if (nodeId = 3814948839){
      Connect = "Connected";
    }
}

void changedConnectionCallback() {
  Serial.printf("Changed connections\n");
}

void nodeTimeAdjustedCallback(int32_t offset) {
    Serial.printf("Adjusted time %u. Offset = %d\n", mesh.getNodeTime(),offset);
}

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

  temperature = temperature - 275.40;  // Temperature in degree celsius
  Serial.print("\n ");
  Serial.print(temperature);
 // Serial.println(" degree celsius");
       }


void setup(void) 
{
  Serial.begin(115200);


//mesh.setDebugMsgTypes( ERROR | MESH_STATUS | CONNECTION | SYNC | COMMUNICATION | GENERAL | MSG_TYPES | REMOTE ); // all types on
mesh.setDebugMsgTypes( ERROR | STARTUP );  // set before init() so that you can see startup messages

  mesh.init( MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT );
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);
  mesh.setContainsRoot(true);
  userScheduler.addTask( taskSendMessage );
  taskSendMessage.enable();
   
 
  u8g2.begin();  
  u8g2.enableUTF8Print();
  pinMode(buttonPinUp, INPUT_PULLUP);
  pinMode(buttonPinDown, INPUT_PULLUP);
  debouncerUp.attach(buttonPinUp);
  debouncerDown.attach(buttonPinDown);
  debouncerUp.interval(debaunceTime);
  debouncerDown.interval(debaunceTime);
  
  readTemp();
  updateDisplay();
}


unsigned long lastMillis, startMillis, buttonMillis, currentMillis, lastUpdateDisplay, lastUpdateDht;

bool lastButtonUp, lastButtonDown, lastButtonMode;
bool stateButtonUp, stateButtonDown, stateButtonMode;
bool displayOn=0; 


void updateDisplay()
{
  u8g2.firstPage();
  do {
    u8g2.setFont(u8g2_font_t0_11_tf);
   // u8g2.drawStr(2,8,"Humidity");
 
    u8g2.setCursor(41, 8);
    u8g2.print(Connect);
   // u8g2.drawStr(114,8,"%");


    u8g2.setCursor(80, 63);
   // u8g2.print("1014hPa");  
    
    u8g2.setCursor(2, 20);
    u8g2.print("T");
    u8g2.setFont(u8g2_font_u8glib_4_tf);
    u8g2.print("SET ");
    
    u8g2.setFont(u8g2_font_logisoso16_tf);
    u8g2.setCursor(2, 44);
    u8g2.print(setTemp, 1);
    

  /*  u8g2.setFont(u8g2_font_t0_11_tf);
    u8g2.setCursor(44, 8);
    u8g2.print(lastMillis);
    u8g2.print(" ms");    // requires enableUTF8Print()*/
            
    u8g2.drawHLine(2, 10, 130);
    u8g2.drawHLine(2, 52, 130);
  //  u8g2.drawVLine(42, 11, 41);
  //  u8g2.drawVLine(2, 10, 130);
    u8g2.setFont(u8g2_font_logisoso24_tf);
    u8g2.setCursor(50, 44);
    u8g2.printf("%.1f",temperature);
    u8g2.print("°C");    // requires enableUTF8Print()
    
    u8g2.setFont(u8g2_font_t0_11_tf);
    u8g2.setCursor(2, 62);

//    u8g2.setFontMode(1);    // 0=solid, 1=transparent
    if(temperature>setTemp)
    {
      u8g2.drawBox(98,53,27,11);
      u8g2.setFontMode(1);
      u8g2.setDrawColor(2);
      u8g2.print("Heat  DORMITOR  Cool");
    } else {
      u8g2.drawBox(2,53,27,11);
      u8g2.setFontMode(1);
      u8g2.setDrawColor(2);
      u8g2.print("Heat  DORMITOR  Cool");    
    }
    
  } while ( u8g2.nextPage() );

}


void updateRelayState()
{
    if(temperature>setTemp)
    {
      relayState = 0;
    } else {    
      relayState = 1;  
    }
}


void updateSetpoint()
{
  u8g2.firstPage();
  do
  {
    u8g2.setFont(u8g2_font_logisoso50_tf);
    u8g2.setCursor(7, 57);
    u8g2.print(setTemp, 1); 
  } while ( u8g2.nextPage() );

}

/////////////////////////////////////  MY OWN ////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////////////////

void loop(void) 
{
   unsigned long currentMillis = millis(); // grab current time. 
  
 if (millis() - lastUpdateDht > 31000) // 31 seconds wait in this Loop before next temperature read cycle. 
 { 
    
  
    // Reading temperature or humidity takes about 250 milliseconds!
    // Wait a few seconds between measurements.
     readTemp();
     updateRelayState();
     lastUpdateDht = millis();
 }
    
  if (debouncerUp.update()) 
  {
    // Get the update value.
    stateButtonUp = debouncerUp.read();
    // Send in the new value.
    if(stateButtonUp == LOW)
    {
      buttonMillis=millis();
      if(displayOn)
      {
        setTemp += 0.5;
        updateSetpoint();
      }
    }
  }
  if (debouncerDown.update()) 
  {
    // Get the update value.
    stateButtonDown = debouncerDown.read();
    // Send in the new value.
    if(stateButtonDown == LOW)
    {
      buttonMillis=millis();
      if(displayOn)
      {
        setTemp -= 0.5;
        updateSetpoint();
      }
    }
  }

  
  startMillis = millis();

  lastMillis = millis() - startMillis;

  if(millis() - buttonMillis > 300000)
  {
    if(displayOn)
    {
      u8g2.setPowerSave(1);
      displayOn = 0;
    }
  }
  else
  {
    if(!displayOn)
    {
      u8g2.setPowerSave(0);
      displayOn = 1;
    }
  }
  if(millis() - buttonMillis > 1000 && millis() - lastUpdateDisplay > 1000 && displayOn)
  {
    updateDisplay();
    lastUpdateDisplay = millis();
  }

mesh.update();
}
