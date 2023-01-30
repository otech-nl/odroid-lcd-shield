# CLI to control LCD and LEDs of 16x2 LCD IO shield

Adapted from [ODROID Wiki](https://wiki.odroid.com/accessory/display/16x2_lcd_io_shield/c/start).

Tested on Odroid C4

Run : sudo ./lcd

## Debug in VS Code

Write `<username> ALL=(ALL) NOPASSWD:/usr/bin/gdb` to `/etc/sudoers.d/gdb` to allow you to run gdb as sudo without password (as per https://stackoverflow.com/a/63738783).
