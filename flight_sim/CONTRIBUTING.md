Flight Simulation Subteam / SlayterHIL

## Overview
The Flight Simulation Subteam is responsible for the control, creation, and distribution of all physics and simulation-related data. We receive test environments from the Test Automation team, run them through our physics engine, and send that data over SPI communication to Testnode firmware. Additionally, we handle UDP communication for external data exchange and provide graphical representations of the simulation for visualization purposes.

This document outlines the structure of the project and provides guidelines for contributing to the repository.

---

## Project Structure
The project is divided into the following key areas:

### 1. **Physics Engine**
   - **Description**: The physics engine is the core of the simulation. It processes input data from the test environments and calculates realistic physical interactions, such as forces, motion, and collisions.
   - **Key Responsibilities**:
     - Implement and optimize physics algorithms.
     - Ensure accuracy and performance of the simulation.
     - Maintain modularity for easy integration with other systems.

### 2. **SPI Communication**
   - **Description**: The SPI (Serial Peripheral Interface) communication module is responsible for transmitting simulation data to the Testnode firmware. This ensures that the hardware receives accurate and timely data for further processing.
   - **Key Responsibilities**:
     - Develop and maintain SPI communication protocols.
     - Ensure data integrity and synchronization.
     - Debug and test communication with hardware.

### 3. **Graphics and Visualization**
   - **Description**: The graphics module provides a visual representation of the simulation. This helps in debugging, validation, and presenting results to stakeholders.
   - **Key Responsibilities**:
     - Develop and maintain the graphical interface.
     - Integrate physics data into the visualization pipeline.
     - Optimize rendering performance.

---

## Contribution guidelines

### SPI Communications

For data communications between the ESP and RPI.

The protobuf schema was manually compiled using the `protoc` compiler (v32.1) more information seen in official [documentation](https://protobuf.dev/installation/).

The subproject itself can be found in `flight_sim/` compiled using CMAKE.

```bash
mkdir build
cd build
cmake ..
```

The spiritual predecessor is established in the `setup.sh` script.
