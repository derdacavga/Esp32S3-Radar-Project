#include <Servo.h>
#include <U8g2lib.h>

// Sensor and Servo Pins
#define TRIG_PIN 3
#define ECHO_PIN 4
#define SERVO_PIN 5

// Max distance to track on the radar (in cm)
#define MAX_DISTANCE 100

Servo radarServo;

// Initialize HX1230 via Software SPI
U8G2_HX1230_96X68_F_3W_SW_SPI u8g2(U8G2_R2, 13, 11, 10, 8);

int angle = 0;
int direction = 1;

void setup() {
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  radarServo.attach(SERVO_PIN);
  radarServo.write(0);

  u8g2.begin();
  u8g2.setFont(u8g2_font_ncenB08_tr);
}

float getDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 30000);  // 30ms timeout (~500cm max)
  if (duration == 0) return MAX_DISTANCE + 1;
  return duration * 0.034 / 2.0;
}

void loop() {
  float distance = getDistance();

  u8g2.clearBuffer();

  // Setup radar canvas origin (bottom center of 96x68 screen)
  int origin_x = 48;
  int origin_y = 67;
  int radius = 45;

  // Draw wireframe radar arcs
  u8g2.drawCircle(origin_x, origin_y, radius, U8G2_DRAW_UPPER_RIGHT | U8G2_DRAW_UPPER_LEFT);
  u8g2.drawCircle(origin_x, origin_y, radius - 15, U8G2_DRAW_UPPER_RIGHT | U8G2_DRAW_UPPER_LEFT);
  u8g2.drawCircle(origin_x, origin_y, radius - 30, U8G2_DRAW_UPPER_RIGHT | U8G2_DRAW_UPPER_LEFT);

  // Calculate sweeping line endpoint
  float rad = angle * PI / 180.0;
  int line_x = origin_x - radius * cos(rad);
  int line_y = origin_y - radius * sin(rad);

  u8g2.drawLine(origin_x, origin_y, line_x, line_y);

  // Plot the detected object as a blip
  if (distance > 0 && distance <= MAX_DISTANCE) {
    int dot_dist = map(distance, 0, MAX_DISTANCE, 0, radius);
    int dot_x = origin_x - dot_dist * cos(rad);
    int dot_y = origin_y - dot_dist * sin(rad);

    // Draw a prominent circle for the object
    u8g2.drawDisc(dot_x, dot_y, 3);
  }
  // Overlay current distance data
  u8g2.setCursor(0, 10);
  if (distance <= MAX_DISTANCE) {
    u8g2.print("Dist: ");
    u8g2.print((int)distance);
    u8g2.print("cm");
  } else {
    u8g2.print("Clear");
  }
  u8g2.sendBuffer();
  // Step the servo
  radarServo.write(angle);
  angle += direction * 3;  // Sweep speed and resolution

  // Reverse direction at limits
  if (angle >= 180 || angle <= 0) {
    direction *= -1;
  }
  delay(100);  // Allow physical servo movement time
}