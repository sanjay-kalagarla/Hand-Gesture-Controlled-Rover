#include <Wire.h>
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <MPU6050.h>

#define CE_PIN 4
#define CSN_PIN 5

RF24 radio(CE_PIN, CSN_PIN);
const byte address[6] = "00001";

MPU6050 mpu;
int16_t ax, ay, az;

struct ControlData {
  int motorSpeed;
  char direction;
};

ControlData data;

void setup() {
  Serial.begin(115200);
  Wire.begin();
  mpu.initialize();

  if (!mpu.testConnection()) {
    Serial.println("MPU FAIL");
    while (1);
  }

  if (!radio.begin()) {
    Serial.println("NRF FAIL");
    while (1);
  }

  radio.openWritingPipe(address);
  radio.setPALevel(RF24_PA_HIGH);
  radio.setDataRate(RF24_250KBPS);
  radio.stopListening();

  Serial.println("TX READY");
}

void loop() {
  mpu.getAcceleration(&ax, &ay, &az);

  int speedValue = 0;

  if (abs(ay) > abs(ax)) {
    speedValue = map(abs(ay), 0, 17000, 0, 255);
    if (ay > 4000)       data.direction = 'F';
    else if (ay < -4000) data.direction = 'B';
    else                 data.direction = 'S';
  } else {
    speedValue = map(abs(ax), 0, 17000, 0, 255);
    if (ax > 4000)       data.direction = 'R';
    else if (ax < -4000) data.direction = 'L';
    else                 data.direction = 'S';
  }

  if (data.direction == 'S') {
    data.motorSpeed = 0;
  } else {
    data.motorSpeed = constrain(speedValue, 150, 255);
  }

  radio.write(&data, sizeof(data));

  Serial.print("Dir: ");
  Serial.print(data.direction);
  Serial.print(" Speed: ");
  Serial.println(data.motorSpeed);

  delay(20);
}
