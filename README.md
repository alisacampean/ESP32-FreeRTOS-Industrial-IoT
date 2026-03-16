# ESP32-FreeRTOS-Industrial-IoT

Proiect Wowki Sistem IoT Industrial de Monitorizare și Control cu FreeRTOS
Campean Alisa-Madalina
Chapter 1. Hardware Architecture Design
1.1. Introduction and System Description The hardware architecture of this IoT (Internet of Things) node was designed to ensure real-time environmental data acquisition, local processing, and the actuation of execution elements based on a predefined set of rules. The system centralizes data acquired from sensors, provides visual feedback via a human-machine interface (HMI), and controls a physical actuator, while also being capable of wireless connectivity.
1.2. Component Selection The following hardware components were selected to implement the system, with the choice being based on performance, power consumption, and industrial compatibility requirements:
Microcontroller (Central Processing Unit): ESP32 DevKit V1. It was selected due to its high-performance 32-bit architecture (Tensilica Xtensa Dual-Core) running at a frequency of 240 MHz. This computational power is essential for efficiently running a real-time operating system (FreeRTOS) and for managing the TCP/IP stack (WiFi) concurrently, thus guaranteeing smooth operation without blocking the continuous data acquisition process.
Acquisition Module (Sensor): DHT22. Used for monitoring temperature and relative humidity. It provides a calibrated digital output, eliminating the need to use the microcontroller's internal analog-to-digital converters (ADCs) and reducing noise on the transmission line.
Visual Interface (HMI): OLED Display SSD1306 (0.96"). A low-power screen that uses the I2C (Inter-Integrated Circuit) communication protocol. Using the I2C bus is essential because it allows the connection of multiple peripherals using only two data pins, thereby optimizing the microcontroller's resources.
Execution Element (Actuator): Micro-Servomotor. It simulates an industrial safety valve. Its control is achieved through a PWM (Pulse Width Modulation) signal, with the microcontroller adjusting the duty cycle to obtain precise angular positioning.
Status Indicators: LEDs. Two light-emitting diodes (green and red), connected in series with current-limiting resistors, used for quick system status diagnosis (Normal Operation vs. Alarm State).
1.3. Wiring Diagram and Pin Allocation (Pinout) The pin allocation (GPIO - General Purpose Input/Output) was done while respecting the architectural constraints of the ESP32 board, separating the communication buses from the PWM signals and standard digital inputs/outputs.

Chapter 2. Software Architecture and System Functionality
2.1. Core Architecture (FreeRTOS) To ensure determinism and parallel execution of processes, the application was developed using the FreeRTOS real-time operating system. The classic "super-loop" approach (void loop()) was eliminated in favor of an architecture based on independent Tasks, each having its own memory stack and priority level. The system is divided into four functional cores:
TaskCitireSenzor: Environmental data acquisition.
TaskDisplayOLED: Updating the human-machine interface (HMI).
TaskControlSistem: Evaluating the control law (highest priority).
TaskConexiuneWiFi (MQTT): Maintaining the network connection and publishing data to the Cloud.
Communication between these concurrent processes is achieved through global variables (shared memory), updated asynchronously.
2.2. Signal Simulation (Software-in-the-Loop) To validate the robustness and dynamic behavior of the controller, the physical acquisition of the DHT22 sensor was temporarily replaced with a Software-in-the-Loop (SIL) testing technique. The microcontroller internally generates a sinusoidal test signal with a frequency of 0.1 Hz, an amplitude of 10°C, and an offset of 30°C. This signal simulates the natural, continuous variation of industrial temperature, allowing the mechanical response of the execution element to be observed in real-time.
2.3. Control Logic (Proportional Controller) The system implements a closed-loop control system with negative feedback, designed to prevent overheating of the monitored enclosure. A reference value (setpoint) of 30°C was set. Every 100 milliseconds, TaskControlSistem calculates the system error (). To ensure a fluid movement of the cooling valve (simulated by the servomotor), a Proportional (P) Controller was implemented. The control law is , where the proportional constant was tuned to the value . To respect the physical limitations of the actuator, the command is passed through a saturation block, limiting the maximum valve opening to 90 degrees. Exceeding the critical threshold also triggers a local visual alarm (red LED).
2.4. Cloud Telemetry and the MQTT Protocol To transform the local assembly into a true Internet of Things (IoT) node, a TCP/IP connectivity module was integrated. The system connects to the local WiFi network, obtaining a dynamic IP address, after which it initiates a connection with a public MQTT Broker (broker.hivemq.com). The choice of the MQTT protocol was based on its lightweight structure, ideal for embedded systems (Edge devices). The ESP32 board acts as a Publisher, packaging the data (temperature and alarm system status) and transmitting it every 3 seconds on the dedicated unique topic (alisa/master/senzori). Any web client or mobile application subscribed to this topic can monitor the machine's status in real-time, from anywhere in the world.
The demonstration of the successful connection to the HiveMQ cloud platform and the reception of data packets can be observed in the figure below:


Chapter 3. Block Diagram and System Operation
To visualize how the system's "brain" (ESP32) interacts with the physical environment and the user, I have designed the following block diagram. This illustrates the informational flow in an automatic closed-loop control system.
Here is how each component translates into our project:
1. Reference (Setpoint - ): This is the desired value, meaning 30°C. It is the critical threshold defined by the engineer. The controller does not "want" the temperature to exceed this value.
2. Error Comparator (Sum): Every 100 milliseconds, the system calculates the difference between the reference and reality: .
Note: In a cooling system, the error is considered positive when the actual temperature is higher than the desired one, indicating the need for an action.
3. Controller: This is the core of the control algorithm, implemented in TaskControlSistem. It receives the error and decides on a command. In our case, it uses the mathematical equation of a Proportional (P) Controller:

where  is the controller's amplification. If the error is 2 degrees, the calculated command is 30 degrees.
4. Saturation Block (Limitation): This block is essential for the system's safety, simulating the physical limitations of the actuator. Although the controller can calculate a command of 150 degrees, the Servomotor (valve) cannot physically open more than 90 degrees. Our code applies these hard limiters:
(In this project, we have not implemented a lower limit, as we consider the valve to be fully closed at a command of 0 degrees.)
5. Actuator / Execution Element (Motor): This is the physical Servomotor, which receives the limited command (from 0 to 90 degrees) and physically rotates the ventilation damper.
6. Process (Controlled System): This is the physical environment whose temperature we are controlling. In the final application, it would be an industrial enclosure or a room. Opening the valve (damper) allows cold air to enter, affecting the process temperature.
7. Sensor / Measurement (DHT22): This is our temperature transducer. It measures the final result of the process and converts it into a digital signal. This step is crucial for achieving the Feedback Loop, sending the information back to the comparator to start the next cycle.
8. Disturbances (): In real industry, these are unpredictable external factors that affect the process, such as opening a door in a room, a hot day, or a malfunction. FreeRTOS and the P controller work together to reject these perturbations and keep the temperature as close to the reference of 30°C as possible. 


