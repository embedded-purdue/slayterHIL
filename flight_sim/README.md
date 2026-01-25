# Flight Simulator

Flight simulation branch.

## Fall 2025 Contributions

**Data Packaging and SPI Communication**
    - `spi_new_test.cpp`: Creates test data, packages it into a protobuf header, and performs a SPI transfer from the RPI to the ESP32
            - This is the outline for future data transfer, but needs to be updated so the protobuf data is set to the generated output from the physics engine. `
    - `../shared/sensor_data.proto` defines the fields to package sensor data for transfer to the testnode
    - `../../testnode/.../spi_slave_protobuf.c`: Code for the esp32 side of the SPI communication

    

