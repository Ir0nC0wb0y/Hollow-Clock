# Hollow Clock
 This project is derived from [shiura's Hollow Clock 3](https://www.thingiverse.com/thing:5142739).
 I also used the [remix for the Wemos D1 Mini](https://www.thingiverse.com/thing:5160250)

## Modifications
So far, I have:
  - migrated to PlatformIO (I'm not a fan of the Arduino IDE)
  - implemented standard libraries (such as AccelStepper and NTP)
  - added a buttons to push forward/reverse

## Intended Modifications
Eventually I'll add some kind of feedback to "close the loop" for more accurate time. Most likely this will be a magnet/hall effect combination in the minute rotor.

