# gdb script to connect to mspdebug and load main.elf

# Connect to board
target remote rpi3-0:2000

# Set target program
file main.elf

# Load common defines
source gdb/msp430fr5994_defs.gdb
