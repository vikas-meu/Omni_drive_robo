# ESP32 Omni-Drive Robot Controller

## Overview

Firmware for ESP32 to control a three-wheeled omni-directional robot. Uses inverse kinematics for holonomic movement. Wireless control via WebSocket in AP mode.

![Robot Setup](annotated_robot.jpg)  
*(Image of the assembled robot on a tiled floor.)*

![Annotated Robot](robot_setup.jpg)  
*(Annotated image showing directions and labels like "PART 3: OMNI DRIVE ROBOT", "ESP32 + WIFI", "FINALLY WORKING", "MOVE TO ANY DIRECTION".)*

## Features

- Holonomic motion: Translate and rotate simultaneously.
- Control: Web interface with joystick (touch/mouse) and keyboard (arrows/numpad, R/L for rotation).
- Ramping for smooth acceleration.
- Per-wheel calibration (Kp, Kb, Ks).
- Speed scaling (MAX_SPEED_SCALE).
- Failsafe stop on disconnect.

## Hardware

- ESP32 board.
- 3 DC motors with drivers (e.g., L298N).
- 3 omni wheels in 120° triangle.
- Pins: W1 (21,19,32), W2 (27,14,13), W3 (22,23,26).
- Power: Separate for motors and ESP32.

## Software

- Arduino IDE with ESP32 support.
- Libraries: WiFi, WebServer, WebSocketsServer.

## Installation

1. Save code as `.ino`.
2. Adjust pins, calibration, ramp rate, max speed in code.
3. Upload to ESP32.

## Setup

- Power on ESP32.
- Connect to AP: SSID="RobotControl", PW="12345678".
- Browse to 192.168.4.1.

## Usage

- Joystick: Drag for direction/speed.
- Keyboard: Up/8 forward, Down/2 back, Left/4 strafe left, Right/6 right; 9/3/7/1 diagonals; R right rotate, L left.
- Monitor via Serial (115200).

## Kinematics

Inverse: Vx, Vy, Vr to wheel speeds.
- W1: Vy - Vr
- W2: -0.5*Vy - 0.866*Vx - Vr
- W3: -0.5*Vy + 0.866*Vx - Vr
Normalized to [-100,100]%, scaled, calibrated, ramped to PWM.

## Troubleshooting

- No connect: Check AP network/IP.
- Jerky: Tune ramp/calibration.
- Uneven: Adjust Kp/Kb/Ks.
- Overheat: Lower max speed.

## License

MIT.
