#include "SPI.h"
#include "TFT_eSPI.h"
#include <ESP32Servo.h>
 
const int TRIG_PIN = 7;
const int ECHO_PIN = 16;
const int SERVO_PIN = 17;
 
TFT_eSPI tft = TFT_eSPI();
TFT_eSprite radarSprite = TFT_eSprite(&tft);
Servo myServo;
 
const int SCREEN_W = 320;
const int SCREEN_H = 240;
const int CX = SCREEN_W / 2;
const int CY = SCREEN_H - 10;
const int MAX_RADIUS = 150;
const int MAX_RANGE_CM = 50; //2-400
const int ANGLE_MIN = 0;
const int ANGLE_MAX = 180;
const int ANGLE_STEP = 2;
 
unsigned long lastSweepTime = 0;
unsigned long sweepInterval = 30;
unsigned long lastMeasureTime = 0;
unsigned long measureInterval = 50;
 
int currentAngle = ANGLE_MIN;
int sweepDir = 1;
 
struct Detection {
  uint8_t angle;
  uint16_t dist_cm;
};
#define MAX_DETECTIONS 200
Detection detections[MAX_DETECTIONS];
int det_count = 0;
 
#define TRAIL_DECAY 15  
uint8_t fadeBuffer[MAX_RADIUS];
 
int prevX = CX;
int prevY = CY;
 
int distToRadius(uint16_t dcm) {
  if (dcm >= MAX_RANGE_CM) return MAX_RADIUS;
  return (int)(((long)dcm * MAX_RADIUS) / MAX_RANGE_CM);
}

unsigned int readUltrasonicCM() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  unsigned long duration = pulseIn(ECHO_PIN, HIGH, 30000UL);
  if (duration == 0) return 0;
  return (unsigned int)(duration / 58.0);
}
 
void drawRadarBackground() {
  radarSprite.fillSprite(TFT_BLACK);

  for (int r = MAX_RADIUS; r > 0; r -= 22) {
    radarSprite.drawCircle(CX, CY, r, TFT_DARKGREY);
  }
  radarSprite.drawLine(CX - MAX_RADIUS, CY, CX + MAX_RADIUS, CY, TFT_DARKGREY);

  for (int a = 0; a <= 180; a += 30) {
    float rad = a * PI / 180.0;
    int x = CX + (int)(MAX_RADIUS * cos(rad));
    int y = CY - (int)(MAX_RADIUS * sin(rad));
    radarSprite.drawLine(CX, CY, x, y, TFT_DARKGREY);
  }

  radarSprite.setTextColor(TFT_WHITE);
  radarSprite.drawString("ESP32 Radar", 6, 6, 2);
  radarSprite.drawString("Max: " + String(MAX_RANGE_CM) + "cm", 6, 26, 1);
}
 
void fadeRadar() { 
  for (int y = 0; y < SCREEN_H; y += 2) {
    for (int x = 0; x < SCREEN_W; x += 2) {
      uint16_t c = radarSprite.readPixel(x, y);
      if (c != TFT_BLACK && c != TFT_DARKGREY) {
        uint8_t r = (c >> 11) & 0x1F;
        uint8_t g = (c >> 5) & 0x3F;
        uint8_t b = c & 0x1F;
        r = (r > 0) ? r - 1 : 0;
        g = (g > 0) ? g - 1 : 0;
        b = (b > 0) ? b - 1 : 0;
        uint16_t faded = (r << 11) | (g << 5) | b;
        radarSprite.drawPixel(x, y, faded);
      }
    }
  }
}
 
void drawDetections() {
  for (int i = 0; i < det_count; ++i) {
    uint8_t a = detections[i].angle;
    uint16_t d = detections[i].dist_cm;
    if (d == 0 || d > MAX_RANGE_CM) continue;
    float rad = a * PI / 180.0;
    int rpx = distToRadius(d);
    int x = CX + (int)(rpx * cos(rad));
    int y = CY - (int)(rpx * sin(rad));
    uint16_t color = (d < 25) ? TFT_RED : (d < 40 ? TFT_WHITE : TFT_BLUE);
    radarSprite.fillCircle(x, y, 3, color);
  }
}
 
void updateRadarSweep(int angle) {
  float rad = angle * PI / 180.0;
  int x = CX + (int)(MAX_RADIUS * cos(rad));
  int y = CY - (int)(MAX_RADIUS * sin(rad));
  radarSprite.drawLine(CX, CY, x, y, TFT_GREEN);
  prevX = x;
  prevY = y;
}
 
void setup() {
  Serial.begin(115200);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  digitalWrite(TRIG_PIN, LOW);

  myServo.setPeriodHertz(50);
  myServo.attach(SERVO_PIN);
  myServo.write(currentAngle);

  tft.init();
  tft.setRotation(3); //0-3
  tft.fillScreen(TFT_BLACK);

  radarSprite.createSprite(SCREEN_W, SCREEN_H);
  radarSprite.setColorDepth(16);

  drawRadarBackground();
  radarSprite.pushSprite(0, 0);
}

void loop() {
  unsigned long now = millis();
 
  if (now - lastSweepTime >= sweepInterval) {
    lastSweepTime = now;
    currentAngle += sweepDir * ANGLE_STEP;

    if (currentAngle >= ANGLE_MAX) {
      sweepDir = -1;
      det_count = 0;  
      drawRadarBackground();
    } else if (currentAngle <= ANGLE_MIN) {
      sweepDir = 1;
      det_count = 0;
      drawRadarBackground();
    }

    myServo.write(currentAngle);

    fadeRadar();   
    updateRadarSweep(currentAngle);
 
    radarSprite.fillRect(200, 6, 114, 20, TFT_BLACK);
    radarSprite.setTextColor(TFT_WHITE, TFT_BLACK);
    radarSprite.drawString("A:" + String(currentAngle) + "°", 202, 10, 1);

    radarSprite.pushSprite(0, 0);
  }
 
  if (now - lastMeasureTime >= measureInterval) {
    lastMeasureTime = now;
    unsigned int d = readUltrasonicCM();

    radarSprite.fillRect(200, 26, 114, 20, TFT_BLACK);
    radarSprite.setTextColor(TFT_YELLOW, TFT_BLACK);
    radarSprite.drawString("D:" + String(d) + "cm", 202, 30, 1);

    if (d > 0 && d <= MAX_RANGE_CM) {
      if (det_count < MAX_DETECTIONS) {
        detections[det_count++] = { (uint8_t)currentAngle, d };
      }

      float rad = currentAngle * PI / 180.0;
      int rpx = distToRadius(d);
      int x = CX + (int)(rpx * cos(rad));
      int y = CY - (int)(rpx * sin(rad));
      uint16_t color = (d < 25) ? TFT_RED : (d < 40 ? TFT_YELLOW : TFT_GREEN);
      radarSprite.fillCircle(x, y, 3, color);
    }
  }
}
