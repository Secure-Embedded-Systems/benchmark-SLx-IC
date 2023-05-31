# gdb script to connect to mspdebug and load main.elf

target remote rpi3-0:2003
file main.elf
load main.elf
