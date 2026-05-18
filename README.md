# ESP32-FreeRTOS-Industrial-IoT

**[Click here to run the Live Simulation on Wokwi](https://wokwi.com/projects/458425376017219585)**

## Project Overview
This project implements a real-time IoT node designed for industrial environmental data acquisition, local processing, and physical actuation based on a closed-loop control system. Built on an **ESP32** microcontroller running **FreeRTOS**, the system completely eliminates the classic `void loop()` approach in favor of concurrent, prioritized tasks.

The system monitors temperature, provides visual HMI feedback, computes a Proportional (P) control law to adjust a safety valve (Servomotor), and publishes telemetry data to the Cloud via the MQTT protocol.

## Hardware Stack
* **Microcontroller:** ESP32 DevKit V1 (Tensilica Xtensa Dual-Core @ 240 MHz)
* **Sensor:** DHT22 (Temperature & Humidity)
* **HMI Display:** OLED SSD1306 0.96" (I2C Protocol)
* **Actuator:** Micro-Servomotor (PWM Control)
* **Indicators:** Status LEDs (Green = Normal Operation, Red = Alarm)

## Software Architecture & FreeRTOS
To ensure determinism and parallel execution, the application is divided into 5 independent FreeRTOS Tasks:
1. `TaskCitireSenzor` (Priority 1): Acquires environmental data. Currently uses a Software-in-the-Loop (SIL) approach, generating a 0.1 Hz sinusoidal test signal to simulate continuous industrial temperature variation.
2. `TaskDisplayOLED` (Priority 1): Updates the local UI with real-time telemetry and system status.
3. `TaskControlSistem` (Priority 2 - High): Evaluates the control law at a 10Hz sampling rate (100ms).
4. `TaskConexiuneWiFi` (Priority 1): Manages network connectivity.
5. `TaskMQTT` (Priority 1): Handles the TCP/IP connection to a public HiveMQ Broker and publishes data packets every 3 seconds.

## Control Logic (P-Controller)
The core of the system is a closed-loop negative feedback controller designed to prevent enclosure overheating (Reference Setpoint = 30°C). 

The `TaskControlSistem` computes the error: $e(t) = y(t) - r(t)$
And applies the Proportional Control Law: $u(t) = K_p \cdot e(t)$

With the proportional gain set to $K_p = 15.0$, the calculated command is passed through a saturation block to respect the actuator's physical limitations (maximum valve opening of 90 degrees).

## IoT & Cloud Telemetry
The ESP32 acts as an Edge device (Publisher) utilizing the lightweight **MQTT** protocol. Telemetry data (Temperature and Alarm Status) is securely packaged and transmitted to `broker.hivemq.com` on a unique topic (`alisa/master/senzori`), allowing real-time global monitoring via any subscribed web or mobile client.

---
*Note: The source code (`sketch.ino`) contains inline comments in Romanian as part of the initial development and learning process.*
