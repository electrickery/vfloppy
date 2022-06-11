Vfloppy
Intro

Vfloppy is a disk simulator for the Epson PX-4 and PX-8. It uses the epsp protocol used by the Epson TF-15, TF-20, PF-10 and compatible disk stations. It is written in C for Linux, and should be portable to any *nix with standard libraries. Apart from the program, only a serial port and a cable to connect it to the PX-4 or PX-8 is needed. 

Origin

The original version, written by Justin Mitchell and friends, consisted only of a TF-15 simulator for the PX-4 and a formatter to create empty images. The floppy images were files containing only the sector information of the tracks 4 and up, as the lower tracks are not used by CP/M. This format uses the extension ".vfd".

Utilities

The versions 1.1 to 1.4 add support for the PX-8, and a read and write utility to transfer files between the images and the file system. Due to a bug, only file sizes below 32 kByte are supported by these utilities. These versions of the utilities use functions which make them less ideal for porting to Windows.

Emulators and D88

With the advent of the PX4 and PX-8 emulators, a new floppy image format appeared, Pasopia D88. As this format contains the complete floppy (also tracks 0 to 3).

The 2.0 version of the simulator is an almost complete rewrite based on the Epson protocol description. This was done to make extension of the command set doable. By the way, only the basic commands for reading and writing are supported, the commands for formatting and disk copy are not. Having those too would be nice, but not really needed as these operations can be done on the host computer.

The 2.2 version is tested on x86 Linux and both Raspberry Pi (Raspbian) and BeagleBone ARM (Debian).

Programs exist to convert Teledisk and ImageDisk images to D88 (http://homepage3.nifty.com/takeda-toshiya/).

Addendum for 2.3: The only change in this version is a bug fix from Ivo Burkart. It fixes the occasional failing seek operation. 

fjkraan, 2015-10-27


The original site for this project is http://fjkraan.home.xs4all.nl/comp/px4/vfloppy/. There is an other repository at https://sourceforge.net/projects/vfloppy/