![ubirch logo](https://ubirch.de/wp-content/uploads/2018/10/cropped-uBirch_Logo.png)

# ESP32 ubirch console

This is an implementation of the ESP32 console with some default commands
added for ubirch handling of networking and key management.

It is an adaptation from [Console example](https://github.com/espressif/esp-idf/tree/master/examples/system/console)

## Prerequisits

The following components are required for the functionality, see also
[CMakeLists.txt](https://github.com/ubirch/ubirch-esp32-console/blob/master/CMakeLists.txt)

- [ubirch-esp32-networking](https://github.com/ubirch/ubirch-esp32-networking)
- [ubirch-esp32-storage](https://github.com/ubirch/ubirch-esp32-storage)
- [nvs_flash](https://github.com/espressif/esp-idf/tree/master/components/nvs_flash)
- [console](https://github.com/espressif/esp-idf/tree/master/components/console)


## Command Overview
- **help** -> Print the list of registered commands
- **free** -> Get the total size of heap memory available
- **restart** -> Restart the program
- **deep_sleep**  `[-t <t>] [--io=<n>] [--io_level=<0|1>]` ->
Enter deep sleep mode. Two wakeup modes are supported: timer and GPIO.
If no wakeup option is specified, will sleep indefinitely.
    - `-t, --time=<t>`  Wake up time, ms (optional)
    - `--io=<n>`  If specified, wakeup using GPIO with given number (optional)
    - `--io_level=<0|1>`  GPIO level to trigger wakeup (optional)

- **join**  `[--timeout=<t>] <ssid> [<pass>]` ->
  Join WiFi AP as a station
  - `--timeout=<t>`  Connection timeout, ms (optional)
  - `<ssid>`  SSID of AP
  - `<pass>`  PSK of AP (optional)

- **update_backendkey** `[<backendkey>]` ->
  Updates the backend public key, which is used to verify responses from the backend
  - `[<backendkey>]` backend public key in base64 string format (optional, if none is given, the key is reset to the
     default key given as Kconfig configuration value)
- **update_password** [<password>] ->
  Updates the password of currently loaded ID

- **status** -> Get the current status of the system, including details of the currently loaded ID

- **exit** -> Exit the console and resume with program

## Example

In this code example snippet, the console is started, when the combination
`Ctrl+C` or `Ctrl+U` is pressed, while the esp32 is connected to a serial console.

```c
init_console();

char c = (char) fgetc(stdin);
if ((c == 0x03) || (c == 0x15)) {  //0x03 = Ctrl + C      0x15 = Ctrl + U
    // If Ctrl + C was pressed, enter the console and suspend the other tasks until console exits.
    run_console();
}

```

## Tools for Serial Connection

- `$IDF_PATH/tools/idf.py monitor`
- `miniterm.py` is a tool from the [pyserial](https://github.com/pyserial/pyserial) package.
    -  to use miniterm.py, open your prefered terminal and enter: `miniterm.py /dev/ttyUSB0 115200 --raw`
        - where `/dev/ttyUSB0` is the COM port and `115200` is the baudrate. Both might have to be adjusted.
         The `--raw` will give you the raw output, without the special control characters.
