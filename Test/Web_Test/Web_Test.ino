#include <WiFi.h>
#include <WebServer.h>

#define LED_PIN 15

const int freq = 5000;
const int ledChannel = 0;
const int resolution = 8;

const char* ssid = "AIOT";
const char* pwd = "00000000";
WebServer server(80);

const char* html_pwm = R"=====(
  <!DOCTYPE html>
  <html lang="en">
  <head>
      <meta charset="UTF-8">
      <meta name="viewport" content="width=device-width, initial-scale=1.0">
      <title>PWM LED Control</title>
  </head>
  <body>
      <input type="range" min="0" max="255" value="0" onchange="adjustPWM(this.value)">
      <script>
          function adjustPWM(value) {
              var xhttp = new XMLHttpRequest();
              xhttp.open("GET", "/setPWM?value=" + value, true);
              xhttp.send();
          }
      </script>
  </body>
  </html>
)=====";

void wifi_set(){
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pwd);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println(".");
  }

  Serial.print("\n內網 IP 位址：");
  Serial.println(WiFi.localIP());
}

void set_pwm() ;

void setup() {
  ledcSetup(ledChannel, freq, resolution);
  // 将通道绑定到 GPIO 引脚
  ledcAttachPin(LED_PIN, ledChannel);
  
  wifi_set() ;

  server.on("/", []() {
    server.send(200, "text/html", html_pwm);
  });

  server.on("/setPWM", set_pwm);
  
  server.begin();
}

void loop() {
  server.handleClient(); 
}

void set_pwm(){
  if(server.args() == 1 && server.hasArg("value")) {
    int pwmValue = server.arg("value").toInt();
    ledcWrite(ledChannel, pwmValue); 
    server.send(200, "text/plain", "PWM value set to " + String(pwmValue));
    Serial.println("PWM value set to " + String(pwmValue));
  } else {
    server.send(400, "text/plain", "Invalid request");
    Serial.println("Invalid request");
  }
}
