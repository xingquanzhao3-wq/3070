// 引入必要的库
#include <SoftwareSerial.h>
#include <ArduinoJson.h>
#include <WiFiEsp.h> // 你需要安装 ArduinoJson 库来方便地创建JSON

// ESP8266 设置
#define ESP_BAUDRATE 115200

// WiFi 设置
const char* ssid = "你的WiFi名字";       // 修改为实际的SSID
const char* password = "你的WiFi密码";   // 修改为实际的密码

// 服务器设置 (你同学的Flask服务器)
const char* serverHost = "192.168.1.100"; // 修改为你同学电脑的IP地址
const int serverPort = 5000;               // Flask默认端口是5000

// 创建用于和ESP8266通信的串口对象 (使用Mega的Serial1)
// Mega的Serial1 对应 RX1 (19), TX1 (18)
// 记得在Mega上把ESP8266的RX接18, TX接19

// WiFiEspClient 对象 (需要 WiFiEsp 库)
WiFiEspClient client;

// 传感器值变量 (示例)
float temperature = 0;
float humidity = 0;
int lightLevel = 0;

void setup() {
  // 初始化电脑串口，用于调试
  Serial.begin(9600);
  // 初始化与ESP8266通信的串口 (Serial1)
  Serial1.begin(ESP_BAUDRATE);
  // 初始化ESP模块
  WiFi.init(&Serial1);

  // 连接WiFi
  connectToWiFi();
}

void loop() {
  // 1. 读取传感器数据 (这里替换成你实际的传感器读取代码)
  readSensors(); // 你需要自己实现这个函数

  // 2. 将数据发送到服务器
  sendDataToServer(temperature, humidity, lightLevel);

  // 3. 等待一段时间再上传 (例如30秒)
  delay(30000);
}

// 连接WiFi的函数
void connectToWiFi() {
  Serial.print("正在连接 WiFi ...");
  while (WiFi.begin(ssid, password) != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }
  Serial.println(" 连接成功！");
}

// 发送数据到 Flask 服务器的函数
void sendDataToServer(float temp, float humi, int light) {
  Serial.println("准备发送数据到服务器...");

  if (client.connect(serverHost, serverPort)) {
    Serial.println("连接到服务器成功！");

    // --- 关键部分：构造HTTP POST请求 ---
    // 1. 创建一个JSON文档来存放数据
    StaticJsonDocument<200> doc;
    doc["temperature"] = temp;
    doc["humidity"] = humi;
    doc["light"] = light;

    // 2. 将JSON文档序列化成一个字符串
    String jsonString;
    serializeJson(doc, jsonString);

    // 3. 构建HTTP请求头
    String httpRequest = String("POST /upload HTTP/1.1\r\n") +
                         "Host: " + serverHost + "\r\n" +
                         "Content-Type: application/json\r\n" +
                         "Content-Length: " + jsonString.length() + "\r\n" +
                         "Connection: close\r\n\r\n" +
                         jsonString; // 最后加上JSON数据

    // 4. 通过client发送请求到服务器
    client.print(httpRequest);
    Serial.println("数据发送完毕，等待服务器响应...");

    // 5. 等待并读取服务器的响应 (可选，用于调试)
    unsigned long timeout = millis();
    while (client.available() == 0) {
      if (millis() - timeout > 5000) {
        Serial.println(">>> 客户端超时！");
        client.stop();
        return;
      }
    }

    // 读取服务器的响应
    while (client.available()) {
      String line = client.readStringUntil('\r');
      Serial.print(line);
    }

    Serial.println("\n关闭连接。");
    client.stop(); // 关闭连接

  } else {
    Serial.println("连接到服务器失败！");
  }
}

// 模拟读取传感器的函数，你需要根据自己的硬件来写
void readSensors() {
  // 这里只是模拟数据，你要用 analogRead 或 pulseIn 等函数替换
  temperature = 25.0 + random(-10, 10) / 10.0;
  humidity = 60.0 + random(-20, 20) / 10.0;
  lightLevel = random(200, 800);
  Serial.println("传感器读取完成。");
}