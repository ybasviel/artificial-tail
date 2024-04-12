#include <ESP32Servo.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include "wifiConfig.h"

/*
  in wifiConfig.h

  const char ssid[] = "ssid";
  const char pass[] = "password";
*/

const int localPort = 10000;

const IPAddress ip(192, 168, 4, 1);
const IPAddress subnet(255, 255, 255, 0);

WiFiUDP udp;

struct SensorPosition {
  uint16_t front;
  uint16_t back;
};

struct Sensor {
  SensorPosition left;
  SensorPosition right;
};

enum SensorId {
  LEFT,
  RIGHT
};

unsigned char data[5];

volatile Sensor sensor = {};


Servo yoko1, yoko2, tate1, tate2;

volatile int formerTate1arg = 80;
volatile int formerTate2arg = 90;
volatile int formerYoko1arg = 90;
volatile int formerYoko2arg = 90;

volatile int yoko1arg = 90;
volatile int yoko2arg = 90;
volatile int tate1arg = 80;
volatile int tate2arg = 90;


hw_timer_t *timer1 = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;


void IRAM_ATTR onTimer1() {
  portENTER_CRITICAL_ISR(&timerMux);

  int leftVal  = 3*sensor.left.front - sensor.left.back;
  int rightVal = 3*sensor.right.front - sensor.right.back;

  int currentYoko1arg = map(leftVal - rightVal, -6000, 6000, 60, 120);
  int currentYoko2arg = map(leftVal - rightVal, -6000, 6000, 60, 140);

  int currentTate1arg = map(leftVal + rightVal, -6500, 8000, 0, 80);
  int currentTate2arg = map(leftVal + rightVal, -6000, 6500, 90, 120);

  const float T = 100;
  const float T_i = 500;

  tate1arg = T_i / (T + T_i) * formerTate1arg + T / (T + T_i) * currentTate1arg;
  tate2arg = T_i / (T + T_i) * formerTate2arg + T / (T + T_i) * currentTate2arg;

  yoko1arg = T_i / (T + T_i) * formerYoko1arg + T / (T + T_i) * currentYoko1arg;
  yoko2arg = T_i / (T + T_i) * formerYoko2arg + T / (T + T_i) * currentYoko2arg;

  formerTate1arg = tate1arg;
  formerTate2arg = tate2arg;

  formerYoko1arg = yoko1arg;
  formerYoko2arg = yoko2arg;

  yoko1.write(yoko1arg);
  yoko2.write(yoko2arg);
  tate1.write(tate1arg);
  tate2.write(tate2arg);

  portEXIT_CRITICAL_ISR(&timerMux);
}

void setup() {
  WiFi.softAP(ssid, pass);
  delay(100);
  WiFi.softAPConfig(ip, ip, subnet);

  IPAddress myIP = WiFi.softAPIP();
  udp.begin(localPort);


  yoko1.attach(2, 500, 2500);
  yoko2.attach(17, 500, 2400);
  tate1.attach(21, 500, 2500);
  tate2.attach(4, 500, 2400);

  yoko1.write(90);
  yoko2.write(90);
  tate1.write(80);
  tate2.write(90);


  timer1 = timerBegin(0, 80, true);
  timerAttachInterrupt(timer1, &onTimer1, true);
  timerAlarmWrite(timer1, 100000, true);
  timerAlarmEnable(timer1);
}

void loop() {
  if (udp.parsePacket()) {
    udp.read(data, 5);

    if (data[0] == LEFT) {
      sensor.left.front = data[1] + (data[2] << 8);
      sensor.left.back = data[3] + (data[4] << 8);
    } else {
      sensor.right.front = data[1] + (data[2] << 8);
      sensor.right.back = data[3] + (data[4] << 8);
    }
  }
}
