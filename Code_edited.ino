#include <Wire.h>
#include <MPU6050_light.h>

// ─── PINS ──────────────────────────────────────────────────
#define CURRENT_SENSOR_PIN  34
#define IN1                 16
#define IN2                 17
#define ENA                  5

// ─── CONFIG ────────────────────────────────────────────────
#define SAMPLE_INTERVAL_MS  20      // 50 Hz → one row every 20ms
#define RECORD_DURATION_MS  120000  // 2 minutes per file

// ─── ACS712 ────────────────────────────────────────────────
#define ACS712_SENSITIVITY  0.185f
#define ACS712_ZERO_V       1.65f
#define ACS712_VREF         3.3f
#define ADC_RESOLUTION      4095.0f

MPU6050 mpu(Wire);

unsigned long start_time    = 0;
unsigned long last_sample   = 0;
int           sample_count  = 0;

// ─── MOTOR ─────────────────────────────────────────────────
void motor_start() {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  analogWrite(ENA, 255);
}

void motor_stop() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  analogWrite(ENA, 0);
}

// ─── SETUP ─────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  delay(1000);

  // motor pins
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(ENA, OUTPUT);
  motor_stop();

  // ADC
  analogReadResolution(12);
  pinMode(CURRENT_SENSOR_PIN, INPUT);

  // MPU6050
  Wire.begin(21, 22);
  
  byte status = mpu.begin();
  if (status != 0) {
    Serial.print("ERROR: MPU6050 initialization failed with code: ");
    Serial.println(status);
    while (1) delay(1000); // halt
  }
  
  Serial.println("Calculating offsets, do not move MPU6050...");
  delay(1000);
  mpu.calcOffsets(true, true); // Calibrate gyro and accelerometer
  Serial.println("Calibration Done!");

  delay(500);

  // print CSV header
  Serial.println("timestamp_ms,x_g,y_g,z_g,current_A");

  // start pump
  motor_start();
  start_time  = millis();
  last_sample = millis();
}

// ─── LOOP ──────────────────────────────────────────────────
void loop() {
  unsigned long now = millis();

  // stop after 2 minutes
  if (now - start_time >= RECORD_DURATION_MS) {
    motor_stop();
    Serial.println("# DONE — 2 minutes recorded");
    while (1) delay(1000);  // halt
  }

  // sample every 20ms
  if (now - last_sample >= SAMPLE_INTERVAL_MS) {
    last_sample = now;

    // Refresh MPU6050 data
    mpu.update();

    // MPU6050_light returns accelerometer values directly in g
    float x_g = mpu.getAccX();
    float y_g = mpu.getAccY();
    float z_g = mpu.getAccZ();

    // read current
    float raw_v   = (analogRead(CURRENT_SENSOR_PIN) / ADC_RESOLUTION) * ACS712_VREF;
    float current = (raw_v - ACS712_ZERO_V) / ACS712_SENSITIVITY;

    // timestamp relative to start
    unsigned long ts = now - start_time;

    // print CSV row
    Serial.print(ts);          Serial.print(",");
    Serial.print(x_g, 4);    Serial.print(",");
    Serial.print(y_g, 4);    Serial.print(",");
    Serial.print(z_g, 4);    Serial.print(",");
    Serial.println(current, 4);

    sample_count++;
  }
}
