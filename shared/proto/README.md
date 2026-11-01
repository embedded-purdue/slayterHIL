# Using Protobuf

We use protobuf to standardize our data packets between different devices. The format of packets is defined in this folder, in .proto files. When each application is built, the .proto files get compiled into files of the appropriate language. 

For example, a .proto file that looks like this:

```protobuf
// protobuf - "example.proto"
syntax = "proto3";

message Example {
    int32 val = 1;
}
```

Will generate a file that looks something like this:

```C
// C - "example.pb.h"
typdef struct {
    uint32_t val;
} Example;
```
or
```Cpp
// C++ - "example.pb.h"
class Example {
  private:
    uint32_t val;
  public:
    uint32_t val();
    void set_val();
}
```

In every language, protobuf libraries provide ways to encode and decode these messages so that they have a predictable binary representation. This is why protobuf is useful: we don't have to worry about consistent padding and ordering of struct information, and we only have to define data types once.

# Using Protobuf with C (test_node / zephyr):

We use the [nanopb](https://jpa.kapsi.fi/nanopb/) library, which is integrated with zephyr. This is enabled in west.yml and prj.conf files.


When compiling, ensure protobuf compiler is installed and west workspace is updated (see project README):
```bash
`sudo apt install -y protobuf-compiler` OR `brew install protobuf`
`west update`
`west packages pip --install`
```


To use protobuf definitions in your code:
1. Include associated proto file header
    * `#include "example.pb.h" // Contains definitions for all messages and enums in "example.proto"`
2. Include encode / decode header
    * For encoding: `#include <pb_encode.h>`
    * For decoding: `#include <pb_decode.h>`
3. Encode / decode messages 
    * Great example in `zephyr/samples/modules/nanopb`
    * Also look through nanopb documentation and utilize llm

Tips:
* To get autocomplete/intellisense with new protobuf definitions, you should build the project first to generate the .pb.h and .pb.c files. These will exist in the test_node/build directory.

# Using Protobuf with C++ (flight sim)
1. install protobuf 
    * `sudo apt install -y protobuf-compiler` OR `brew install protobuf`

`sensor_data.proto` supports GPS through `SensorPacket.gps` (field 6).
Populate it with `packet.mutable_gps()` in C++; nanopb exposes `packet.gps`
and `packet.has_gps`. Missing GPS means no GPS update; a present payload
with `valid = false` reports an invalid fix. Coordinates use positive decimal
degree magnitudes with separate `N`/`S` and `E`/`W` direction strings.
Time is UTC `hhmmss.ss`, altitude is above mean sea level with units `M`,
and HDOP is supplied by the sender (use 1 for the emulated fix).

`sensor_data.options` bounds the GPS strings for nanopb so they decode into
fixed C arrays. Rebuild both endpoints after changing the schema. This adds
the wire fields; populating GPS in the simulation and forwarding it to a GPS
emulator are separate application work.
