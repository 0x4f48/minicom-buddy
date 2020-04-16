# minicom-buddy

The *minicom* has a macro feature to transmit pre-defined text to its terminal.
The macro function is extremly useful for the embedded software engineers
who usually type the same commands repeatedly. However, editing and storing
macros are inconvenient with minicom's interface.

The *mbuddy* is a GUI based minicom macro tool to overcome the issues above.

This projects has two source code package, one is 'mbuddy' itself and 'minicom-2.7'.
The minicom is slightly modified to work with mbuddy.

See how it works @ https://www.youtube.com/watch?v=7liZnp4GTzA

[![mbuddy demo](https://img.youtube.com/vi/7liZnp4GTzA/0.jpg)](https://www.youtube.com/watch?v=7liZnp4GTzA)

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

### 1. Required Libraries

libncurses5-dev

### 2. Build

Run the following commands.

$ sudo apt-get install libncurses5-dev

$ ./configure

$ make

* If you have warning like "aclocal-1.14 is missing...", replace "aclocal-1.14" with "aclocal-1.15" (the version on your system) in the ./configure file. Run ./configure again and build the minicom.



### 3. Installation

$ sudo make install
