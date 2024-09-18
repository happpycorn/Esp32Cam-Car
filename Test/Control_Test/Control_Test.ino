// Import

#include <math.h>
#include <WiFi.h>
#include <stdio.h>
#include "config.h"
#include <WebServer.h>

#define MAX(a, b) ((a) > (b) ? (a) : (b))

// Define : car

const int countPin = 4 ;
const int motorPin[countPin] = {12, 13, 14, 15} ;
const int ledPin = 4 ;

const int freq = 5000 ;
const int resolution = 8 ;

const int servoPin = 2 ;
const int servoFreq = 50 ;
const int servoResolution = 16 ;

void ledControl() ;
void motorControl() ;
void servoControl() ;

// Wifi setting

void wifiConnect() ;

const char* ssid_STA = "AIOT" ;
const char* pwd_STA = "00000000" ;
const char* ssid_AP = "Esp32Cam_car" ;
const char* pwd_AP = "12345678" ;

WebServer server(80);

void setup() 
{

  Serial.begin(115200) ; // 傳訊息用
  wifiConnect() ; // wifi 連接

  ledcSetup(ledPin, freq, resolution) ;
  ledcAttachPin(ledPin, ledPin) ;

  for (int i = 0; i < countPin; i++)
  {
    ledcSetup(motorPin[i], freq, resolution) ;
    ledcAttachPin(motorPin[i], motorPin[i]) ;
  }

  ledcSetup(servoPin, servoFreq, servoResolution) ;
  ledcAttachPin(servoPin, servoPin) ;

  server.on("/", []() {server.send(200, "text/html", indexHtml);}) ;

  server.on("/led", ledControl) ;
  server.on("/motor", motorControl) ;
  server.on("/servo", servoControl) ;
  
  server.begin() ;

  Serial.println("OK") ;

  ledcWrite(ledPin, 255) ;
  delay(1000) ;
  ledcWrite(ledPin, 0) ;
}

void loop(){server.handleClient();}

// car control

void ledControl() // 255 ~ 0
{
  int duty = server.arg("duty").toInt() ;
  ledcWrite(ledPin, duty) ;
}

void motorControl() // 255, 255 ~ -255, -255
{
  float motorL, motorR ;
  sscanf(server.arg("motor").c_str(), "%f,%f", &motorL, &motorR) ;

  motorL = map(motorL, -80, 80, -255, 255) ;
  motorR = map(motorR, -80, 80, -255, 255) ;

  ledcWrite(motorPin[0], MAX(motorL, 0)) ;
  ledcWrite(motorPin[1], MAX(-motorL, 0)) ;
  ledcWrite(motorPin[2], MAX(motorR, 0)) ;
  ledcWrite(motorPin[3], MAX(-motorR, 0)) ;

  server.send(200, "OK");
}

void servoControl() // 100 ~ 0
{
  String dutyStr = server.arg("duty") ;
  int duty = map(dutyStr.toInt(), 0, 50, 819, 4915) ;
  ledcWrite(servoPin, duty) ;
}

// Wifi

void wifiConnect()
{

  WiFi.mode(WIFI_AP_STA) ; // 設定 STA 模式
  WiFi.begin(ssid_STA, pwd_STA) ; // 連線 WiFi
  
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500) ;
    Serial.println(".") ;
  }

  Serial.print("\n內網 IP 位址：") ;
  Serial.println(WiFi.localIP()) ;

  WiFi.softAP(ssid_AP, pwd_AP) ;

}
