# Embedded 3D Spatial Mapping System

An embedded indoor spatial-mapping system developed using an ARM Cortex-M4F
microcontroller, VL53L1X Time-of-Flight sensor, stepper motor, and MATLAB.

## Overview

The system performs 360-degree spatial scanning using a Time-of-Flight sensor
mounted on a stepper motor. Distance data is collected by the microcontroller,
transmitted to a PC through UART, and processed in MATLAB to reconstruct a
3D representation of the scanned environment.

## Features

- 360° spatial scanning
- 32 distance measurements per scan
- 11.25° angular resolution
- VL53L1X Time-of-Flight distance sensing
- I²C communication between sensor and microcontroller
- 115200-baud UART communication with PC
- Stepper motor positioning
- Push-button control and LED status indicators
- MATLAB-based 3D visualization

## Technologies

- C
- MATLAB
- ARM Cortex-M4F
- MSP-EXP432E401Y
- VL53L1X
- I²C
- UART
- Keil µVision

## Project Architecture

1. The VL53L1X sensor measures distance.
2. The ARM Cortex-M4F microcontroller collects and stores measurements.
3. A stepper motor rotates the sensor through 360°.
4. Measurement data is transmitted to the PC using UART.
5. MATLAB converts distance and angle data into Cartesian coordinates.
6. Multiple scan planes are combined to generate a 3D visualization.

## Results

The system successfully reconstructed the geometry of an indoor environment
using multiple 360° scan planes.

## Author

Hongfan Li  
Electrical Engineering  
McMaster University
