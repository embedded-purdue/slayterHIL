# Test Node <-> Orchestrator Interface Design Considerations

## Data Format
We will be using [protobuf](https://protobuf.dev/) to format our packets. This allows us to make struct-like definitions that we can use across multiple languages and devices, and are guaranteed to have the same byte representation. This is a much more efficient serialization method than something like json that you may use in a less resource-constrained environment.

Since it seems like we will be using C++ on the orchestrator and C on the test node, another option could be shared headers with c-style struct definitions. However, there may be issues with inconsistent padding and endianness between different compilers. Protobuf provides explicitly-defined padding and byte order. It also lets us use our definitions in python, etc if necessary.

On the test node, we will be using [nanopb](https://docs.zephyrproject.org/latest/services/serialization/nanopb.html), which is a c implementation of protobuf designed to be very lightweight for embedded systems.

This is what a protobuf definition of our packets may end up loooking like:

    enum Sensor {
        SENSOR_UNKNOWN = 0;
        SENSOR_IMU = 1;
    }

    message SensorData {
        Sensor sensor;
        int32 timestamp;
        int32 data;
    }

    enum Command {
        COMMAND_UNKNOWN = 0;
        COMMAND_RESET = 1;
    }

    message CommandData {
        Command command;
        int32 param;
    }

## Physical Layer
There will be a lot of data frequently passed between the orchestrator and test node, so we need to use a protocol that can handle high speeds. We have a few good options:

* SPI
    * Pros: 
        * Very high bitrate (10s of MHz)
        * Low test node overhead
    * Cons: 
        * Only works with single board computer (Pi) as orchestrator (can't upgrade to server) 
* USB
    * Pros: 
        * Reasonably high bitrate (~10Mhz)
        * Compatible with any orchestrator
    * Cons: 
        * Overhead on test node
* UART
    * Pros: 
        * Very low overhead on test node
        * Compatible with any orchestrator (uart-usb bridge)
    * Cons:
        * Slower bitrate (~1-5Mhz) (likely limited by uart-usb bridge)


**Since we will mostly likely be using a single board computer for the orchestrator, we will go with SPI.**

### SPI Comms Design
Using SPI does have some complexity. One device is the master, and the other is the slave. They both send data at the same time. The slave is unable to start transactions. This means the slave will have to somehow signal that it has data available, which will trigger the master to start a transaction. In this setup, the master will also have to know exactly how many bytes the slave wants to transfer. We will probably need a small wrapper format around the protobuf payload. This is WIP.