#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>
#include <BH1750.h>

// --- 1. 双 OLED 显示屏配置 ---
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

Adafruit_SSD1306 displayPlant(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);  // 0x3C: 显示植物信息
Adafruit_SSD1306 displaySensor(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET); // 0x3D: 显示传感器与硬件状态

// --- 2. 传感器配置 ---
#define DHTPIN 2       
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);
BH1750 lightMeter;     

// --- 3. NMOS 风扇电机配置 ---
const int FAN_PIN = 7;       // NMOS 控制端接 Digital Pin 7 (支持 PWM)
const int FAN_SPEED = 150;   // PWM 风扇转速值，范围 0-255。150 约为 60% 转速

// --- 4. HW-269 3W LED 补光配置 ---
const int LED_PIN = 9; // HW-269 信号控制端接 Digital Pin 9 (支持 PWM)
const int LED_BRIGHTNESS = 255; // PWM 亮度值，范围 0-255。50 大约是 20% 亮度

// --- 5. NMOS 水泵配置 ---
const int PUMP_PIN = 6;       // NMOS 控制端接 Digital Pin 6

// --- 6. 系统全局状态变量 ---
char currentCommand = 'N';           
unsigned long lastSensorRead = 0;    
float currentTemp = 0.0;
float currentHum = 0.0;
float currentLux = 0.0; // 【同步代码1】改为 float 以便保留小数位

// 通讯与任务定时器变量
unsigned long lastSerialTime = 0;    // 记录上一次收到 YOLO 数据的时间
unsigned long lastPumpToggleTime = 0;// 记录上一次水泵切换状态的时间
unsigned long lastJsonSendTime = 0;  // 【整合代码1】记录上一次发送 JSON 数据的时间
bool isPumpOn = false;               // 当前水泵状态

void setup() {
  Serial.begin(9600);   // 连接电脑，接收 YOLO 指令及打印发送日志
  Serial1.begin(9600);  // 【整合代码1】连接 ESP32，用于发送数据 (引脚 18 TX1, 19 RX1)

  Wire.begin();

  // 初始化 NMOS (风扇电机)
  pinMode(FAN_PIN, OUTPUT);
  analogWrite(FAN_PIN, 0);

  // 初始化 NMOS (水泵)
  pinMode(PUMP_PIN, OUTPUT);
  digitalWrite(PUMP_PIN, LOW); 

  // 初始化 HW-269 LED
  pinMode(LED_PIN, OUTPUT);
  analogWrite(LED_PIN, 0); 

  // 初始化传感器
  dht.begin();
  if (lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE)) {
    Serial.println(F("BH1750 初始化成功"));
  } else {
    Serial.println(F("BH1750 初始化失败"));
  }
  Serial.println(F("DHT11 已启动"));

  // 初始化双屏
  if(!displayPlant.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println(F("OLED 0x3C Init Failed"));
    for(;;); 
  }
  if(!displaySensor.begin(SSD1306_SWITCHCAPVCC, 0x3D)) { 
    Serial.println(F("OLED 0x3D Init Failed"));
    for(;;); 
  }
  
  // 开机画面
  displayPlant.clearDisplay(); displayPlant.setTextSize(1); displayPlant.setTextColor(SSD1306_WHITE);
  displayPlant.setCursor(0, 20); displayPlant.println("AIoT System Ready"); displayPlant.display();

  displaySensor.clearDisplay(); displaySensor.setTextSize(1); displaySensor.setTextColor(SSD1306_WHITE);
  displaySensor.setCursor(0, 20); displaySensor.println("Sensors Ready"); displaySensor.display();
  
  delay(2000);
}

void loop() {
  bool needUpdateScreen = false;
  unsigned long currentMillis = millis();

  // --- 任务 1: 监听 Python 的 YOLO 串口数据与超时保护 ---
  if (Serial.available() > 0) {
    char incomingChar = Serial.read();
    if (incomingChar == '0' || incomingChar == '1' || incomingChar == '2' || incomingChar == 'N') {
      lastSerialTime = currentMillis; 
      if (incomingChar != currentCommand) {
        currentCommand = incomingChar;
        needUpdateScreen = true; 
      }
    }
  }

  if (currentCommand != 'N' && (currentMillis - lastSerialTime > 1500)) {
    currentCommand = 'N';
    needUpdateScreen = true;
  }

  // --- 任务 2: 传感器定时读取 (每 2 秒读取一次，供屏幕和 JSON 共享使用) ---
  if (currentMillis - lastSensorRead > 2000) {
    currentTemp = dht.readTemperature();
    currentHum = dht.readHumidity();
    currentLux = lightMeter.readLightLevel();

    // 【整合代码1】排除 NaN 错误，防止发送无效 JSON
    if (isnan(currentTemp) || isnan(currentHum)) {
      Serial.println(F("警告: 无法从 DHT11 读取数据，请检查接线！"));
      currentTemp = 0.0;
      currentHum = 0.0;
    }

    lastSensorRead = currentMillis;
    needUpdateScreen = true; 
  }

  // --- 任务 3: 水泵间歇控制逻辑 ---
  if (currentCommand != 'N') {
    if (isPumpOn) {
      if (currentMillis - lastPumpToggleTime >= 2000) { 
        isPumpOn = false;
        digitalWrite(PUMP_PIN, LOW); 
        lastPumpToggleTime = currentMillis;
        needUpdateScreen = true;
      }
    } else {
      if (currentMillis - lastPumpToggleTime >= 5000) { 
        isPumpOn = true;
        digitalWrite(PUMP_PIN, HIGH); 
        lastPumpToggleTime = currentMillis;
        needUpdateScreen = true;
      }
    }
  } else {
    if (isPumpOn) {
      isPumpOn = false;
      digitalWrite(PUMP_PIN, LOW);
      needUpdateScreen = true;
    }
    lastPumpToggleTime = currentMillis; 
  }

  // --- 任务 4: 【核心整合】向 ESP32 发送 JSON 数据 (每 6 秒触发一次) ---
  // 使用 millis() 替代 delay(6000)，保证系统完全不卡顿
  if (currentMillis - lastJsonSendTime >= 6000) {
    
    // 土壤湿度暂时还没接，保留 0.0
    float moist = 0.0;

    // 构造标准的 JSON 字符串 (严格采用代码 1 的格式)
    String jsonPayload = "{\"temperature\":" + String(currentTemp, 1) + 
                         ",\"humidity\":" + String(currentHum, 1) + 
                         ",\"light\":" + String(currentLux, 2) + 
                         ",\"moisture\":" + String(moist, 1) + "}";

    // 1. 打印到电脑串口，方便确认 Mega 发了什么
    Serial.print("发给 ESP32-S3 的数据: ");
    Serial.println(jsonPayload);

    // 2. 实际发送给 ESP32-S3
    Serial1.println(jsonPayload);

    // 更新时间戳
    lastJsonSendTime = currentMillis;
  }

  // --- 任务 5: 刷新屏幕与执行主硬件动作 ---
  if (needUpdateScreen) {
    updateSystemUI();
  }
}

// === UI 绘制与主硬件控制 ===
void updateSystemUI() {
  displayPlant.clearDisplay(); displayPlant.setCursor(0, 0);
  displaySensor.clearDisplay(); displaySensor.setCursor(0, 0);

  // 【注】此处使用了你刚才修正后的目标温度范围
  float activeMaxTemp = 99.0; 
  uint16_t activeMinLux = 0;  

  // ==========================================
  // 屏幕 1 (0x3C): 植物种类
  // ==========================================
  displayPlant.setTextSize(2); 
  
  if (currentCommand == '0') {
    displayPlant.println("CACTUS"); displayPlant.setTextSize(1); displayPlant.println("---------------------");
    displayPlant.println("Temp: 18 - 32 C"); displayPlant.println("Hum : 10 - 30 %"); displayPlant.println("Lux : > 5000 lx");
    activeMaxTemp = 15.0; activeMinLux = 5000;
  } 
  else if (currentCommand == '1') {
    displayPlant.println("POTHOS"); displayPlant.setTextSize(1); displayPlant.println("---------------------");
    displayPlant.println("Temp: 15 - 29 C"); displayPlant.println("Hum : 50 - 70 %"); displayPlant.println("Lux : > 1000 lx");
    activeMaxTemp = 29.0; activeMinLux = 1000;
  } 
  else if (currentCommand == '2') {
    displayPlant.println("SUCCULENT"); displayPlant.setTextSize(1); displayPlant.println("---------------------");
    displayPlant.println("Temp: 15 - 28 C"); displayPlant.println("Hum : 30 - 50 %"); displayPlant.println("Lux : > 4000 lx");
    activeMaxTemp = 15.0; activeMinLux = 4000;
  } 
  else {
    displayPlant.println("NO PLANT"); displayPlant.setTextSize(1); displayPlant.println("---------------------");
    displayPlant.println("Waiting for target...");
    activeMaxTemp = 99.0; activeMinLux = 0; 
  }

  // ==========================================
  // 屏幕 2 (0x3D): 实时环境与外设状态
  // ==========================================
  displaySensor.setTextSize(2);
  displaySensor.println("SYSTEM");
  displaySensor.setTextSize(1);
  displaySensor.println("---------------------");
  
  // 1. 温度与 NMOS 风扇电机状态 (PWM控制)
  displaySensor.print("T:"); displaySensor.print(currentTemp, 1); displaySensor.print("C F:");
  if (currentTemp > activeMaxTemp && currentCommand != 'N') {
    displaySensor.print("ON("); 
    displaySensor.print(map(FAN_SPEED, 0, 255, 0, 100)); 
    displaySensor.println("%)");
    analogWrite(FAN_PIN, FAN_SPEED); 
  } else {
    displaySensor.println("OFF"); 
    analogWrite(FAN_PIN, 0); 
  }

  // 2. 湿度与水泵状态 
  displaySensor.print("H:"); displaySensor.print(currentHum, 1); displaySensor.print("% P:");
  if (isPumpOn) {
    displaySensor.println("ON");
  } else {
    displaySensor.println("OFF");
  }
  
  // 3. 光照与补光灯状态
  displaySensor.print("L:"); displaySensor.print(currentLux, 0); displaySensor.print("lx D:");
  
  // 使用 PWM 控制亮度和熄灭
  if (currentLux < activeMinLux && currentCommand != 'N') {
    displaySensor.print("ON (");
    displaySensor.print(map(LED_BRIGHTNESS, 0, 255, 0, 100)); 
    displaySensor.println("%)");
    
    analogWrite(LED_PIN, LED_BRIGHTNESS); 
  } else {
    displaySensor.println("OFF");
    analogWrite(LED_PIN, 0);  
  }

  displayPlant.display(); 
  displaySensor.display(); 
}
