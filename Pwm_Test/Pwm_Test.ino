const int motorPin = 14; // 直流电机连接到 GPIO14
const int freq = 5000;   // PWM 频率为 5kHz
const int ledChannel = 0;
const int resolution = 8; // 8 位分辨率

void setup() {
  // 配置 PWM 频道
  ledcSetup(ledChannel, freq, resolution);
  // 将通道绑定到 GPIO 引脚
  ledcAttachPin(motorPin, ledChannel);
}

void loop() {
  // 逐渐增加直流电机转速
  for (int dutyCycle = 0; dutyCycle <= 255; dutyCycle++) {
    ledcWrite(ledChannel, dutyCycle);   
    delay(10); 
  }
  
  // 逐渐降低直流电机转速
  for (int dutyCycle = 255; dutyCycle >= 0; dutyCycle--) {
    ledcWrite(ledChannel, dutyCycle);
    delay(10);
  }
}
