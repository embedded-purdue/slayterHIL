# Architecture

## TENTATIVE Thread Layout
1. Main Thread
    * Spawns other threads
2. Orchestrator <-> SPI Comms
    * Handles SPI communication structure
        * Signaling to master when data is available
        * Interpreting packet format
    * Other threads can send messages to this thread to tell it to send data over spi
    * Can send messages to other threads about what was sent
3. Sensor data interpretation
    * Receives byte array from SPI comms thread
    * Decodes using protobuf
    * Queues actions based on timestamps
4. DUT interface
    * Sends sensor data to DUT
    * Reads DUT outputs. Sends sensor commands to sensor emulation.
5. Sensor emulation
    * Gets sensor commands from DUT interface.
    * Gets timestamps of when sensor data should change from sensor data interpretation.
    * Sends communication instructions to DUT interface.