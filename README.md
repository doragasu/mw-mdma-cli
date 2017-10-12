# mw-mdma-cli
MegaWiFi MegaDrive Memory Administration(MDMA) command line interface. This program allows to read and write ROMs from/to MegaWiFi cartridges, using a MegaWiFi programmer. It also allows to upload firmware blobs to the in-cart ESP8266 WiFi module. Starting with version 0.4, the program can also be built with a nice Qt5 GUI. The GUI does not currently support the bootloader and WiFI related options, but other than that is completely usable.

# Building
For the basic CLI version, you just need to install `libusb-1.0` development packages and the standard development tools. then cd to the path with the sources and call
```
$ make -f Makefile-no-qt
```
If everything goes OK, you should have the `mdma` binary sitting in the same directory.

If you want to be able to launch the Qt GUI (in addition to being able to use the program in CLI mode), you will have to install the `qt5-base` development packages (I think it is called `qt5-default` in Ubuntu and derivatives). Then run:
```
$ qt5-qmake
$ make
```
If the build process completes successfully, you should be able to run `mdma -Q` to start the GUI.

# Usage
Once you have plugged a MegaWiFi cartridge into a MegaWiFi Programmer, you can use mdma. The command line application invocation must be as follows:
```
$ mdma [option1 [option1_arg]] […] [optionN [optionN_arg]]
```
The options (option1 ~ optionN) can be any combination of the ones listed below. Options must support short and long formats. Depending on the used option, and option argument (option_arg) must be supplied. Several options can be used on the same command, as long as the combination makes sense (e.g. it does make sense using the flash and verify options together, but using the help option with the flash option doesn't make too much sense).

| Option | Argument type | Description |
|---|---|---|
| --qt-gui | -Q | N/A | Use the Qt GUI (if supported). |
| --flash, -f | R - File | Programs the contents of a file to the cartridge flash chip. |
| --read, -r | R - File | Read the flash chip, storing contents on a file. |
| --erase, -e | N/A | Erase entire flash chip. |
| --sect-erase, -s | R - Address | Erase flash sector corresponding to address argument. |
| --range-erase | -A | R - File | Erase flash memory range. |
| --auto-erase | -a | N/A | Auto-erase (use it with flash command). |
| --verify, -V | N/A | Verify written file after a flash operation. |
| --flash-id, -i | N/A | Print information about the flash chip installed on the cart. |
| --gpio-ctrl, -g | R - Pin data | Manually control GPIO port pins of the microcontroller. |
| --wifi-flash, -w | R - File | Uploads a firmware blob to the cartridge WiFi module. |
| --pushbutton, -p | N/A | Read programmer pushbutton status. |
| --bootloader, -b | N/A | Enters DFU bootloader mode, to update programmer firmware. |
| --dry-run, -d | N/A | Performs a dry run (parses command line but does nothing). |
| --version, -R | N/A | Print version information and exit. |
| --verbose, -v | N/A | Write additional information on console while performing actions. |
| --help, -h | N/A | Print a brief help screen and exit. |

The Argument type column contains information about the parameters associated with every option. If the option takes no arguments, it is indicated by “N/A” string. If the option takes a required argument, the argument type is prefixed with “R” character. Supported argument types are File, Address and Pin Data:
* File: Specifies a file name. Along with the file name, optional address and length fields can be added, separated by the colon (:) character, resulting in the following format:
file_name[:address[:length]]
* Address: Specifies an address related to the command (e.g. the address to which to flash a cartridge ROM or WiFi firmware blob).
* Pin Data: Data related to the read/write operation of the port pins, with the format:
pin_mask:read_write[:value]

When using Pin Data arguments, each of the 3 possible parameters takes 6 bytes: one for each 8-bit port on the chip from PA to PF. Each of the arguments corresponds to the row with the same name on table 3. The value parameter is only required when writing to any pin on the ports. It is recommended to specify each parameter using hexadecimal values (using the prefix '0x').

The --pushbutton switch returns pushbutton status on the program exit code (so it is easily readable for programs/scripts using mdma-cli. The returned code uses the two least significant bits:
* BIT0: pushbutton status. The pushbutton is pressed if this bit is set.
* BIT1: pushbutton event. If this bit is set, there has been an event (button press and/or release) since the last mdma-cli invocaton. Note this bit is reset each time the program is launched with the --pushbutton switch.

E.g. if the button is pressed, and keeps being pressed when the program evaluates the --pushbutton function, the returned code will be 0x03 (pushbutton event + button pressed). If immediately called before the button is released, returned code will be 0x01 (no event + button pressed). If the button is released and then the program is called again, returned code will be 0x02 (pushbutton event + no button pressed).

Some more examples of the command invocation and its arguments are:
* `$ mdma -Q → Starts the QT GUI.
* `$ mdma -ef rom_file` → Erases entire cartridge and flashes rom_file.
* `$ mdma --erase -f rom_file:0x100000` → Erases entire cartridge and flashes contents of rom_file, starting at address 0x100000.
* `$ mdma -s 0x100000` → Erases flash sector containing 0x100000 address.
* `$ mdma -Vf rom_file:0x100000:32768` → Flashes 32 KiB of rom_file to address 0x100000, and verifies the operation.
* `$ mdma --read rom_file::1048576` → Reads 1 MiB of the cartridge flash, and writes it to rom_file. Note that if you want to specify length but do not want to specify address, you have to use two colon characters before length. This way, missing address argument is interpreted as 0.
* `$ mdma -g 0xFF00FFFF0000:0x110000000000:0x000012340000` → Reads data on port A, and writes 0x1234 on ports PC and PD.
* `$ mdma -w wifi-firm:0x40000` → Uploads wifi-firm firmware blob to the WiFi module, at address 0x40000.

# Authors
This program has been written by Migue/Manveru and doragasu.

# Contributions
Contributions are welcome. If you find a bug please open an issue, and if you have implemented a cool feature/improvement, please send a pull request.

# License
This program is provided with NO WARRANTY, under the [GPLv3 license](https://www.gnu.org/licenses/gpl-3.0.html).
