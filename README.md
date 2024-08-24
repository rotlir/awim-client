# AWiM

AWiM (Android Wireless Mic) is a tool that allows you to stream microphone from your Android device to a PC running a GNU/Linux distribution with Pipewire as a sound server over Wi-Fi.

This repository contains source code of the client part (i.e. running on your PC). For the source code of the server part visit https://github.com/rotlir/awim-server

## Dependencies

Currently, the only dependence for this program is libpipewire-0.3

## Building

Simply run

```shell
make
```

Make sure you have pkg-config installed prior to building.

## Usage

You can either run

```shell
awim
```

without any arguments (the program will ask for IP address and port of the server) or specify IP address and port as arguments:

```shell
awim --ip 192.168.88.24 --port 1242
```

The program will create a virtual microphone "AWiM-source".

## Feedback

If you found some bug or have an idea for improvement, feel free to open an issue or contact me in Matrix: @rotlir:matrix.org


