# Vivacity UDP Occupancy Data Example

This example application listens for UDP packets on port 8000, and attempts to decode them as protobuf messages of `vivacity.DetectorTrackerFrame`s

It prints the sensor ID and timestamp of each message it receives, and prints a table of zonal occupancies by class whenever these values change.

It is intended to be used as a starting point for developing a C application for consuming and processing Vivacity `DetectorTrackerFrame`s eg on traffic signal controllers or UTC instations. 

A dummy message sender application is provided for convenience.

## Dependencies

* `build-essential` (or equivalent for your distro)
* [`protoc` (3.11.4)](https://github.com/protocolbuffers/protobuf/releases/tag/v3.11.4)
* `cmake` (3.15) (will probably work with lower if `CMakeLists.txt` is edited)
* `python3`

## Submodules
Initialise and update the protobuffet submodule:
```bash
git submodule update --init --recursive
```


## Getting started

First, create a Python virtual environment to work in and install Python protobuf in. This is needed by nanopb to be able to generate C source files. 

```bash
python3 -m virtualenv venv
source venv/bin/activate
pip3 install -r requirements.txt
```

Then build the project:

```bash
mkdir build
cd build
cmake ..
make -j$(nproc)
```


Then run

```bash
./vivacity_occupancy_example
```

## Generating C files manually without CMake
```bash
mkdir vivacity
cp -r proto-buffet/* vivacity 
./nanopb/generator/protoc --nanopb_out=. --plugin=protoc-gen-nanopb=./nanopb/generator/protoc-gen-nanopb -I. ./vivacity/**/*.proto
```

If generating manually, you'll need to set up your build system with `vivacity` in the include path, and to compile and link all of the generated `*.pb.c` files
  
## Dummy Message Sender
Run this program to send dummy Detector Tracker Frame messages. 

The program takes 3 arguments:
- `-r` rate: the rate at which to send messages. Measured in microsends between each message.
- `-a` ip address: the IP address to send the UDP frames to
- `-p` port: the port to send the UDP frames to.

For example:
`./dummy_message_sender -a 127.0.0.1 -p 8000 -r 10000` 
will send the messages to the example decoder application at an interval of 10ms. This is equivalent to 10 sensors sending messages at 10Hz each
