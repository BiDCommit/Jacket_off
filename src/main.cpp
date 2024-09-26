

#include <Arduino.h>

// Подключаем библиотеки для работы с Ble на esp32
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLE2902.h>
#include <BLEUtils.h>
//#include "DHTesp.h"
#include <ESP32Servo.h> 
#include <EEPROM.h>
#include "SmartReliefPro.h"
#include "SSD1306Wire.h"   
#include "fontsRus.h"
//#include "DHT.h"
#include <Button2.h>
#include <vector>
#include <bitset>
#include <Preferences.h>
#include "DS18B20.h"
#define NAME_DEVICE "SJ_V3.0" // Имя которое будет отображаться в поиске блютус устройст

// Сервисы
#define SERVICE_CONTROL_UUID "e36b7a71-0c2a-4fff-b055-7ec727be37bd"  // Cервис для управления команд

//Характеристики
#define CONTROL_REQUEST_UUID  "fd660c79-7cef-40b5-9d6f-8789db614aca" // Характеристика для команд
#define CONTROL_RESPONSE_UUID "e6696221-d48d-4f88-90c2-7ac013fcc406" // Характеристика для ответа
#define TEMPERATURE_UUID      "06f8a67e-b89f-4961-ab25-6a82e0052c1f" // Характеристика для показания температуры

//Пины управления18
#define BUTTON_PIN_1 2 // - 
#define BUTTON_PIN_2 4  // +
#define BUTTON_PIN_3 15 // >
#define PIN_LED_FIRST  18 // Пин Led
#define PIN_LED_SECOND 5 // Пин Led
#define PIN_LED_THREE  17 //Пин Led
#define PIN_LED_FOUR   16  //Пин Led

#define ONE_WIRE_BUS  19

OneWire oneWire(ONE_WIRE_BUS);
DS18B20 sensor(&oneWire);


#define LED_FADE_DURATION 1000 // Длительность плавного включения/выключения в миллисекундах
u_int step=1;
hw_timer_t *timer = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;
volatile int currentActiveChannel = -1; // Текущий активный канал (-1, чтобы начать с 0)


#define PIN_LED_Ai   25   //Пин Led
#define PIN_LED_Auto 32 //Пин Led

//Команды
#define CMD_ENABLE_LED 0x1 // Команда на включения светодиода

// Раскомментируйте ту строку, которая соответствует вашему датчику
//#define DHTTYPE DHT11   // DHT 11
//#define DHTTYPE DHT21   // DHT 21 (AM2301)
//#define DHTTYPE DHT22  // DHT 22 (AM2302), AM2321
// Датчик DHT
Preferences preferences;
// Объявление объектов кнопок
Button2 button1(BUTTON_PIN_1);
Button2 button2(BUTTON_PIN_2);
Button2 button3(BUTTON_PIN_3);


struct TemperaturePWMData {
    float temperature; // Температура
    uint8_t pwmValues; // Массив для хранения 4 значений ШИМ (uint8_t)
};

const int arraySize = 500;

// Массив для хранения данных
TemperaturePWMData dataArray[arraySize];
uint16_t tempdat=0;
//uint8_t DHTPin = 4;///33
int timerab=0;
// Функция для проверки близости числа к 36.6 с учетом погрешности
bool isCloseToTarget(float value, float target, float tolerance) {
    return std::abs(value - target) <= tolerance;
}

// Функция для создания результирующего паттерна из данных
std::vector<uint8_t> createPattern(float targetTemperature, float tolerance) {
    std::vector<uint8_t> resultPattern;

    // Находим ближайшие к targetTemperature значения
    std::vector<std::pair<float, uint8_t>> closestValues;
    for (int i = 0; i < arraySize; ++i) {
        if (isCloseToTarget(dataArray[i].temperature, targetTemperature, tolerance)) {
            closestValues.push_back(std::make_pair(dataArray[i].temperature, dataArray[i].pwmValues));
        }
    }

    // Если нет подходящих значений, возвращаем пустой результат
    if (closestValues.empty()) {
        return resultPattern;
    }

    // Сортируем по близости температуры к targetTemperature
    std::sort(closestValues.begin(), closestValues.end(),
              [targetTemperature](const std::pair<float, uint8_t>& a, const std::pair<float, uint8_t>& b) {
                  return std::abs(a.first - targetTemperature) < std::abs(b.first - targetTemperature);
              });

    // Выбираем первые 20 значений (или меньше, если таких меньше 20)
    int count = std::min(20, static_cast<int>(closestValues.size()));
    for (int i = 0; i < count; ++i) {
        resultPattern.push_back(closestValues[i].second);
    }

    return resultPattern;
}
//Переменные
String menu[5]={"Паттерны","Количество зон","Температура","Время работы","нейросеть AI"};

  String heatingPatterns[] = {
        "Полная нагрузка",
        "Нейросеть AI",
        "Горячий Шторм",
        "Тепловой Кокон",
        "Огненный Вихрь",
        "Теплый Очаг",
        "Экваториальный Свет",
        "Южное Сияние",
        "Пламенное Сердце",
        "Горный Огонь",
        "Солнечный Зенит",
        "Убежище",
        "Тепловой Барьер",
        "Жаркая Волна",
        "Тропики",
        "Альпийское Тепло",
        "Теплый Отдых",
        "Золотое Пламя",
        "Лавовый Путь"
    };

    int patternCount = sizeof(heatingPatterns) / sizeof(heatingPatterns[0]);

u_int8_t menudef=0;
int menuclick=-1;
int oldclick=0;
bool fade=true;
u_int8_t defpm=0;
int valAuto = 0; // состояние пина авто
 SSD1306Wire display(0x3c, SDA, SCL,GEOMETRY_128_32); //инициализируем дисплей
// Сделал переменные характеристики (глобальные)
BLECharacteristic *controlRequest;
BLECharacteristic *controlResponse;
BLECharacteristic *temperatureCharacteristic;

BLEService *serviceControl;
BLEService *serviceControlTemp;

BLEServer *server;


//DHT dht(DHTPin, DHTTYPE);
struct DeviceStatus {
    float currentTemperature;   // Текущая температура
    float humidity;             // Влажность
    int workPattern;            // Паттерн работы (целое число)
    float maxTemperature;       // Максимальная температура
    unsigned long workTime;     // Время работы каждого элемента в миллисекундах
    unsigned long deviceUpTime; // Общее время работы устройства в миллисекундах
    bool elementStates[4];      // Состояние включенности 4 элементов

    // Конструктор по умолчанию
    DeviceStatus() 
        : currentTemperature(0.0), humidity(0.0), workPattern(0), maxTemperature(0.0), workTime(0), deviceUpTime(0) {
            for (int i = 0; i < 4; i++) {
                elementStates[i] = false; // Инициализация всех элементов как выключенные
            }
        }

    // Конструктор с параметрами
    DeviceStatus(float temp, float hum, int pattern, float maxTemp, unsigned long time, unsigned long upTime, bool states[4])
        : currentTemperature(temp), humidity(hum), workPattern(pattern), maxTemperature(maxTemp), workTime(time), deviceUpTime(upTime) {
            for (int i = 0; i < 4; i++) {
                elementStates[i] = states[i];
            }
        }
};

bool initialStates[4] = {true, false, true, false}; 
DeviceStatus device(25.5, 60.0, 1, 30.0, 3600000, millis(), initialStates);
float Temperature=33.0;
float Humidity;

void replacePattern(int channelIndex, const std::vector<uint8_t>& newPattern) {
    if (newPattern.size() > maxIndex) {
     
        return;
    }

    // Заменяем строку в массиве
    for (size_t i = 0; i < newPattern.size(); ++i) {
        pulseStrengths[channelIndex][i] = newPattern[i];
    }

    // Заполнение оставшихся элементов нулями, если новый паттерн меньше maxIndex
  
}

void saveDeviceStatus(DeviceStatus device) {
    // Открываем раздел NVS с именем "device", указывая, что мы будем его читать и писать
    preferences.begin("device", false);
    
    // Сохраняем значения полей структуры DeviceStatus
    preferences.putFloat("currentTemp", device.currentTemperature);
    preferences.putFloat("humidity", device.humidity);
    preferences.putInt("workPattern", device.workPattern);
    preferences.putFloat("maxTemp", device.maxTemperature);
    preferences.putULong("workTime", device.workTime);
    preferences.putULong("deviceUpTime", device.deviceUpTime);

    char key[15];
    for (int i = 0; i < 4; i++) {
        snprintf(key, sizeof(key), "elementState%d", i);
        preferences.putBool(key, device.elementStates[i]);
    }
    // Закрываем раздел NVS
    preferences.end();
}

DeviceStatus loadDeviceStatus() {;
    DeviceStatus device;

    // Открываем раздел NVS с именем "device", указывая, что мы будем его только читать
    preferences.begin("device", false);

    // Загружаем значения полей структуры DeviceStatus
    device.currentTemperature = preferences.getFloat("currentTemp", 0.0);
    device.humidity = preferences.getFloat("humidity", 0.0);
    device.workPattern = preferences.getInt("workPattern", 0);
    device.maxTemperature = preferences.getFloat("maxTemp", 30);
    device.workTime = preferences.getULong("workTime", 10);
    device.deviceUpTime = preferences.getULong("deviceUpTime", 0);

  
    char key[15];
    for (int i = 0; i < 4; i++) {
        snprintf(key, sizeof(key), "elementState%d", i);
        device.elementStates[i] = preferences.getBool(key, false);
    }
    
    // Закрываем раздел NVS
    preferences.end();
    return device;
}




void setupBluetooth();                 // Создаём сигнатуру BLE
void setupLeds();                      // Создаём сигнатуру LED
void enabledLed(int pin, bool enable); // Создаём сигнатура управления светодиодами
void enableAuto();                     //Создал сигнатуру режим авто
void drawdef();
void displayStatus();
void drawlin(int i, int val, int max);
void drawmenu(int i);
void  drawzon(int ii);
void   drawpatten(int ii);
void drawai();
std::vector<std::bitset<4>> generateCombinations() {
    std::vector<std::bitset<4>> combinations;
    for (int i = 0; i < 16; ++i) { // 16 = 2^4, все возможные комбинации 4 битов
        combinations.push_back(std::bitset<4>(i));
    }
    return combinations;
}

std::vector<std::bitset<4>> combinations = generateCombinations();
// Создаём слушатель событий
class BlutoothEventCallback : public BLECharacteristicCallbacks
{
  void onWrite(BLECharacteristic *characteristic)
  {
    
    uint8_t *data = characteristic->getData(); // Сюда априходит массив байт с телефона
    // далее происходит парсинг первого байта  и определяет какая команда пришла
    if (data[0] == CMD_ENABLE_LED)
    {
      enabledLed(data[1], data[2] > 0);
    }
  }
};


char FontUtf8Rus(const byte ch) { 
    static uint8_t LASTCHAR;

    if ((LASTCHAR == 0) && (ch < 0xC0)) {
      return ch;
    }

    if (LASTCHAR == 0) {
        LASTCHAR = ch;
        return 0;
    }

    uint8_t last = LASTCHAR;
    LASTCHAR = 0;
    
    switch (last) {
        case 0xD0:
            if (ch == 0x81) return 0xA8;
            if (ch >= 0x90 && ch <= 0xBF) return ch + 0x30;
            break;
        case 0xD1:
            if (ch == 0x91) return 0xB8;
            if (ch >= 0x80 && ch <= 0x8F) return ch + 0x70;
            break;
    }

    return (uint8_t) 0;
}
void menucall(){
  Serial.println(defpm);
Serial.println(menuclick);
if(menuclick<0) {drawdef(); oldclick=menuclick; return;}
     if(menuclick==1){


if(defpm==0) {
  drawpatten(menudef);
} else 
if(defpm==1) {
  drawzon(menudef);
}else
if(defpm==2) {
   
 
  drawlin(defpm,menudef,40);
 device.maxTemperature=menudef;
}
else
if(defpm==3) {
   
 
  drawlin(defpm,menudef,30);
 device.workTime=menudef;
 
}
else
if(defpm==4) {
   
 
  drawai();
 
 
}

else drawlin(defpm,menudef,5);
    } else {
       if (menudef+1> sizeof(menu) / sizeof(menu[0])) menudef=0;;
     Serial.println(menudef);
      drawmenu(menudef);
    }
    oldclick=menuclick;
}
void IRAM_ATTR onTimer() {
  
   

    portENTER_CRITICAL_ISR(&timerMux);
 ledcWrite(0,0);
ledcWrite(1,0);
ledcWrite(2,0);
ledcWrite(3,0);
uint8_t dfz=0;
 do {
    currentActiveChannel = (currentActiveChannel + 1) % 4; // Круговой переход по каналам от 0 до 3
    dfz++;
   
     if(dfz>8){currentActiveChannel=1; break;}
  } while (device.elementStates[currentActiveChannel] != 1);
 
// device.deviceUpTime=currentActiveChannel;
u_int8_t wp=device.workPattern;
if (wp<1) wp=1;if(wp>11)wp=11;

timerab++;if(timerab>240){timerab=0;
replacePattern(1,createPattern(36.6,1.0));

}
dataArray[timerab].temperature=device.currentTemperature;
dataArray[ timerab].pwmValues=stren;
  // Вызываем функцию управления для текущего активного канала
  callPulseFunction(wp,currentActiveChannel);
    portEXIT_CRITICAL_ISR(&timerMux);
    
    // Обновляем интервал таймера на основе workTime

}



void setup()
{
 device= loadDeviceStatus();
 maxvalue=device.maxTemperature;
  button1.setLongClickHandler([](Button2 & b) {
   menuclick++;
  if(menuclick==1) {
     {defpm=menudef;menudef=0;step=1;}
 if(defpm==2) {menudef=device.maxTemperature;step=5;}
  if(defpm==3) {menudef=device.workTime;step=5;}
  }   
   
if ((oldclick==1)&&(menuclick==2)){
Serial.println("save");step=1;
if(defpm==0) device.workPattern=menudef;
if(defpm==1){
for (size_t i = 0; i < combinations[menudef].size(); ++i) {
       
     device.elementStates[i]=combinations[menudef][i];
    }

}
if(defpm==3) {device.workTime=menudef;};
if(defpm==2) {device.maxTemperature=menudef;maxvalue=menudef;};
//

 timerAlarmDisable(timer);
    timerAlarmWrite(timer,device.workTime*1000000, true);
     timerAlarmEnable(timer);
 saveDeviceStatus(device);
}

   if(menuclick==2) {menuclick=-1;}
menucall();

    });
 
  button2.setLongClickHandler([](Button2 & b) {
     menudef+=step;
   
menucall();
    });

     button3.setLongClickHandler([](Button2 & b) {
     menudef-=step;; Serial.println("btn3");
     if(menudef<1)menudef=1;
menucall();
    });
  //pinMode(DHTPin, INPUT);
  //dht.begin();
  display.init();
  display.flipScreenVertically();
  display.setFontTableLookupFunction(FontUtf8Rus);
  display.setFont(ArialRus_Plain_24);

  Serial.begin(115200);           // Настроиваем скорость порта
  Serial.println("launching..."); // Выводим в порт
  

  setupBluetooth(); // Вызов функции BLE
  //setupLeds();      // Вызов функции Led



Serial.begin(115200);
    pinMode(PIN_LED_FIRST, OUTPUT);
        pinMode(PIN_LED_SECOND, OUTPUT);
        pinMode(PIN_LED_THREE, OUTPUT);
  pinMode(PIN_LED_FOUR, OUTPUT);

    ledcSetup(0, 5000, 8); // Канал 0, частота ШИМ 5 кГц, разрядность 8 бит
      ledcSetup(1, 5000, 8); // Канал 1, частота ШИМ 5 кГц, разрядность 8 бит
        ledcSetup(2, 5000, 8); // Канал 2, частота ШИМ 5 кГц, разрядность 8 бит
          ledcSetup(3, 5000, 8); // Канал 3, частота ШИМ 5 кГц, разрядность 8 бит
    ledcAttachPin(PIN_LED_FIRST, 0); // Привязываем канал 0 к пину PIN_LED_FIRST
    ledcAttachPin(PIN_LED_SECOND, 1); // Привязываем канал 0 к пину PIN_LED_FIRST
    ledcAttachPin(PIN_LED_THREE, 2); // Привязываем канал 0 к пину PIN_LED_FIRST
    ledcAttachPin(PIN_LED_FOUR, 3); // Привязываем канал 0 к пину PIN_LED_FIRST





  timer = timerBegin(0, 80, true); // Настраиваем таймер 0, делитель 80 (частота 80МГц), таймер не глубокоспящий
    timerAttachInterrupt(timer, &onTimer, true); // Подключаем обработчик прерывания
    timerAlarmWrite(timer, device.workTime*1000000, true); // Устанавливаем интервал таймера на основе workTime
    timerAlarmEnable(timer); // Включаем таймер



}
void drawdef() {
  display.clear();
  display.setFont(ArialRus_Plain_10); // Шрифт кегль 24
char buffer[10];

display.drawString(0, 0,heatingPatterns[device.workPattern]);
 display.drawLine(0,12,100,12); 
sprintf(buffer, "%04.1f", device.currentTemperature);
display.drawString(105, 0, "" + String(buffer));
//sprintf(buffer, "%04.1f", device.humidity);
//display.drawString(105, 11, "" + String(buffer));
 //display.drawLine(100,12,134,12); 
 //Serial.println(device.deviceUpTime/1000);
sprintf(buffer, "%04d", device.deviceUpTime/1000);// / 1000device.deviceUpTime/1000
display.drawString(105, 21,String(buffer));//String(device.deviceUpTime/1000)
display.drawLine(100,20,134,20);  
display.drawLine(100,0,100,34); 
sprintf(buffer, "%02.1f", device.maxTemperature);
display.drawString(70, 11,String(buffer));
 sprintf(buffer, "%02d", device.workTime);
display.drawString(70, 21,String(buffer));
display.drawLine(60,13,60,34);

for (int i = 0; i < 4; i++) {
    int x = 11 * i; // Смещение по оси X для каждого следующего прямоугольника
    if(device.elementStates[i])display.fillRect(x, 21, 10, 10); else    display.drawRect(x, 21, 10, 10); // Рисуем прямоугольник
    // Также можно добавить вашу строку здесь, если она нужна

//        display.drawString(64, 38 + i * 10, "Elem" + String(i + 1) + ": " + (device.elementStates[i] ? "ON" : "OFF"));
    }


 display.display();

}

void drawzon(int ii){
     display.clear();
     if(menudef>15)menudef=0;
  for (size_t i = 0; i < combinations[ii].size(); ++i) {
        if (combinations[ii][i]==0)
      display.drawRect(1+i*23,15,20,15); else display.fillRect(1+i*23,15,20,15);
    }
     display.setFont(ArialRus_Plain_10);
    display.drawString(0,0,String("Включенные зоны"));
     display.display();

}
void drawai(){uint8_t ax=0;
display.clear();
 display.setFont(ArialRus_Plain_10);
 // Serial.print(dataArray[i].pwmValues);Serial.print(",");
            display.drawString(0,0,"Ai intelecence");
display.drawRect(0,12,140,24);
for (size_t i = 0; i < 140; i++)
{

 
  
  
   long h = map(dataArray[i].pwmValues, 1, 255, 1, 18);
            display.drawLine(i, 12, i, 12 + h);
  

  
}

 display.display();
}
void drawpatten(int ii){
 display.clear();
     if(menudef>patternCount-1)menudef=0;
  
     display.setFont(ArialRus_Plain_14);
    display.drawString(0,0,String(heatingPatterns[ii]));
     display.display();
for (size_t iii = 0; iii < (sizeof(heatingPatterns) / sizeof(heatingPatterns[0])); iii++)
{
display.drawRect(0+iii*6,20,5,10);
}
 display.fillRect(0+ii*6,20,5,10);
  display.display();


}
void displayStatus() {
    display.clear();
    display.setFont(ArialMT_Plain_10);
    display.setTextAlignment(TEXT_ALIGN_LEFT);

    // Заголовок
   
    // Температура
    display.setFont(ArialMT_Plain_10);
    display.drawString(0, 0, "Temp: " + String()+" c");

    // Влажность
    display.drawString(64,0, "Hum: " + String(device.humidity)+ "%");

    // Паттерн работы
    display.drawString(0, 28, "Pattern: " + String(device.workPattern));

    // Максимальная температура
    display.drawString(64, 28, "MaxTemp: " + String(device.maxTemperature) + " C");

    // Время работы элемента
    display.drawString(0, 38, "Work Time: " + String(device.workTime / 1000) + " s");

    // Время работы устройства
    display.drawString(0, 48, "Up Time: " + String(device.deviceUpTime / 1000) + " s");

    // Состояние элементов
    for (int i = 0; i < 4; i++) {
        display.drawString(64, 38 + i * 10, "Elem" + String(i + 1) + ": " + (device.elementStates[i] ? "ON" : "OFF"));
    }

    display.display();
}
void drawlin(int i, int val, int max) {
       Serial.println(max); 
       val=menudef;
if(menudef>max)menudef=max;
      display.clear();
    display.setFont(ArialRus_Plain_10);
    display.drawString(0, 0, String(menu[i]));
    
    int barLength = map(val, 0, max-1, 0, 120); // Вычисляем длину прогресс-бара в зависимости от val и max
   
  //   display.drawProgressBar(1,20,120,10,50);
    display.fillRect(1, 20, barLength, 10); // Отрисовка прогресс-бара
      display.drawLine(0,18,160,18);
        display.drawString(110, 0, String(val));
    display.display();
}
void drawmenu(int i) {
  // Font Demo1
  // create more fonts at http://oleddisplay.squix.ch/
display.clear();
  display.setFont(ArialRus_Plain_16); // Шрифт кегль 24

  display.drawString(0, 0, menu[i]);
for (size_t ii = 0; ii < (sizeof(menu) / sizeof(menu[0])); ii++)
{
display.drawRect(1+ii*14,20,10,10);
}
 display.fillRect(1+i*14,20,10,10);
  display.display();
}
unsigned long previousMillis = 0;
const long interval = 1000;
void loop() {

 button1.loop();
  button2.loop();
    button3.loop();
 unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
 sensor.requestTemperatures();

  //  wait until sensor is ready
  
   Temperature =sensor.getTempC(); // Получаем значение температуры
     Humidity =0.0;


    // Отображение температуры в порт
  //  Serial.print(Temperature, 1);
   // Serial.println("\t");

    // Устанавливаем новое значение температуры в характеристике
    temperatureCharacteristic->setValue(Temperature);
    temperatureCharacteristic->notify(); // Уведомляем клиентов о новом значении температуры
device.currentTemperature=Temperature;
 device.deviceUpTime = millis(); // Обновление времени работы устройства
 /*if (menudef>0) drawmenu(menudef);else 
 drawdef();*/

  menucall();


  }

/*
  //в данном участке кода происходит автовключение зон прогрева  при включения команды авто  и его контроль
  valAuto = digitalRead(PIN_LED_Auto);//считываем состояния пина
  while (valAuto == HIGH)
  {
    valAuto = digitalRead(PIN_LED_Auto);//считываем состояния пина

    digitalWrite(PIN_LED_FIRST, HIGH);
    delay(1000);
    digitalWrite(PIN_LED_FIRST, LOW);

    digitalWrite(PIN_LED_SECOND, HIGH);
    delay(1000);
    digitalWrite(PIN_LED_SECOND, LOW);

    digitalWrite(PIN_LED_THREE, HIGH);
    delay(1000);
    digitalWrite(PIN_LED_THREE, LOW);

    digitalWrite(PIN_LED_FOUR, HIGH);
    delay(1000);
    digitalWrite(PIN_LED_FOUR, LOW);

//если выключаем режим авто то после запершения прохода происхордит выход из цикла
    if (valAuto == LOW)
    {
      digitalWrite(PIN_LED_FOUR, LOW);
      digitalWrite(PIN_LED_THREE, LOW);
      digitalWrite(PIN_LED_SECOND, LOW);
      digitalWrite(PIN_LED_FIRST, LOW);
      break;
    }
  }*/
}

// Функция реализации работы BLE
void setupBluetooth()
{
  BLEDevice::init(NAME_DEVICE);// Иницилизируем устройство с названием
  server = BLEDevice::createServer(); // Создаём сервер

  // Создаём сервис
  serviceControl = server->createService(SERVICE_CONTROL_UUID); // Создали сервис для управления команд
  //serviceControlTemp = server->createService(SERVICE_TEMP_UUID); // Создали сервис для управления команд
  // Создаём характеристику
  controlRequest = serviceControl->createCharacteristic(CONTROL_REQUEST_UUID, BLECharacteristic::PROPERTY_WRITE);
  controlRequest->setCallbacks(new BlutoothEventCallback()); // Для возврата

  controlResponse = serviceControl->createCharacteristic(CONTROL_RESPONSE_UUID, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
  controlResponse->addDescriptor(new BLE2902()); // Добавляем дескриптор

  temperatureCharacteristic = serviceControl->createCharacteristic(TEMPERATURE_UUID, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_WRITE);
  temperatureCharacteristic->addDescriptor(new BLE2902()); // Добавляем дескриптор

  //Запускаем  сервис
  serviceControl->start();

  //Открываем доступ к сервисам
  BLEAdvertising *advertising = BLEDevice::getAdvertising();
  advertising->addServiceUUID(SERVICE_CONTROL_UUID);
  advertising->setMinPreferred(0x06);//Для айфона
  advertising->setMinPreferred(0x12);//для айфона
  advertising->start();//запускаем сервис
}

//Функция для управления светодиодами
void setupLeds()
{
  pinMode(PIN_LED_FIRST, OUTPUT);
  pinMode(PIN_LED_SECOND, OUTPUT);
  pinMode(PIN_LED_THREE, OUTPUT);
  pinMode(PIN_LED_FOUR, OUTPUT);
  pinMode(PIN_LED_Ai, OUTPUT);
  pinMode(PIN_LED_Auto, OUTPUT);
}

// Функция проверки  уровня led
void enabledLed(int pin, bool enable)
{
  digitalWrite(pin, (enable) ? HIGH : LOW);
}

