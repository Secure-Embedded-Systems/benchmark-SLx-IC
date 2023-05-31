ifeq ($(OS),Windows_NT)
  # Cygwin Configuration
  MSPBASE=C:/ti/msp430-gcc/bin
  INCLUDEFLAGS=-IC:/ti/msp430-gcc/bin/../include 
else
  # Linux Ubuntu Configuration
  MSPBASE=/opt/ti/msp430-gcc/bin
  INCLUDEFLAGS=-I/opt/ti/msp430-gcc/include 
endif

# tools
MSPCC=$(MSPBASE)/msp430-elf-gcc
MSPAS=$(MSPBASE)/msp430-elf-as
MSPOBJCOPY=$(MSPBASE)/msp430-elf-objcopy
MSPOBJDUMP=$(MSPBASE)/msp430-elf-objdump
MSPSIZE=$(MSPBASE)/msp430-elf-size

# non-os related config
STACKSIZE=160
MCU=msp430fr5994
# Errata for 5994, GCC uneffected by CPU40, but does not currently support CPU21 or CPU22
SIERRATA=-msilicon-errata=CPU21 -msilicon-errata=CPU22 -msilicon-errata=CPU40

#MSPASFLAGS=-mmcu=$(MCU) --defsym CTPL_STACK_SIZE=$(STACKSIZE)

MSPCCFLAGS=-Wall -g -Os -fdata-sections -ffunction-sections -mmcu=$(MCU) -mlarge -DCTPL_STACK_SIZE=$(STACKSIZE) $(LPMDEBUG)
MSPLDFLAGS=-mmcu=$(MCU) -Wl,-gc-sections -T $(MCU).ld
MSPCCASFLAGS=-g -mmcu=$(MCU) -mlarge -DCTPL_STACK_SIZE=$(STACKSIZE) -x assembler-with-cpp $(LPMDEBUG) 
BUILDDIR=./build

# non-os related includes
INCLUDEFLAGS += \
	-Ictpl/ \
	-Idriverlib/MSP430FR5xx_6xx/ \
	-Irng/ \
	-Icifra/ \
	-Isic/ \

SRCS_C = \
	main.c 

# ctpl sources
SRCS_C += \
	ctpl/ctpl.c \
	ctpl/ctpl_pre_init.c \
	ctpl/ctpl_msp430fr5994.c \
	ctpl/peripherals/ctpl_adc10_b.c \
	ctpl/peripherals/ctpl_adc12_b.c \
	ctpl/peripherals/ctpl_adc.c \
	ctpl/peripherals/ctpl_captio.c \
	ctpl/peripherals/ctpl_comp_d.c \
	ctpl/peripherals/ctpl_comp_e.c \
	ctpl/peripherals/ctpl_crc16.c \
	ctpl/peripherals/ctpl_crc32.c \
	ctpl/peripherals/ctpl_cs.c \
	ctpl/peripherals/ctpl_dmax_3.c \
	ctpl/peripherals/ctpl_dmax_6.c \
	ctpl/peripherals/ctpl_ecomp.c \
	ctpl/peripherals/ctpl_eusci_a.c \
	ctpl/peripherals/ctpl_eusci_b.c \
	ctpl/peripherals/ctpl_fram.c \
	ctpl/peripherals/ctpl_lcd_c.c \
	ctpl/peripherals/ctpl_lcd_e.c \
	ctpl/peripherals/ctpl_mpu.c \
	ctpl/peripherals/ctpl_mpy32.c \
	ctpl/peripherals/ctpl_pmm.c \
	ctpl/peripherals/ctpl_port.c \
	ctpl/peripherals/ctpl_port_int.c \
	ctpl/peripherals/ctpl_rc.c \
	ctpl/peripherals/ctpl_ref_a.c \
	ctpl/peripherals/ctpl_rtc_b.c \
	ctpl/peripherals/ctpl_rtc.c \
	ctpl/peripherals/ctpl_rtc_c.c \
	ctpl/peripherals/ctpl_sac.c \
	ctpl/peripherals/ctpl_sfr.c \
	ctpl/peripherals/ctpl_sys.c \
	ctpl/peripherals/ctpl_timer_2.c \
	ctpl/peripherals/ctpl_timer_3.c \
	ctpl/peripherals/ctpl_timer_5.c \
	ctpl/peripherals/ctpl_timer_7.c \
	ctpl/peripherals/ctpl_tri.c \
	ctpl/peripherals/ctpl_wdt_a.c 

SRCS_S = \
	ctpl/ctpl_low_level.S 

# rng sources
SRCS_C += \
	rng/rng.c \
	rng/rng_pre_init.c

# cifra sources (eax)
SRCS_C += \
	cifra/aes.c \
	cifra/blockwise.c \
	cifra/cmac.c \
	cifra/eax.c \
	cifra/gf128.c \
	cifra/hmac.c \
	cifra/modes.c 

	#cifra/norx.c \
	#cifra/ocb.c \
	#cifra/pbkdf2.c \
	#cifra/poly1305.c \
	#cifra/salsa20.c \
	#cifra/sha1.c \
	#cifra/sha256.c \
	#cifra/sha3.c \
	#cifra/sha512.c \
	#cifra/cbcmac.c \
	#cifra/ccm.c \
	#cifra/chash.c \
	#cifra/drbg.c \
	#cifra/gcm.c 

# sic sources 
SRCS_C += \
	sic/sic.c



OBJS = $(SRCS_C:%.c=$(BUILDDIR)/%.o) $(SRCS_S:%.S=$(BUILDDIR)/%.o)

DEP = $(OBJS:%.o=%.d)

# Include all the .d files (dependencies)
-include $(DEP)

all:
	@echo "Targets: "
	@echo "  compile    Compile software packages"
	@echo "  clean      Remove intermediate files"

compile debug: main.elf

main.elf: $(OBJS) 
	cd driverlib/MSP430FR5xx_6xx/; make; cd ../..
	$(MSPCC) $(INCLUDEFLAGS) $(MSPCCFLAGS) $(OBJS) driverlib/MSP430FR5xx_6xx/libdriverf5994.a $(MSPLDFLAGS) -o main.elf
	$(MSPSIZE) $@
	$(MSPOBJDUMP) -S -D main.elf > main.lst
	$(MSPCC) $(INCLUDEFLAGS) $(MSPCCFLAGS) $(SRCS_C) $(SRCS_S) -E -dM > $(BUILDDIR)/defines.txt

# pattern rule for c files
# the -MMD switch creates a .d file with the same name as the .o file
$(BUILDDIR)/%.o : %.c
	mkdir -p $(@D)
	$(MSPCC) $(INCLUDEFLAGS) $(MSPCCFLAGS) -MMD -c $< -o $@ 
#	$(MSPSIZE) $@

# pattern rule for assembly files
# the -MMD switch creates a .d file with the same name as the .o file
$(BUILDDIR)/%.o : %.S
	mkdir -p $(@D)
	$(MSPCC) $(INCLUDEFLAGS) $(MSPCCASFLAGS) -MMD -c $< -o $@ 
#	$(MSPSIZE) $@

# Debug option for LPM
.PHONY: debug
debug: LPMDEBUG = -DCTPL_LPM_DEBUG

.PHONY: clean
clean:
	cd driverlib/MSP430FR5xx_6xx/; make clean; cd ../..
	rm -rf main.elf $(OBJS) $(DEP) *~ $(BUILDDIR)/defines.txt


