# ArtNetDMXtoSD
WiFi Access Point for receiving ArtNet Frames Via Wifi and Saving them to SD card - with Web Server

- Currently under construction - different script redoing arhitecture and library for SD Card.

<em>This will need to be adapted to suit your board and your ArtNet Setup</em>

Hardware it works on:

ESP32: TTGO T1 w/ SD card
ESP32: WeMos D1 w/o SD card

1) Setup Arduino (or VSCode) to build for ESP32: https://randomnerdtutorials.com/installing-the-esp32-board-in-arduino-ide-windows-instructions/

2) Attach your LED strip (WS2812, WS2811, APA102 etc.) to a Pin (I used 27)- try to avoid using pins used for the SD Card (in my build these were 2,13,14,15).

3) Add the FastLED library to you project from: https://github.com/FastLED/FastLED

4) Add Libraries such int the header from relevant Repos.

5) Make sure to configure lines:
const int pixelMultiplier = 3; //This is how many LEDS should do the same thing (act as one LED)

const int numLeds = 36*24; // Change if your setup has more or less LED's (this was a string of 864 LEDS using 2 ArtNet Universes) - remember to do this in adrix or Resolume.

const int universes = 2;

const int numberOfChannels = ((numLeds * 3)/pixelMultiplier)/universes; // Total number of DMX channels you want to receive (1 led = 3 channels)

#define DATA_PIN 27 (this is the Pin used to drive the data for the LEDS)

6) Plug in the ESP32 and select port and copile and upload.
