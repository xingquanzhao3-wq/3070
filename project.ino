
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>
#include <BH1750.h>

// ================= OLED 屏幕设置 =================
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ================= DHT11 设置 =================
#define DHTPIN 2       // DHT11 数据引脚连接到 Mega 的数字引脚 2
#define DHTTYPE DHT11  
DHT dht(DHTPIN, DHTTYPE);

// ================= BH1750 设置 =================
BH1750 lightMeter;

void setup() {
  Serial.begin(9600);
  Wire.begin(); // 初始化 I2C 总线，这一句对于 Mega 驱动多个 I2C 设备很重要

  Serial.println(F("正在初始化综合环境监测系统..."));

  // 1. 初始化 OLED
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println(F("OLED 初始化失败！"));
    for(;;); 
  }

  // 2. 初始化 DHT11
  dht.begin();

  // 3. 初始化 BH1750
  if (lightMeter.begin()) {
    Serial.println(F("BH1750 初始化成功"));
  } else {
    Serial.println(F("BH1750 初始化失败！"));
  }

  // 显示开机画面
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(20, 25);
  display.println(F("System Ready!"));
  display.display();
  delay(1500); // 停留 1.5 秒
}

void loop() {
  // ==========================================
  // 第一页：读取并显示温湿度 (停留 1 秒)
  // ==========================================
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print(F("--- Temp & Humi ---")); // 标题
  display.drawLine(0, 10, 128, 10, SSD1306_WHITE);

  // 检查 DHT11 是否读取成功
  if (isnan(h) || isnan(t)) {
    display.setCursor(0, 30);
    display.setTextSize(2);
    display.print(F("DHT Error!"));
  } else {
    // 显示温度
    display.setTextSize(2);
    display.setCursor(0, 20);
    display.print(F("T:"));
    display.print(t, 1);
    display.setTextSize(1);
    display.print(F(" C"));

    // 显示湿度
    display.setTextSize(2);
    display.setCursor(0, 45);
    display.print(F("H:"));
    display.print(h, 1);
    display.setTextSize(1);
    display.print(F(" %"));
  }
  
  display.display(); // 推送到屏幕
  delay(1000);       // 第一页停留 1 秒

  // ==========================================
  // 第二页：读取并显示光照度 (停留 1 秒)
  // ==========================================
  float lux = lightMeter.readLightLevel();

  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print(F("--- Illuminance ---")); // 标题
  display.drawLine(0, 10, 128, 10, SSD1306_WHITE);

  // 显示光照数据
  display.setTextSize(2);
  display.setCursor(0, 30);
  display.print(lux, 1); // 居中显示数值
  
  display.setTextSize(1);
  display.print(F(" Lux"));
  
  // 底部加个小提示
  display.setCursor(0, 55);
  display.print(F("Light Sensor"));

  display.display(); // 推送到屏幕
  delay(1000);       // 第二页停留 1 秒
  
  // loop 结束，重新回到第一页，总耗时正好约 2 秒
}