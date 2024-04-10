#include <WiFi.h>
#include <WiFiUdp.h>
#include "M5StickCPlus2.h"
#include "wifiConfig.h"

/*
  in wifiConfig.h

  const char ssid[] = "ssid";
  const char pass[] = "password";
*/


#define LEFT
//#define RIGHT

static WiFiUDP udp;
static const char *remoteIp = "192.168.4.1";
static const int remotePort = 10000;


enum SensorId {
  LEFT_ID,
  RIGHT_ID
};


uint16_t maxFrontSensorVal = 3000;
uint16_t minFrontSensorVal = 0;

uint16_t maxBackSensorVal = 3000;
uint16_t minBackSensorVal = 0;


void setup() {
  auto cfg = M5.config();
  StickCP2.begin(cfg);
  StickCP2.Display.setRotation(0);
  StickCP2.Display.setTextColor(WHITE);
  StickCP2.Display.setTextDatum(middle_center);
  StickCP2.Display.setFont(&fonts::FreeSansBold9pt7b);
  StickCP2.Display.setTextSize(1);

  StickCP2.Display.setCursor(0, 40);
  StickCP2.Display.clear();

#ifdef LEFT
  StickCP2.Display.printf("Left:\r\n");
#else
  StickCP2.Display.printf("Right:\r\n");
#endif

  // キャリブレーション
  maxFrontSensorVal = 0;
  minFrontSensorVal = 3000;
  maxBackSensorVal = 0;
  minBackSensorVal = 3000;

  for(uint16_t time = 0; time*100 < 3000 /*ms*/; time++ ){
    uint16_t front = analogRead(G32);
    uint16_t back = analogRead(G33);

    maxFrontSensorVal = (maxFrontSensorVal > front) ? maxFrontSensorVal : front;
    minFrontSensorVal = (minFrontSensorVal < front) ? minFrontSensorVal : front;
    maxBackSensorVal = (maxBackSensorVal > back) ? maxBackSensorVal : back;
    minBackSensorVal = (minBackSensorVal < back) ? minBackSensorVal : back;

    delay(100);
  }

  // キャリブレーション終了ビープ音
  StickCP2.Speaker.tone(4400, 1000);


  static const int localPort = 5000;

  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  udp.begin(localPort);
}

uint8_t data[5];

void loop() {
  uint16_t front = map(analogRead(G32), minFrontSensorVal, maxFrontSensorVal, 0, 3000);
  uint16_t back = map(analogRead(G33), minBackSensorVal, maxBackSensorVal, 0, 3000);


  StickCP2.Display.setCursor(0, 40);
  StickCP2.Display.clear();

#ifdef LEFT
  StickCP2.Display.printf("Left:\r\n");
#else
  StickCP2.Display.printf("Right:\r\n");
#endif

  StickCP2.Display.printf("Front: %d\r\n", front);
  StickCP2.Display.printf("%4d ~ %4d\r\n", minFrontSensorVal, maxFrontSensorVal);
  StickCP2.Display.printf("Back: %d\r\n", back);
  StickCP2.Display.printf("%4d ~ %4d\r\n", minBackSensorVal, maxBackSensorVal);

#ifdef LEFT
  data[0] = LEFT_ID;
#else
  data[0] = RIGHT_ID;
#endif
  data[1] = front & 0xff;
  data[2] = (front >> 8) & 0xff;
  data[3] = back & 0xff;
  data[4] = (back >> 8) & 0xff;

  udp.beginPacket(remoteIp, remotePort);
  udp.write((const uint8_t *)data, 5);
  udp.endPacket();

  delay(100);
}