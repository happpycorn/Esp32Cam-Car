// Import

#include <math.h>
#include <WiFi.h>
#include "fb_gfx.h"
#include "Arduino.h"
#include "soc/soc.h"
#include "esp_timer.h"
#include <WebServer.h>
#include "esp_camera.h"
#include "img_converters.h"
#include "soc/rtc_cntl_reg.h"

#define MAX(a, b) ((a) > (b) ? (a) : (b))

// Define : cam

#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27

#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

void initcam() ;
void handleStream() ;

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

const char* ssid = "AIOT" ;
const char* pwd = "00000000" ;
WebServer server(80);

void setup() 
{
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0) ; // 禁用電壓檢測(據說可以降低功耗)
  Serial.begin(115200) ; // 傳訊息用
  wifiConnect() ; // wifi 連接
  initcam() ;

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

  WiFi.mode(WIFI_STA) ; // 設定 STA 模式
  WiFi.begin(ssid, pwd) ; // 連線 WiFi
  
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500) ;
    Serial.println(".") ;
  }

  Serial.print("\n內網 IP 位址：") ;
  Serial.println(WiFi.localIP()) ;

}

// Cam

void initcam()
{
  // CAM SET

  camera_config_t config ;
  config.ledc_channel = LEDC_CHANNEL_0 ;
  config.ledc_timer = LEDC_TIMER_0 ;
  config.pin_d0 = Y2_GPIO_NUM ;
  config.pin_d1 = Y3_GPIO_NUM ;
  config.pin_d2 = Y4_GPIO_NUM ;
  config.pin_d3 = Y5_GPIO_NUM ;
  config.pin_d4 = Y6_GPIO_NUM ;
  config.pin_d5 = Y7_GPIO_NUM ;
  config.pin_d6 = Y8_GPIO_NUM ;
  config.pin_d7 = Y9_GPIO_NUM ;
  config.pin_xclk = XCLK_GPIO_NUM ;
  config.pin_pclk = PCLK_GPIO_NUM ;
  config.pin_vsync = VSYNC_GPIO_NUM ;
  config.pin_href = HREF_GPIO_NUM ;
  config.pin_sscb_sda = SIOD_GPIO_NUM ;
  config.pin_sscb_scl = SIOC_GPIO_NUM ;
  config.pin_pwdn = PWDN_GPIO_NUM ;
  config.pin_reset = RESET_GPIO_NUM ;
  config.xclk_freq_hz = 20000000 ;
  config.pixel_format = PIXFORMAT_JPEG ;

  config.frame_size = FRAMESIZE_VGA ;
  config.jpeg_quality = 63 ; // 影像品質
  config.fb_count = 2 ;

  // Cam Init

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err) ;
    return ;
  }

}

void getPhoto() {
  camera_fb_t * fb = esp_camera_fb_get();
  if (fb) {
    server.sendHeader("Content-Type", "image/jpeg");
    server.sendHeader("Content-Disposition", "inline", true);
    server.sendHeader("Cache-Control", "no-cache");
    server.sendContent(fb->buf, fb->len);
    esp_camera_fb_return(fb);
  }
}