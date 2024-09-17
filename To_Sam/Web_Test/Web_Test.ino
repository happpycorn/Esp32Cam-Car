#include <WiFi.h>
#include <WebServer.h>

#define LED_PIN 4

const char* ssid = "AIOT";
const char* pwd = "00000000";
WebServer server(80);

const char* html_basic = R"=====(
  <!DOCTYPE html>
  <html lang="en">
  <head>
      <meta charset="UTF-8">
      <meta name="viewport" content="width=device-width, initial-scale=1.0">
      <title>Web Test</title>
  </head>
  <body>
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
)=====";

void wifi_set(){
  Serial.begin(115200);           // 設定 Baud
  WiFi.mode(WIFI_STA);            // 設定 STA 模式
  WiFi.begin(ssid, pwd);          // 連線 WiFi
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println(".");
  }

  Serial.print("\n內網 IP 位址：");
  Serial.println(WiFi.localIP());
}

void switch_led() ;

void setup() {

  pinMode(LED_PIN, OUTPUT);
  
  wifi_set() ;

  server.on("/", []() {
    server.send(200, "text/html", html_basic);
  });

  server.on("/toggleLED", switch_led);
  
  server.begin(); // 啟動伺服器
}

void loop() {
  server.handleClient(); 
}

void switch_led(){
  static bool ledState = false;
  ledState = !ledState;
  digitalWrite(LED_PIN, ledState);
  server.send(200, "text/plain", "LED toggled");
  Serial.println("LED toggled to " + String(ledState ? "ON" : "OFF"));
}