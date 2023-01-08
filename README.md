# Somehow I Manage

If you need some inspiration from *The Office* to get you through the day, this little Arduino project might be it!

![e-ink display](.assets/e-ink_display.jpeg)

## prerequisites

Get yourself an E-Ink / e-Paper display and esp32, I got the following for this project (it's pre-configured for them):

- [Arduino IDE](https://www.arduino.cc/en/software)
- [Waveshare ESP32 Driver Board](https://www.waveshare.com/e-Paper-ESP32-Driver-Board.htm)
- [800×480, 7.5inch E-Ink display](https://www.waveshare.com/7.5inch-e-paper-hat.htm)

Alternatively, if you don't want to buy anything but still read the quotes, [just go here](https://sim.ilayk.com).

## configuration

tl;dr:

1. clone this GitHub repository
2. add it to your Arduino IDE
3. update the Wi-Fi credentials in line `6` and `7`
4. update the `TIME_TO_SLEEP` interval how often you want it to update
5. (update the display configuration if you're using another one)
6. upload it to your ESP32 through the Arduino IDE

If you got the exact hardware as linked above, you just need to open up the `sim-dashbard.ino` sketch in your Arduino IDE and update line `6` and `7` with your Wi-Fi credentials and upload the code to your ESP32.

If you want to **change the interval** the display is being updated, you can change it in **line 15** or look for `TIME_TO_SLEEP`. It's defined in seconds.

### using other displays

If you have a different display, you need to update the `GxEPD2_display_selection_new_style.h` file and **comment out line 59** by adding `//` in front of it, like so:

```
//#define GxEPD2_DRIVER_CLASS GxEPD2_750_T7  // GDEW075T7   800x480, EK79655 (GD7965)
```

…now find the corresponding configuration for your display and uncomment the given line to use the right library for hardware.

You can find more information about supported displays and how they're defined in the code [here](https://github.com/ZinggJM/GxEPD2).

## Licenses

[they're included here](LICENSES.md)
