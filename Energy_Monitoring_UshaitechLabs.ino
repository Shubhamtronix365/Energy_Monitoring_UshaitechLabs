/************************************************************
 SMART HOME AUTOMATION WITH SAFETY & ENERGY MONITORING
 ESP32 + Blynk + PZEM + LCD + Gas + Flame
************************************************************/

#define BLYNK_TEMPLATE_ID "TMPL3GUEzm7_V"
#define BLYNK_TEMPLATE_NAME "Relay 1"
#define BLYNK_AUTH_TOKEN "vOm_EeOl8BjbMYxq0YohAwBrqXnx_1Xu"

#define BLYNK_PRINT Serial

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <PZEM004Tv30.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ----------- WIFI -----------
char ssid[] = "SSID";
char pass[] = "PASSWORD";

// ----------- RELAYS (ACTIVE LOW) ----------
#define RELAY1 13
#define RELAY2 14
#define RELAY3 27
#define RELAY4 26

// ----------- SWITCHES (INPUT_PULLUP) --------
#define SW1 25
#define SW2 32
#define SW3 33
#define SW4 4

// ----------- SENSORS ---------
#define GAS_PIN   34
#define FLAME_PIN 35

// ----------- OBJECTS ---------
LiquidCrystal_I2C lcd(0x27, 16, 2);
HardwareSerial pzemSerial(2);
PZEM004Tv30 pzem(pzemSerial, 16, 17);
BlynkTimer timer;

// ----------- STATES ----------
bool relayState[4] = {0,0,0,0};
bool lastSwitchState[4] = {HIGH,HIGH,HIGH,HIGH};

// ----------- SENSOR DETECTION (REVERSED LOGIC) ----------
#define GAS_DETECTED   (digitalRead(GAS_PIN) == LOW)
#define FLAME_DETECTED (digitalRead(FLAME_PIN) == LOW)

// ----------- BLYNK BUTTON CONTROL -----------
BLYNK_WRITE(V0){ relayState[0]=param.asInt(); digitalWrite(RELAY1, relayState[0]?LOW:HIGH); }
BLYNK_WRITE(V1){ relayState[1]=param.asInt(); digitalWrite(RELAY2, relayState[1]?LOW:HIGH); }
BLYNK_WRITE(V2){ relayState[2]=param.asInt(); digitalWrite(RELAY3, relayState[2]?LOW:HIGH); }
BLYNK_WRITE(V3){ relayState[3]=param.asInt(); digitalWrite(RELAY4, relayState[3]?LOW:HIGH); }

// ----------- EMERGENCY SHUTDOWN -----------
void emergencyShutdown()
{
  for(int i=0;i<4;i++) relayState[i]=0;

  digitalWrite(RELAY1,HIGH);
  digitalWrite(RELAY2,HIGH);
  digitalWrite(RELAY3,HIGH);
  digitalWrite(RELAY4,HIGH);

  Blynk.virtualWrite(V0,0);
  Blynk.virtualWrite(V1,0);
  Blynk.virtualWrite(V2,0);
  Blynk.virtualWrite(V3,0);

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("!!! ALERT !!!");
  lcd.setCursor(0,1);
  lcd.print("Gas/Flame!");

  Blynk.logEvent("fire_alert","Gas or Flame Detected! Power Cut Off!");
}

// ----------- SAFETY CHECK -----------
void checkSafety()
{
  if(GAS_DETECTED || FLAME_DETECTED)
  {
    emergencyShutdown();
  }
}

// ----------- SWITCH CHECK -----------
void checkSwitches()
{
  int swPins[4]={SW1,SW2,SW3,SW4};
  int relPins[4]={RELAY1,RELAY2,RELAY3,RELAY4};

  for(int i=0;i<4;i++)
  {
    bool currentState=digitalRead(swPins[i]);

    if(currentState!=lastSwitchState[i])
    {
      delay(50);  // debounce

      if(currentState==LOW)
      {
        relayState[i]=!relayState[i];
        digitalWrite(relPins[i], relayState[i]?LOW:HIGH);
        Blynk.virtualWrite(V0+i, relayState[i]);
      }

      lastSwitchState[i]=currentState;
    }
  }
}

// ----------- ENERGY DISPLAY -----------
void readEnergy()
{
  float voltage = pzem.voltage();
  float current = pzem.current();
  float power   = pzem.power();
  float freq    = pzem.frequency();

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("V:");
  lcd.print(voltage);
  lcd.print(" I:");
  lcd.print(current);

  lcd.setCursor(0,1);
  lcd.print("P:");
  lcd.print(power);
  lcd.print(" F:");
  lcd.print(freq);

  Blynk.virtualWrite(V4, voltage);
  Blynk.virtualWrite(V5, current);
  Blynk.virtualWrite(V6, power);
  Blynk.virtualWrite(V7, freq);
}

// ----------- SETUP -----------
void setup()
{
  Serial.begin(115200);

  Wire.begin(21,22);
  lcd.init();
  lcd.backlight();

  pinMode(RELAY1,OUTPUT);
  pinMode(RELAY2,OUTPUT);
  pinMode(RELAY3,OUTPUT);
  pinMode(RELAY4,OUTPUT);

  // Turn OFF all relays (Active LOW)
  digitalWrite(RELAY1,HIGH);
  digitalWrite(RELAY2,HIGH);
  digitalWrite(RELAY3,HIGH);
  digitalWrite(RELAY4,HIGH);

  pinMode(SW1,INPUT_PULLUP);
  pinMode(SW2,INPUT_PULLUP);
  pinMode(SW3,INPUT_PULLUP);
  pinMode(SW4,INPUT_PULLUP);

  pinMode(GAS_PIN,INPUT);
  pinMode(FLAME_PIN,INPUT);

  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  timer.setInterval(2000L, readEnergy);

  lcd.setCursor(0,0);
  lcd.print("Smart System");
  delay(2000);
  lcd.clear();
}

// ----------- LOOP -----------
void loop()
{
  Blynk.run();
  timer.run();
  checkSwitches();
  checkSafety();
}