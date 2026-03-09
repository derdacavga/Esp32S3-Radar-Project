#include <U8g2lib.h>
#include <SPI.h>
#include <Servo.h>

#define TRIG_PIN 3
#define ECHO_PIN 4
#define SERVO_PIN 5

// Clock = 13, Data = 11, CS = 10, Reset = 8
U8G2_HX1230_96X68_F_3W_SW_SPI u8g2(U8G2_R2, 13, 11, 10, 8);
Servo radarServo;

int currentAngle = 0;
int sweepDirection = 1;
long duration;
int distance;

int prevLineEndX = 48;
int prevLineEndY = 67;

void setup() {
  Serial.begin(9600);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  radarServo.attach(SERVO_PIN);
  radarServo.write(180 - currentAngle);

  u8g2.begin();
  u8g2.setFont(u8g2_font_6x10_tr);
  u8g2.clearBuffer();
}

void loop() {
  radarServo.write(180 - currentAngle);
  delay(50);

  distance = getDistance();
  updateDisplay(currentAngle, distance);

  currentAngle += sweepDirection;

  if (currentAngle >= 180) {
    currentAngle = 180;
    sweepDirection = -1;
    u8g2.clearBuffer();
  } else if (currentAngle <= 0) {
    currentAngle = 0;
    sweepDirection = 1;
    u8g2.clearBuffer();
  }
}

int getDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);

  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  duration = pulseIn(ECHO_PIN, HIGH, 30000);

  if (duration == 0) return 200;

  int dist = duration * 0.034 / 2;

  if (dist == 0 || dist > 200) {
    dist = 200;
  }

  return dist;
}

void updateDisplay(int angle, int dist) {
  u8g2.setDrawColor(0);
  u8g2.drawBox(0, 0, 96, 12);

  u8g2.drawLine(48, 67, prevLineEndX, prevLineEndY);

  u8g2.setDrawColor(1);

  u8g2.setCursor(0, 10);
  u8g2.print("Ang: ");
  u8g2.print(angle);
  u8g2.print((char)176);

  u8g2.setCursor(50, 10);
  u8g2.print("Dst: ");
  if (dist == 200) u8g2.print("--");
  else u8g2.print(dist);

  int originX = 48;
  int originY = 67;
  float radians = angle * PI / 180.0;

  if (dist < 100) {
    int blipRadius = map(dist, 0, 100, 0, 48);
    int blipX = originX - (blipRadius * cos(radians));
    int blipY = originY - (blipRadius * sin(radians));

    u8g2.drawBox(blipX - 1, blipY - 1, 3, 3);
  }

  int maxRadius = 47;
  int endX = originX - (maxRadius * cos(radians));
  int endY = originY - (maxRadius * sin(radians));
  u8g2.drawLine(originX, originY, endX, endY);

  prevLineEndX = endX;
  prevLineEndY = endY;

  u8g2.drawCircle(originX, originY, 48, U8G2_DRAW_UPPER_RIGHT | U8G2_DRAW_UPPER_LEFT);

  u8g2.sendBuffer();
}