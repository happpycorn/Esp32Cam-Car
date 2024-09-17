// Import

#include <WiFi.h>
#include "fb_gfx.h"
#include "Arduino.h"
#include "soc/soc.h"
#include "esp_timer.h"
#include <WebServer.h>
#include "esp_camera.h"
#include "img_converters.h"
#include "soc/rtc_cntl_reg.h"

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

#define LED_PIN 4

void switch_led() ;

// Wifi setting

void wifi_connect() ;

const char* ssid = "AIOT" ;
const char* pwd = "00000000" ;
WebServer server(80) ;

const char webhtml[] PROGMEM = R"=====(
  <!DOCTYPE html>
  <html lang="en">
  <head>
      <meta charset="UTF-8">
      <meta name="viewport" content="width=device-width, initial-scale=1.0">
      <title>Web Test</title>
  </head>
  <body>
    <img src="/stream" />
    <button onclick="toggleLED()">LED Switch</button>

    <script>
        function toggleLED() {
            var xhttp = new XMLHttpRequest();
            xhttp.open("GET", "/toggleLED", true);
            xhttp.send();
        }
    </script>
  </body>
  </html>
)=====" ;

// Main

void setup()
{
  // SET

  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0) ; // 禁用電壓檢測(據說可以降低功耗)
  Serial.begin(115200) ; // 傳訊息用
  wifi_connect() ; // wifi 連接
  initcam() ;

  // PIN MODE SET

  pinMode(LED_PIN, OUTPUT) ;  

  server.on("/", []() {server.send(200, "text/html", webhtml);}) ;

  server.on("/stream", HTTP_GET, handleStream);

  server.on("/toggleLED", switch_led) ;
  
  server.begin() ; // 啟動伺服器

}

void loop(){server.handleClient();}

// car control

void switch_led()
{
  static bool ledState = LOW ;

  ledState = !ledState ;
  digitalWrite(LED_PIN, ledState) ;

  String ledStateStr = ledState ? "ON" : "OFF" ;
  server.send(200, "text/plain", "LED toggled to " + ledStateStr) ;
  Serial.println("LED toggled to " + ledStateStr) ;

}

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

void handleStream() {
  camera_fb_t *fb = NULL; // img place
  size_t _jpg_buf_len = 0; // img length
  uint8_t *_jpg_buf = NULL; // img local
  char part_buf[64]; // a part of img (use to send to web)

  // Send HTTP header
  server.sendContent_P("HTTP/1.1 200 OK\r\nContent-Type: multipart/x-mixed-replace; boundary=frame\r\n\r\n");

  while (true) {
    fb = esp_camera_fb_get(); // get img

    // preprocessing
    if (!fb) {
      Serial.println("Camera capture failed"); // img get failed
      break;
    } else {
      if (fb->format != PIXFORMAT_JPEG) { // type change
        bool converted = frame2jpg(fb, 80, &_jpg_buf, &_jpg_buf_len);
        esp_camera_fb_return(fb);
        fb = NULL;

        if (!converted) {
          Serial.println("JPEG compression failed");
          break;
        }
      } else {
        _jpg_buf_len = fb->len;
        _jpg_buf = fb->buf;
      }
    }

    // send img
    size_t hlen = snprintf(part_buf, 64, "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n", _jpg_buf_len);
    server.sendContent_P(part_buf, hlen); // 发送HTTP头信息
    server.sendContent_P((const char *)_jpg_buf, _jpg_buf_len); // 发送JPEG图像数据
    server.sendContent_P("\r\n--frame\r\n", 14); // 发送帧边界

    // release resource and error
    if (fb) {
      esp_camera_fb_return(fb); // 释放帧缓冲区
      fb = NULL;
      _jpg_buf = NULL;
    } else if (_jpg_buf) {
      free(_jpg_buf); // 释放JPEG缓冲区
      _jpg_buf = NULL;
    }
  }
}