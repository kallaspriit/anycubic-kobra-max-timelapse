# Anycubic Kobra Max-timelapse

Smartphone timelapse trigger for a Anycubic Kobra Max 3D printer.

It works by triggering a button at the end of the X axis after every layer which sends take picture key press (volume down button) to your smartphone via bluetooth.

The smartphone will take a picture of the print after every layer that can be combined in an external video editing program (such as built-in Photos app in Windows) to a timelapse video.

## Firmware

Communication to the smartphone is done using Bluetooth Low Energy (BLE) microcontroller that mimics a wireless HID keyboard. You just bind it with your phone and whenever the trigger button is pressed by the script, the phone takes a picture for every layer that can be combined into a video.

Using a camera app where you can fix focus and exposure is recommended (such as Open Camera on Android).

The firmware is made for Adafruit Feather nRF52840 Express/Sense but could likely be easily adapted to any MCU with BLE running MBED stack.

It's easiest compiled with [Platform IO](https://platformio.org) but normal Arduino IDE should work fine too.

![Adafruit Feather nRF52840 Express](https://cdn-shop.adafruit.com/970x728/4062-02.jpg)

## Hardware

The trigger uses a basic momentary limit switch in a custom 3D-printable mount. This can be printed with either filament or resin printer.

You need a cable between the switch (common and normally open terminals) and the microcontroller (ground and digital input, firmware is using `A0` for the Adafruit Sense board that is next to ground). The pins can be edited in firmware.

## Setup

Compile and upload the firmware, power the MCU with USB and add it as a bluetooth device on your smarphone.

Open a camera app that takes a photo when volume buttons are pressed (the keycode sent can also be modified in the firmware) and press the momentary switch, your phone should now take a picture.

Somehow fix the phone so that it faces the thing you're printing and let it do it's thing. You should end up with a bunch of images, one for each layer of the print. Combine them to a video in Windows Photos app or similar.

## Cura script for triggering after every layer

Below is the script used in [Ultimaker Cura](https://ultimaker.com/software/ultimaker-cura) slicing program that moves the X axis to the end of it's travel at the end of every layer, pressing the trigger button.

To use this do the following:
- Open Cura
- Open `Extensions > Post Processing > Modify G-Code`
- Click `Add a script`
- Choose `Search and Replace` script type
- Enter `;LAYER:` as the Search field.
- Enter the single line script where all linefeeds are replaced with `\n`

Note that you can't copy the multi-line replace script in the text field (at least in Cura v5.0.0). To use it, modify the script as needed and then create a version of it where all linefeeds (`\r\n` in Windows) are replaced with `\n`. Such a modified single line version of this script is below.

```gcode
;LAYER:
;Take timelapse photo script
G91 ;Use relative positioning mode
G1 F2400 E-6 ;Retract filament
G0 F6000 Z2 ;Move Z up 2mm
G90 ;Use absolute positioning mode
G0 F12000 X400 Y200 ;Quick move to center of right edge
G4 P1500 ;Pause for 1.5 seconds
G0 F12000 X200 Y200 ;Quick move to center of the build plate
G91 ;Use relative positioning mode
G1 F6000 Z-2 ;Move Z back down
G90 ;Go back to absolute position mode
```

Single line version:
```gcode
;LAYER:\n;Take timelapse photo script\nG91 ;Use relative positioning mode\nG1 F2400 E-6 ;Retract filament\nG0 F6000 Z2 ;Move Z up 2mm\nG90 ;Use absolute positioning mode\nG0 F12000 X400 Y200 ;Quick move to center of right edge\nG4 P1000 ;Pause for 1.0 seconds\nG0 F12000 X200 Y200 ;Quick move to center of the build plate\nG91 ;Use relative positioning mode\nG1 F6000 Z-2 ;Move Z back down\nG90 ;Go back to absolute position mode
```
