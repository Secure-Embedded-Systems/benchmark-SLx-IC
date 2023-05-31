ifeq ($(OS),Windows_NT)
  # Cygwin Configuration
  MSPBASE=/cygdrive/c/ti/msp430_gcc/bin
  MSPCCFLAGS=-Wall -g -Os -fdata-sections -ffunction-sections -mmcu=msp430fr5994 
  INCLUDEFLAGS=-IC:/ti/msp430_gcc/bin/../include -Idriverlib/MSP430FR5xx_6xx
else
  # Linux Ubuntu Configuration
  MSPBASE=/opt/ti/msp430_gcc/bin
  MSPCCFLAGS=-Wall -g -Os -fdata-sections -ffunction-sections -mmcu=msp430fr5994 
  INCLUDEFLAGS=-I/opt/ti/msp430_gcc/include/ -Idriverlib/MSP430FR5xx_6xx 
endif

MSPCC=$(MSPBASE)/msp430-elf-gcc
MSPOBJCOPY=$(MSPBASE)/msp430-elf-objcopy
MSPOBJDUMP=$(MSPBASE)/msp430-elf-objdump
MSPSIZE=$(MSPBASE)/msp430-elf-size
MSPAR=$(MSPBASE)/msp430-elf-ar

OBJS=adc12_b.o crc32.o eusci_a_spi.o framctl.o mpu.o ref_a.o sysctl.o wdt_a.o aes256.o cs.o eusci_a_uart.o framctl_a.o mpy32.o rtc_b.o timer_a.o comp_e.o dma.o eusci_b_i2c.o gpio.o pmm.o rtc_c.o timer_b.o crc.o esi.o eusci_b_spi.o lcd_c.o ram.o sfr.o tlv.o

compile: $(OBJS) libdriverfr5994.a

libdriverfr5994.a: $(OBJS)
	$(MSPAR) cr libdriverf5994.a $(OBJS)

%.o: %.c
	$(MSPCC) $(INCLUDEFLAGS) $(MSPCCFLAGS) -c $< -o $@ 
	$(MSPSIZE) $@

clean:
	rm -rf $(OBJS) libdriverfr5994.a

