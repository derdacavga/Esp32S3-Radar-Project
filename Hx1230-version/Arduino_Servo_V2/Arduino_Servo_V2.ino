#include <Servo.h>
#include <U8g2lib.h>

#define TRIG_PIN 3
#define ECHO_PIN 4
#define SERVO_PIN 5

#define MAX_DISTANCE 100

Servo radarServo;

U8G2_HX1230_96X68_F_3W_SW_SPI u8g2(U8G2_R2, 13, 11, 10, 8);

int angle = 0;
int direction = 1;

void setup() {
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  radarServo.attach(SERVO_PIN);
  radarServo.write(180);

  u8g2.begin();
  u8g2.setFont(u8g2_font_ncenB08_tr);
}

float getDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 30000);
  if (duration == 0) return MAX_DISTANCE + 1;
  return duration * 0.034 / 2.0;
}

void loop() {
  float distance = getDistance();

  u8g2.clearBuffer();

  int origin_x = 48;
  int origin_y = 67;
  int radius = 45;

  u8g2.drawCircle(origin_x, origin_y, radius, U8G2_DRAW_UPPER_RIGHT | U8G2_DRAW_UPPER_LEFT);
  u8g2.drawCircle(origin_x, origin_y, radius - 15, U8G2_DRAW_UPPER_RIGHT | U8G2_DRAW_UPPER_LEFT);
  u8g2.drawCircle(origin_x, origin_y, radius - 30, U8G2_DRAW_UPPER_RIGHT | U8G2_DRAW_UPPER_LEFT);

  float rad = angle * PI / 180.0;
  int line_x = origin_x - radius * cos(rad);
  int line_y = origin_y - radius * sin(rad);

  u8g2.drawLine(origin_x, origin_y, line_x, line_y);

  if (distance > 0 && distance <= MAX_DISTANCE) {
    int dot_dist = map(distance, 0, MAX_DISTANCE, 0, radius);
    int dot_x = origin_x - dot_dist * cos(rad);
    int dot_y = origin_y - dot_dist * sin(rad);

    u8g2.drawDisc(dot_x, dot_y, 3);
  }

  u8g2.setCursor(0, 10);
  if (distance <= MAX_DISTANCE) {
    u8g2.print("Dist: ");
    u8g2.print((int)distance);
    u8g2.print("cm");
  } else {
    u8g2.print("Clear");
  }
  u8g2.sendBuffer();

  radarServo.write(180 - angle);
  angle += direction * 2;

  if (angle >= 180 || angle <= 0) {
    direction *= -1;
  }
  delay(100);
}
