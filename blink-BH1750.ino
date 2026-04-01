/* 请将下面三行替换为你在 Blynk 网页端生成的真实凭据 */
#define BLYNK_TEMPLATE_ID "TMPL62s3TdgIQ"
#define BLYNK_TEMPLATE_NAME "Plant Care"
#define BLYNK_AUTH_TOKEN "LANqq7TWTmChKYRzicHSIumEibfdm11e"



// 开启 Blynk 调试信息打印到串口
#define BLYNK_PRINT Serial

#include <ESP8266_Lib.h>
#include <BlynkSimpleShieldEsp8266.h>
#include <Wire.h>
#include <BH1750.h>

// 替换为你所在环境的 Wi-Fi 名称和密码
char ssid[] = "G08GR";
char pass[] = "33338888";

// 根据手册，Mega 使用 Serial1 连接 ESP8266，波特率为 115200
#define EspSerial Serial1
#define ESP8266_BAUD 115200

// 初始化 ESP8266 模块和 BH1750 传感器
ESP8266 wifi(&EspSerial);
BH1750 lightMeter;

// 初始化 Blynk 定时器
BlynkTimer timer;

// 这个函数会读取传感器数据并发送给 Blynk
void sendSensorData() {
  float lux = lightMeter.readLightLevel();
  Serial.print("当前光照强度: ");
  Serial.print(lux);
  Serial.println(" lx");
  
  // 将 lux 数据发送到 Blynk 的虚拟引脚 V0
  Blynk.virtualWrite(V0, lux);
}

void setup() {
  // 启动连接电脑的串口，用于监控调试
  Serial.begin(115200);
  delay(10);
  Serial.println("--- 串口通讯已启动 ---");
  // 启动连接 ESP8266 的串口 (Serial1)
  EspSerial.begin(ESP8266_BAUD);
  delay(10);

  // 启动 I2C 总线和 BH1750
  Wire.begin();
  if (lightMeter.begin()) {
    Serial.println(F("BH1750 初始化成功"));
  } else {
    Serial.println(F("无法初始化 BH1750，请检查接线！"));
  }

  // 连接到 Wi-Fi 和 Blynk 服务器
  Blynk.begin(BLYNK_AUTH_TOKEN, wifi, ssid, pass);

  // 设置定时器，每 2000 毫秒（2秒）调用一次 sendSensorData 函数
  // 注意：不要把传感器读取和上传放在 loop() 里，那样会因为发送太快被服务器断开
  timer.setInterval(2000L, sendSensorData);
}

void loop() {
  // 保持 Blynk 运行
  Blynk.run();
  // 保持定时器运行
  timer.run();
}