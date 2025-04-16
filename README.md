# Raspberry Pi Pico AlphaESS api
Project using an rp2040/rp2340 and w5500 ethernet module to read live solar panel performance data from open.alphaess.com api and displaying it.

![Control Box](https://github.com/Jannis-L/AlphaESS/blob/main/images/IMG_Control.jpg "Control Box")
![Screen Box](https://github.com/Jannis-L/AlphaESS/blob/main/images/IMG_Screen.jpg "Screen Box")

The display is mounted in a light switch box.
While this version does not yet implement the display, it demonstrates how to access the alphaess or similar apis from within the raspberry pi c-sdk and the w5500 Ethernet Chip library supplied by its vendor.

The extra connections on the controller are currently utilized to switch off a circulation pump to save on energy at night and to override temperature readings of a non ethernet enabled heating system to reduce its heat output. These features are also not implemented in this repository.

httpClient.c & httpClient.h based on: \
https://github.com/WIZnet-ioLibrary/W5x00-HTTPClient \
ioLibrary submodule: \
https://github.com/Wiznet/ioLibrary_Driver \
Implementations of ioLibrary for Pico C sdk (Folder "port") \
https://github.com/WIZnet-ioNIC/WIZnet-PICO-C

A document "secrets.h" has to be put in the src folder containing:
#pragma once \
#define APP_ID "alpha#####" \
#define APP_SECRET "#####" \
#define APP_SN "ALA#####"