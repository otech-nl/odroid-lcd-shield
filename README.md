# CLI for 16x2 LCD IO shield

Adapted from [ODROID Wiki](https://wiki.odroid.com/accessory/display/16x2_lcd_io_shield/c/start).

You need wiringPi for this (see tip bleow). Tested on Odroid C4.

## Usage

Run : `sudo ./lcd` (must be elevated)

Positional arguments are used as texts to display on the LCD.

Use `-l 0100000` to enable or disable LEDs.

Example `sudo ./lcd test1 test2 -l 01` show "test1" on the first line, "test2" on the second line, and enables the second LED.

## Development

### Wiring PI

I made a small change to wiringPi: I removed `lcdClear` from `lcdInit` in `devLib/lcd.c:488` so the LCD is not cleared when it is initialized.

### Debug in VS Code

Write `<username> ALL=(ALL) NOPASSWD:/usr/bin/gdb` to `/etc/sudoers.d/gdb` to allow you to run gdb as sudo without password (as per https://stackoverflow.com/a/63738783).

## Tip

- To compile wiringPi as per [ODROID Wiki](https://wiki.odroid.com/accessory/display/16x2_lcd_io_shield/c/start), make sure to select the right branch, or you won't have a `build` script.

- create file `/etc/network/if-up.d/show_ip`, make it executable, and add the code below (with full path):

```
#!/bin/sh

cd <path> && ./show_ip.sh
```
