# minicom-buddy

The *minicom* has a macro feature to transmit pre-defined text to its terminal.
The macro function is extremly useful for the embedded software engineers
who usually type the same commands repeatedly. However, editing and storing
macros are inconvenient with minicom's interface.

The *mbuddy* is a GUI based minicom macro tool to overcome the issues above.

This projects has two source code package, one is 'mbuddy' itself and 'minicom-2.7'.
The minicom is slightly modified to work with mbuddy.

## Building mbuddy

### 1. Required Libraries

gtk+-2.0    (version 2.24.10 or above )
libxml2-dev (version 2.7.8 or above )


### 2. Build

$ make


### 3. Installation

Default installation directories are:

binary : /usr/local/bin
resources : ~/.mbuddy/

If you want to change them, please open Makefile and edit "INSTALL_BIN" and "INSTALL_RESOURCE".

$ sudo make install


## Building minicom-2.7

Run the following commands.

$ ./configure

$ make


