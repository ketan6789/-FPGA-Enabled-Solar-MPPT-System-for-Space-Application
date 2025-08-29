# FPGA Enabled Solar MPPT System for Space Application

<img width="1562" height="730" alt="image" src="https://github.com/user-attachments/assets/d0665a95-0138-4a63-abec-5084d2b9e6dd" />


## Overview

This project implements a **solar Maximum Power Point Tracking (MPPT) system enabled by FPGA** technology, specifically tailored for **space application** scenarios. It integrates reliable control and measurement strategies to maximize solar energy extraction and system robustness in challenging environments.

## Features

- All system algorithms and controls were simulated using MATLAB Simulink before hardware implementation.
- Partial I2C communication achieved for ADC interfacing on MicroChip PolarFire SoC Discovery Kit.
- Closed-loop boost converter and MPPT implemented on MSP430FR5969.
- Real-time MPPT operation demonstrated on MSP430 hardware.

## Hardware Used

- PolarFire Discovery FPGA Kit
- MSP430FR5956 Controller 
- ADC 
- Boost Converter
- PV Array Simulator

## Software Requirements

- Libero SoC for FPGA design
- TI Code Composer Studio for MSP430 development
- Chroma software platform (if available)

## Installation & Usage

1. Clone this repository:
   ```bash
   https://github.com/ketan6789/-FPGA-Enabled-Solar-MPPT-System-for-Space-Application.git
   ```
     
2. **Install development tools** (see Software Requirements above).

3. Open and Flash the code files onto the MSP430 or FPGA board.
   
## Simulation Models
   Before implementing the MPPT algorithm on the Hardware simulations were done for verifying the working of closed loop boost converter and P&O MPPT algorithm.
   - Closed loop Boost Converter Model
     
     <img width="1347" height="624" alt="image" src="https://github.com/user-attachments/assets/bdb30945-5389-439f-a820-b672949127f4" />

   - P&O MPPT Algorithm Model

     <img width="1320" height="610" alt="image" src="https://github.com/user-attachments/assets/85bbdde4-a564-42ff-9d4a-3f8762bebdeb" />


## Hardware Implementation
   To test the basic funcanality of the FPGA board first a simple PWN generation design was created on the smart canvas of the Libero SoC and flashed onto the FPGA boadrd.
   - Libero SoC Smart Canvas Design

     <img width="1568" height="497" alt="image" src="https://github.com/user-attachments/assets/bfaddf98-4ba2-49e3-84ba-f4c14d8cd0e3" />


   - Hardware Setup

     <img width="1320" height="610" alt="image" src="https://github.com/user-attachments/assets/b4dd26a1-4cd8-4b43-ad49-420b81334707" />

   - PWM Output

     <img width="1320" height="610" alt="image" src="https://github.com/user-attachments/assets/da799d7b-7168-4908-8210-59dd5f8766fb" />

   To implement the MPPT on the FPGA Board attempt was made to interface an ADC (MIKROE ADC 3394) via the I2C protocol. while partial implementation was achived by observing the Slave address on the SDA
   line via the DSO, further interfacing was not achived.

   - Slave address of MIKROE ADC 3394 1001000 + ACK Bit observed on the DSO

     <img width="1320" height="610" alt="image" src="https://github.com/user-attachments/assets/05b1be4f-935b-4c12-9861-124f37ed9dde" />

   Complete Implementation of the Closed Loop Control and MPPT algorith was achived by MSP430FR5969 Microcontroller.

   - Setup for Closed Loop Operation of Boost Converter

     ![20250712_145349](https://github.com/user-attachments/assets/d36ca2a8-041d-4c4a-acec-14a6a4a691e0)

   - Video showcasing the Closed Loop Operation

     https://github.com/user-attachments/assets/00e84b1e-4328-49a9-b181-43219ef5dfac

   - Setup for MPPT Algorithm Implementation.

     ![20250712_145359](https://github.com/user-attachments/assets/cbaa19b3-3795-4851-9511-0d4d2d7304e3)


## License

This project is licensed under the **MIT License**. See the [LICENSE](LICENSE) file for details.

## Contact

For any queries or suggestions, feel free to open an issue.



     


   


   


   

   



     

   

   

