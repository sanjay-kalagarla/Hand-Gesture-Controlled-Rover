# Hand Gesture Controlled Rover

I built a wireless rover that responds to hand tilts in real-time — no buttons, no joystick, no remote. You wear a glove, tilt your hand in any direction, and the rover moves. Hold it flat, it stops. The whole thing — hardware selection, PCB assembly, wiring, and firmware for both ends — I put together myself.

---

## What I Built

Two ESP32 nodes talking to each other over 2.4 GHz RF:

- **Glove unit (Transmitter)** — MPU6050 IMU reads my hand tilt → ESP32 encodes it as a direction + speed packet → NRF24L01 broadcasts it wirelessly
- **Rover unit (Receiver)** — NRF24L01 picks up the packet → ESP32 decodes it → L298N drives the motors

```
[MPU6050] → [ESP32 TX]──── 2.4GHz RF ────► [ESP32 RX] → [L298N] → [4× DC Motors]
              NRF24L01                         NRF24L01
```

---

## Key Features

- **< 10ms latency** over a **1km range** via NRF24L01 at RF24_250KBPS data rate
- **Proportional speed control** — the further I tilt, the faster it moves (mapped from raw IMU values to PWM 150–255)
- **Differential turning** — on left/right gestures, one side slows to half speed while the other runs full, giving smooth arcs instead of sharp pivots
- **Dominant-axis gesture detection** — compares `|ax|` vs `|ay|` and picks whichever tilt is stronger, preventing diagonal drift from triggering mixed commands
- **Dead zone at center** — tilt under ±4000 raw units maps to STOP, so the rover doesn't twitch from minor hand wobble
- **Modular PCB** — I used socketed headers throughout so any module can be swapped without touching solder

---

## How the Code Works

### Transmitter — reading my hand and packing data

The MPU6050 gives raw accelerometer values on X and Y axes. I compare which axis has the stronger tilt and treat that as the intended direction:

```cpp
if (abs(ay) > abs(ax)) {
    // forward/backward intent
    if (ay > 4000)       data.direction = 'F';
    else if (ay < -4000) data.direction = 'B';
    else                 data.direction = 'S';
} else {
    // left/right intent
    if (ax > 4000)       data.direction = 'R';
    else if (ax < -4000) data.direction = 'L';
    else                 data.direction = 'S';
}
```

Speed maps to how far the tilt goes, clamped so the rover always moves with enough force:

```cpp
speedValue = map(abs(ay), 0, 17000, 0, 255);
data.motorSpeed = constrain(speedValue, 150, 255);
```

This `ControlData` struct (direction + motorSpeed) gets sent over RF every 20ms.

---

### Receiver — turning packets into movement

The rover picks up the packet and runs it through `motorControl()`. I set the L298N direction pins and write PWM via ESP32's LEDC peripheral:

| Gesture | Left Motor | Right Motor | IN1 | IN2 | IN3 | IN4 |
|---|---|---|---|---|---|---|
| Forward (F) | full speed | full speed | HIGH | LOW | HIGH | LOW |
| Backward (B) | full speed | full speed | LOW | HIGH | LOW | HIGH |
| Left (L) | half speed | full speed | LOW | HIGH | HIGH | LOW |
| Right (R) | full speed | half speed | HIGH | LOW | LOW | HIGH |
| Stop (S) | 0 | 0 | LOW | LOW | LOW | LOW |

For turns, I slow one side rather than reversing it — gives a natural curved arc instead of spinning in place, which feels much better under gesture control.

```cpp
ledcWrite(PWM_CH_A, leftSpeed);
ledcWrite(PWM_CH_B, rightSpeed);
```

PWM runs at 1kHz, 8-bit resolution (0–255).

---

## Hardware I Used

| Component | Spec |
|---|---|
| ESP32 DevkitC-32UE | MCU for both nodes |
| MPU6050 | 6-DOF IMU on the glove |
| NRF24L01+ with adapter | 2.4 GHz RF transceiver |
| L298N Motor Driver | Dual H-bridge |
| DC Gear Motors | 12V, 850 RPM — 4× |
| Li-ion Pack | 4-cell (rover) + 2-cell (glove) |
| DC-DC Buck Converter | Regulated 5V / 3.3V rails |
| 47µF Capacitors | Motor rail transient suppression |

---

## Arduino Setup

### Board Package
Add this URL in Arduino IDE → Preferences → Additional Boards Manager URLs, then install via Boards Manager:
- **esp32 by Espressif Systems** — `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`
- Select board: `ESP32 Dev Module`

### Libraries
Install via Arduino IDE → Library Manager:

| Library | What it does |
|---|---|
| `RF24` by TMRh20 | NRF24L01 driver — SPI comms, PA level, data rate, pipe addressing |
| `MPU6050` by Electronic Cats / Jeff Rowberg | IMU driver — sensor init and raw accelerometer reads over I2C |
| `Wire` | Built-in — I2C for MPU6050 |
| `SPI` | Built-in — SPI for NRF24L01 |

> ESP32 PWM (`ledcSetup` / `ledcAttachPin` / `ledcWrite`) is part of the Espressif Arduino core — no extra library needed.

---

## License

```
MIT License

Copyright (c) 2025 Sanjay Kalagarla
