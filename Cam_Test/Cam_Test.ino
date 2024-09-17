#include "esp_camera.h"
#include <WiFi.h>
#include "esp_timer.h"
#include "img_converters.h"
#include "Arduino.h"
#include "fb_gfx.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include <WebServer.h>

// 摄像头引脚定义

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

// Wi-Fi 配置

void wifi_connect();

const char* ssid = "AIOT" ;
const char* pwd = "00000000" ;
WebServer server(80) ;

const char *index_html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>ESP32-CAM Stream</title>
</head>
<body>
  <h1>ESP32-CAM Stream</h1>
  <img src="/stream" />
</body>
</html>
)rawliteral";

void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); // 禁用低电压检测

  Serial.begin(115200);
  wifi_connect() ; // wifi 連接
  initcam() ;

  server.on("/", []() {server.send(200, "text/html", index_html);}) ;

  server.on("/stream", HTTP_GET, handleStream);
  
  server.begin() ; // 啟動伺服器
}

void loop(){server.handleClient();}

// Wifi

void wifi_connect()
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

  config.frame_size = FRAMESIZE_CIF ;
  config.jpeg_quality = 10 ;
  config.fb_count = 2 ;

  // Cam Init

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err) ;
    return ;
  }

}

void handleStream() {
  camera_fb_t * fb = esp_camera_fb_get();
  if (fb) {
    server.sendHeader("Content-Type", "image/jpeg");
    server.sendHeader("Content-Disposition", "inline", true);
    server.sendHeader("Cache-Control", "no-cache");
    server.sendContent(fb->buf, fb->len);
    esp_camera_fb_return(fb);
  }
}