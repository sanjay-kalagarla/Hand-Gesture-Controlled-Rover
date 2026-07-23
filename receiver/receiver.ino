#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

#define CE_PIN 4
#define CSN_PIN 5

RF24 radio(CE_PIN, CSN_PIN);
const byte address[6] = "00001";

// Motor pins
#define ENA 14
#define IN1 26
#define IN2 27
#define ENB 25
#define IN3 33
#define IN4 32

#define PWM_CH_A 0
#define PWM_CH_B 1

struct ControlData {
  int motorSpeed;
  char direction;
};

ControlData data;

void setup() {
  Serial.begin(115200);

  radio.begin();
  radio.openReadingPipe(0, address);
  radio.setPALevel(RF24_PA_HIGH);
  radio.setDataRate(RF24_250KBPS);
  radio.startListening();

  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  // PWM setup
  ledcSetup(PWM_CH_A, 1000, 8);
  ledcAttachPin(ENA, PWM_CH_A);
  ledcSetup(PWM_CH_B, 1000, 8);
  ledcAttachPin(ENB, PWM_CH_B);

  Serial.println("RX READY");
}

void loop() {
  if (radio.available()) {
    radio.read(&data, sizeof(data));

    Serial.print("Dir: ");
    Serial.print(data.direction);
    Serial.print(" Speed: ");
    Serial.println(data.motorSpeed);

    motorControl(data.direction, data.motorSpeed);
  }
}

void motorControl(char dir, int speed) {
  int leftSpeed = speed;
  int rightSpeed = speed;

  switch (dir) {
    case 'F':
      digitalWrite(IN1, HIGH);
      digitalWrite(IN2, LOW);
      digitalWrite(IN3, HIGH);
      digitalWrite(IN4, LOW);
      break;

    case 'B':
      digitalWrite(IN1, LOW);
      digitalWrite(IN2, HIGH);
      digitalWrite(IN3, LOW);
      digitalWrite(IN4, HIGH);
      break;

    case 'L':
      leftSpeed = speed / 2;
      rightSpeed = speed;
      digitalWrite(IN1, LOW);
      digitalWrite(IN2, HIGH);
      digitalWrite(IN3, HIGH);
      digitalWrite(IN4, LOW);
      break;

    case 'R':
      leftSpeed = speed;
      rightSpeed = speed / 2;
      digitalWrite(IN1, HIGH);
      digitalWrite(IN2, LOW);
      digitalWrite(IN3, LOW);
      digitalWrite(IN4, HIGH);
      break;

    case 'S':
      digitalWrite(IN1, LOW);
      digitalWrite(IN2, LOW);
      digitalWrite(IN3, LOW);
      digitalWrite(IN4, LOW);
      leftSpeed = 0;
      rightSpeed = 0;
      break;
  }

  ledcWrite(PWM_CH_A, leftSpeed);
  ledcWrite(PWM_CH_B, rightSpeed);
}
