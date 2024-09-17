// Import

#include <WiFi.h>
#include "fb_gfx.h"
#include "Arduino.h"
#include "esp_log.h"
#include "soc/soc.h"
#include "esp_wifi.h"
#include "esp_timer.h"
#include "esp_camera.h"
#include "img_converters.h"
#include "esp_http_server.h"
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
esp_err_t stream_handler(httpd_req_t *req) ;

static const char *TAG = "camera_httpd" ;

// Define : car

#define MRTOR_FL 12
#define MRTOR_BL 13
#define MRTOR_FR 15
#define MRTOR_BR 14
#define LED_PIN 4

esp_err_t toggle_led_handler(httpd_req_t *req) ;

// Wifi setting

void wifi_connect();

const char* ssid = "AIOT" ;
const char* pwd = "00000000" ;

esp_err_t index_handler(httpd_req_t *req) ;

const char index_html[] PROGMEM = R"=====(
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
  pinMode(MRTOR_FL, OUTPUT) ;
  pinMode(MRTOR_BL, OUTPUT) ;
  pinMode(MRTOR_FR, OUTPUT) ;
  pinMode(MRTOR_BR, OUTPUT) ;
  
  // START

  startCameraServer();

}

void loop(){delay(1000);}

// car control

bool ledState = false ;

esp_err_t toggle_led_handler(httpd_req_t *req) // Led
{
    ledState = !ledState;
    digitalWrite(LED_PIN, ledState ? HIGH : LOW);

    const char* resp_str = (ledState) ? "LED is ON" : "LED is OFF";
    httpd_resp_send(req, resp_str, strlen(resp_str));

    Serial.println(resp_str);
    return ESP_OK;
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

  Serial.print("\n IP : ") ;
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

  config.frame_size = FRAMESIZE_UXGA ;
  config.jpeg_quality = 10 ;
  config.fb_count = 2 ;

  // Cam Init

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err) ;
    return ;
  }

}

// Web

esp_err_t index_handler(httpd_req_t *req) // index
{
    httpd_resp_send(req, index_html, strlen(index_html)) ;
    return ESP_OK ;
}

esp_err_t stream_handler(httpd_req_t *req) // stream
{
    camera_fb_t *fb = NULL;
    esp_err_t res = ESP_OK;
    size_t _jpg_buf_len = 0;
    uint8_t *_jpg_buf = NULL;
    char *part_buf[64];

    static const char* _STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=frame";
    static const char* _STREAM_BOUNDARY = "\r\n--frame\r\n";
    static const char* _STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

    res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
    if (res != ESP_OK) {
        return res;
    }

    while (true) {
        fb = esp_camera_fb_get();
        if (!fb) {
            Serial.println("Camera capture failed");
            res = ESP_FAIL;
        } else {
            if (fb->format != PIXFORMAT_JPEG) {
                bool converted = frame2jpg(fb, 80, &_jpg_buf, &_jpg_buf_len);
                if (!converted) {
                    Serial.println("JPEG compression failed");
                    esp_camera_fb_return(fb);
                    res = ESP_FAIL;
                }
            } else {
                _jpg_buf_len = fb->len;
                _jpg_buf = fb->buf;
            }
        }

        if (res == ESP_OK) {
            size_t hlen = snprintf((char *)part_buf, 64, _STREAM_PART, _jpg_buf_len);
            res = httpd_resp_send_chunk(req, (const char *)part_buf, hlen);
        }
        if (res == ESP_OK) {
            res = httpd_resp_send_chunk(req, (const char *)_jpg_buf, _jpg_buf_len);
        }
        if (res == ESP_OK) {
            res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
        }

        if (fb) {
            esp_camera_fb_return(fb);
            fb = NULL;
            _jpg_buf = NULL;
        } else if (_jpg_buf) {
            free(_jpg_buf);
            _jpg_buf = NULL;
        }

        if (res != ESP_OK) {
            break;
        }
    }
    return res;
}

void startCameraServer() {
    httpd_handle_t server = NULL ;

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    httpd_uri_t index_uri = {
        .uri       = "/",
        .method    = HTTP_GET,
        .handler   = index_handler,
        .user_ctx  = NULL
    };

    httpd_uri_t stream_uri = {
        .uri       = "/stream",
        .method    = HTTP_GET,
        .handler   = stream_handler,
        .user_ctx  = NULL
    };

    httpd_uri_t toggle_led_uri = {
        .uri       = "/toggleLED",
        .method    = HTTP_GET,
        .handler   = toggle_led_handler,
        .user_ctx  = NULL
    };

    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_register_uri_handler(server, &index_uri);
        httpd_register_uri_handler(server, &stream_uri);
        httpd_register_uri_handler(server, &toggle_led_uri);
    }
}
