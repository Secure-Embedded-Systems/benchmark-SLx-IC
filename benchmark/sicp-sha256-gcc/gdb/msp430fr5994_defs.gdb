# set commands for msp430fr5994 peripherals in gdb

# Individual Bits
set $BIT0 = 0x0001
set $BIT1 = 0x0002
set $BIT2 = 0x0004
set $BIT3 = 0x0008
set $BIT4 = 0x0010
set $BIT5 = 0x0020
set $BIT6 = 0x0040
set $BIT7 = 0x0080

# Status Registers
set $SYSRSTIV = (unsigned char *)0x019e

# Port 1
set $P1OUT = (unsigned char *)0x0202

# Port 5
set $P5OUT = (unsigned char *)0x0242
set $P5REN = (unsigned char *)0x0246
set $P5DIR = (unsigned char *)0x0244
set $P5IES = (unsigned char *)0x0258
set $P5IE  = (unsigned char *)0x025A
set $P5IFG = (unsigned char *)0x025C

# ADC
set $ADC12_BASE_ADDR = 0x0800
set $ADC12IV   = (unsigned int *) ($ADC12_BASE_ADDR + 0x18)
set $ADC12IER2 = (unsigned int *) ($ADC12_BASE_ADDR + 0x16)
set $ADC12HIIE = (unsigned int *) ($ADC12_BASE_ADDR + 0x16)

# General Config/Setup
set disassemble-next-line on
