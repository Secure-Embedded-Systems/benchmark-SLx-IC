ifeq ($(OS),Windows_NT)
  # Cygwin Configuration
  MSPBASE=C:/ti/msp430-gcc/bin
  MSPCCFLAGS=-Wall -Os -fdata-sections -mlarge -ffunction-sections -mmcu=msp430fr5994 
  INCLUDEFLAGS=-IC:/ti/msp430-gcc/bin/../include -Idriverlib/MSP430FR5xx_6xx
else
  # Linux Ubuntu Configuration
  MSPBASE=/opt/ti/msp430-gcc/bin
  MSPCCFLAGS=-Wall -Os -fdata-sections -mlarge -ffunction-sections -mmcu=msp430fr5994 -mlarge
  INCLUDEFLAGS=-I/opt/ti/msp430-gcc/include/ -Idriverlib/MSP430FR5xx_6xx 
endif

MSPCC=$(MSPBASE)/msp430-elf-gcc
MSPOBJCOPY=$(MSPBASE)/msp430-elf-objcopy
MSPOBJDUMP=$(MSPBASE)/msp430-elf-objdump
MSPSIZE=$(MSPBASE)/msp430-elf-size
MSPAR=$(MSPBASE)/msp430-elf-ar

OBJS=adc12_b.o \
        mpu.o \
        gpio.o \
        framctl.o \
        framctl_a.o \
        cs.o \
        sysctl.o \
        sfr.o \
        rtc_c.o \
        mpy32.o \
        timer_a.o \
        dma.o \
        ref_a.o \
        wdt_a.o \
        aes256.o \
        #crc32.o \
        #eusci_a_spi.o \
        #eusci_a_uart.o \
        #rtc_b.o \
        #comp_e.o \
        #eusci_b_i2c.o \
        #pmm.o \
        #timer_b.o \
        #crc.o \
        #esi.o \
        #eusci_b_spi.o \
        #lcd_c.o \
        #ram.o \
        #tlv.o

compile: $(OBJS) libdriverfr5994.a

libdriverfr5994.a: $(OBJS)
	$(MSPAR) cr libdriverf5994.a $(OBJS)

%.o: %.c
	$(MSPCC) $(INCLUDEFLAGS) $(MSPCCFLAGS) -c $< -o $@ 
	$(MSPSIZE) $@

clean:
	rm -rf $(OBJS) libdriverfr5994.a

