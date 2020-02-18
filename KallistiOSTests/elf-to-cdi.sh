#!/bin/bash

PROG="$1"		#Name of your program ($PROG.elf)
IP_PATH="$CRAYON_BASE/IP.BIN"	#The path to your IP.BIN file

sh-elf-objcopy -R .stack -O binary $PROG.elf $PROG.bin
$KOS_BASE/utils/scramble/scramble $PROG.bin 1st_read.bin
mkisofs -G $IP_PATH -C 0,11702 -J -l -r -o $PROG.iso .
cdi4dc $PROG.iso $PROG.cdi

