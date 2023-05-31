ifeq ($(OS),Windows_NT)
  # Cygwin Configuration
  MSPBASE=/cygdrive/c/ti/msp430_gcc/bin
  MSPCCFLAGS=-Wall -g -Os -fdata-sections -ffunction-sections -mmcu=msp430fr5994 
  MSPLDFLAGS=-Wl,--gc-sections -minrt -T msp430fr5994.ld
  INCLUDEFLAGS=-IC:/ti/msp430_gcc/bin/../include -Idriverlib/MSP430FR5xx_6xx
else
  # Linux Ubuntu Configuration
  MSPBASE=/opt/ti/msp430_gcc/bin
  MSPCCFLAGS=-Wall -g -Os -fdata-sections -ffunction-sections -mmcu=msp430fr5994 
  MSPLDFLAGS=-mmcu=msp430fr5994 -Wl,--gc-sections -minrt -T msp430fr5994.ld
  INCLUDEFLAGS=-I/opt/ti/msp430_gcc/include/ -Idriverlib/MSP430FR5xx_6xx 
endif

# tools
MSPCC=$(MSPBASE)/msp430-elf-gcc
MSPOBJCOPY=$(MSPBASE)/msp430-elf-objcopy
MSPOBJDUMP=$(MSPBASE)/msp430-elf-objdump
MSPSIZE=$(MSPBASE)/msp430-elf-size

all:
	@echo "Targets: "
	@echo "  compile    Compile software packages"
	@echo "  clean      Remove intermediate files"

compile: main.elf

main.o: main.c
	$(MSPCC) $(INCLUDEFLAGS) $(MSPCCFLAGS) -c $< -o $@ 
	$(MSPSIZE) $@

main.elf: main.o
	cd driverlib/MSP430FR5xx_6xx/; make; cd ../..
	$(MSPCC) $(MSPCCFLAGS) main.o -Wl,-Map,main.map driverlib/MSP430FR5xx_6xx/libdriverf5994.a $(MSPLDFLAGS) -o main.elf
	$(MSPSIZE) $@
	$(MSPOBJDUMP) -S -D main.elf > main.lst

clean:
	cd driverlib/MSP430FR5xx_6xx/; make clean; cd ../..
	rm -rf main.elf main.map *.o *~ 


