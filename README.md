# benchmark-SLx-IC

This repository provides the artifact for "Benchmarking and configuring security levels in intermittent computing" published in ACM Transactions on Embedded Computing Systems. It contains code authored by Archanaa S. Krishnan, Daniel Dinu, and Charles Suslowicz. The implementation details can be found at [the publication](https://dl.acm.org/doi/10.1145/3522748). If you use this artifact, please cite our [paper](https://dblp.org/rec/journals/tecs/KrishnanS22).

We used several third party implementations were used to create this artifact:
1. Secure Intermittent Computing Protocol's (SICP's) cryptographic algorithms are implemented using [cifra](https://github.com/ctz/cifra).
2. [BEEBs benchmark](https://github.com/mageec/beebs)
3. TI's IP Encapsulation feature, [IPE](http://www.ti.com/lit/slaa685)

## SICP Approach using CTPL 

The checkpoint security is in this benchmark is based on an optimized version of the [Secure Intermittent Computing Protocol (SICP)](https://ieeexplore.ieee.org/document/8714997). At a high-level, it performs the following:
1. On checkpoint call:
	1. Perform authenticated encryption over the checkpoint.
2. On power-up:
	1. Perform authenticated decryption over the checkpoint.
The details of the protocol with optimizations be found in the [artifact's publication](https://dl.acm.org/doi/10.1145/3522748). The implementation of optimized SICP is available in each benchmark under ``sicp`` folder, e.g. in ``sicp-bench-template/sicp``.

### Compute Through Power Loss (CTPL) library 

Our benchmark used TI's CTPL library, which supports saving the processor state and stack to FRAM upon shutdown and restoring the state when power is returned. The base code for CTPL is example 4 from TI's FRAM Utilities. Due to copyright in CTPL files, we are not including the CTPL utility in this artifact. Refer to TI's [MSP-FRAM-UTILTIES](https://www.ti.com/tool/MSP-FRAM-UTILITIES) for free download options. 

# Using the benchmark

## Folder structure
1. [``benchmark``](benchmark) contains the benchmarks from [the publication](https://dl.acm.org/doi/10.1145/3522748).
2. [``ipe``](ipe) contains an example for implementing TI's IP Encapsulation. IPE feature is typically enabled / disabled using TI's CCS. Since this benchmark was built using GNU tools and not CCS and for ease of benchmarking, IPE is not enabled in the benchmark to emulate tamper-free memory. Instead, IPE is provided as a standalone example in [ipe] folder. The ``ipe_example`` provided in [ipe] folder is a port of TI's IPE example provided in [MSP Code Protection Features](http://www.ti.com/lit/slaa685).
3. [``sicp-bench-template``](sicp-bench-template) contains the template used to add multi-level checkpoint security to BEEBs benchmark. 

## Usage
1. Copy [``sicp-bench-template``](sicp-bench-template) to new project as a base template.
2. Add additional source/headers in an appropriate new directory.
3. Copy the CTPL files from TI FRAM Utilies examples along with the example's main.c. Update #defines in `ctpl/ctpl_msp430fr5994.c` to save the state of desired peripherals and save them in secure or non-secure storage. Update 'ctpl/ctpl_low_level.S' to store the CTPL variables required for checkpoints in secure or non-secure memory. 
4. Replace functionality in `main.c` with desired operation(s). Update the main.c to add the benchmark function, benchmark(), and secure checkpoint function call around the benchmark.  
5. Copy the ``rng`` folder from TI FRAM Utilities example to generate the nonce associated with the first checkpoint. Due to copyright in RNG files, we are not including the ``rng`` folder in our benchmarks.
6. Update Makefile to include new source and header file declarations.
7. Check Makefile build options for your project.
8. Update memory map in `msp430fr5994.ld` as necessary for new project. This is only required for projects that need to specify the specific section locations (FRAM, SRAM, stack, etc).
9. Update your installation of `msp430-elf-gcc` based on [CTPL updates](#CTPL updates to core files) .
10. Build and Test.

### Updates to CTPL core files

* The files in ctpl reference directory were updated/changed from the default distributed files in msp430-elf-gcc.
In both cases defines were not properly detected when gcc was being used to compile assembly files that needed to be pre-processed (.S files).

#### in430.h diff

Changed to properly catch GNU Assembler define for processing .S files.

```
diff /opt/ti/msp430_gcc/include/in430.h /opt/ti/msp430_gcc/include/in430.h.orig
42c42
< #if !(defined(__ASSEMBLER__) || defined(_GNU_ASSEMBLER))
---
> #if !defined(__ASSEMBLER__)
```

#### msp430fr5994.h diff

Changed to properly catch GNU Assembler define for processing .S files.

```
diff msp430fr5994.h /opt/ti/msp430_gcc/include/msp430fr5994.h.orig
103c103
< #if (!defined(__STDC__) || defined(_GNU_ASSEMBLER_) || defined(__ASSEMBLER__)) /* Begin #defines for assembler */
---
> #ifndef __STDC__ /* Begin #defines for assembler */
```


## Notes
1. The `msp430fr5994.ld` file contains an updated memory map.  
2. The `Makefile` will create a build directory containing the generated object files (.o), include files (.d), and a defines.txt that contains all defines created during the build process for troubleshooting. The creation of the defines.txt should be removed if your project is very large and it slows down the build process.
3. The gdb directory contains gdb command files used when connecting/loading/troubleshooting an msp430fr5994 via mspdebug.
4. This repository was created and tested against the following version of `msp430-elf-gcc`:
	```
	$ msp430-elf-gcc --version
	msp430-elf-gcc (SOMNIUM Technologies Limited - msp430-gcc 6.4.0.32) 6.4.0
	```



