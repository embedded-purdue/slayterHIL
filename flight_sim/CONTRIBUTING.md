# Flight Simulation Subteam / SlayterHIL

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
   - **Relevant Files**: `physics/`

### 2. **SPI Communication**
   - **Description**: The SPI (Serial Peripheral Interface) communication module is responsible for transmitting simulation data to the Testnode firmware. This ensures that the hardware receives accurate and timely data for further processing.
   - **Key Responsibilities**:
     - Develop and maintain SPI communication protocols.
     - Ensure data integrity and synchronization.
     - Debug and test communication with hardware.
   - **Relevant Files**: `spi/`, `communication/spi/`

### 3. **UDP Communication**
   - **Description**: The UDP module handles external data exchange, such as receiving input from external systems or sending simulation results to other teams or tools.
   - **Key Responsibilities**:
     - Implement UDP sockets for data transmission.
     - Ensure low-latency communication.
     - Handle error detection and recovery mechanisms.
   - **Relevant Files**: `udp/`, `communication/udp/`

### 4. **Graphics and Visualization**
   - **Description**: The graphics module provides a visual representation of the simulation. This helps in debugging, validation, and presenting results to stakeholders.
   - **Key Responsibilities**:
     - Develop and maintain the graphical interface.
     - Integrate physics data into the visualization pipeline.
     - Optimize rendering performance.
   - **Relevant Files**: `graphics/`

---

## Contribution Guidelines

### General Guidelines
- Ensure all code changes are accompanied by relevant unit tests.
- Write clear and concise commit messages.
- Document all new features and modules in the appropriate markdown files.

### Workflow
1. **Fork the Repository**: Create your own fork of the repository.
2. **Create a Branch**: Use a descriptive name for your branch (e.g., `feature/udp-optimization` or `bugfix/spi-sync`).
3. **Make Changes**: Implement your changes and ensure they align with the project structure.
4. **Test Your Changes**: Run all relevant tests to ensure your changes do not break existing functionality.
5. **Submit a Pull Request**: Provide a detailed description of your changes and link to any relevant issues.

### Code Style
- Use meaningful variable and function names.
- Keep functions and classes small and focused.

### Testing
- Write unit tests for all new features.
- Ensure existing tests pass before submitting a pull request.
- Use the `tests/` directory to organize test cases.

---

## Getting Started
To get started with the project, follow these steps:
1. Clone the repository:
   ```bash
   git clone https://github.com/your-org/slayterHIL.git

2. Install dependencies
cd slayterHIL
