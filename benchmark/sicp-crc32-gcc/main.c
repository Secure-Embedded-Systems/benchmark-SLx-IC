/* --COPYRIGHT--,BSD_EX
 * Copyright (c) 2016, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *******************************************************************************
 *
 *                       MSP430 CODE EXAMPLE DISCLAIMER
 *
 * MSP430 code examples are self-contained low-level programs that typically
 * demonstrate a single peripheral function or device feature in a highly
 * concise manner. For this the code may rely on the device's power-on default
 * register values and settings such as the clock configuration and care must
 * be taken when combining code from several examples to avoid potential side
 * effects. Also see www.ti.com/grace for a GUI- and www.ti.com/msp430ware
 * for an API functional library-approach to peripheral configuration.
 *
 * --/COPYRIGHT--*/
//******************************************************************************
//   MSP430FR5x9x Demo - CRC32, Compare CRC32 output with software-based
//                       algorithm
//
//   Description: An array of 16 random 16-bit values are moved through the
//   CRC32 module, as well as a software-based CRC32-ISO3309 algorithm. Due to
//   the fact that the software-based algorithm handles 8-bit inputs only,
//   the 16-bit words are broken into 2 8-bit words before being run through
//   (lower byte first). The outputs of both methods are then compared to ensure
//   that the operation of the CRC module is consistent with the expected
//   outcome. If the values of each output are equal, set P1.0, else reset.
//
//   MCLK = SMCLK = default DCO~1MHz
//
//                MSP430FR5994
//             -----------------
//         /|\|                 |
//          | |                 |
//          --|RST              |
//            |                 |
//            |             P1.0|--> LED
//
//   William Goh
//   Texas Instruments Inc.
//   April 2016
//   Built with IAR Embedded Workbench V6.40 & Code Composer Studio V6.1
//******************************************************************************
#include <msp430.h>
#include <stdlib.h>
#include <string.h>

#include "ctpl.h"
#include "driverlib.h"
#include "rng.h"
#include "sic.h"

#include "sglib.h"

/* ctpl defines */
#define ADC_MONITOR_THRESHOLD   3.0
#define ADC_MONITOR_FREQUENCY   1000

#define MCLK_FREQUENCY          1000000
#define SMCLK_FREQUENCY         1000000

//#define CRC32_HW
#define CRC32_SW

void initGpio(void);
void initClocks(void);
void initAdcMonitor(void);


int benchmark();


// Software CRC32 algorithm function declaration
#ifdef CRC32_SW

void initSwCrc32Table(void);
unsigned long updateSwCrc32( unsigned long crc, char c );

#endif
int main(void)
{
    uint16_t i, j;

    /* Halt the watchdog timer */
    WDTCTL = WDTPW | WDTHOLD;

    /* Initialize the GPIO, clock system and the ADC monitor. */
    initGpio();
    initClocks();
    initAdcMonitor();
    sic_initialize();

    /* Enable global interrupts. */
    __enable_interrupt();

    /* Setup LED 1 */
    P1DIR |= BIT0;
    P1OUT &= ~BIT0;
    /* Setup LED 2 */
    P1DIR |= BIT1;
    P1OUT &= ~BIT1;

    /* Setup GPIO P7.1 */
    P7DIR |= BIT1;
    P7OUT &= ~BIT1;

    /* Wait for button before zeroizing, show LED 2 while waiting */
    P1OUT |= BIT1;
    P1OUT |= BIT0;
    while ((P5IN & BIT6) == BIT6) {}
    P1OUT &= ~BIT1;
    P1OUT &= ~BIT0;

    /* Blink LED when ready for power loss */
    /* Test EAX chaining */
    i = 0;
    P1OUT |= BIT0;
    P7OUT |= BIT1;


  while (1) {


      P1OUT &= ~BIT0;
      P7OUT &= ~BIT1;
//      for(i=0; i<10;i++)
      {
          /* Function to be benchmarked */
          benchmark();
//          ctpl_checkpointOnly();

//          sic_refresh();
//          sic_restore();

      }


      P7OUT |= BIT1;
      P1OUT |= BIT0;
      __delay_cycles(MCLK_FREQUENCY/10);
      benchmark();

    }


    /* catch everything loop -- both leds on */
    P1OUT |= BIT1;
    P1OUT |= BIT0;
    while (1) {};

    return 0;
}

void initGpio(void)
{
    /* Configure GPIO to default state */
    P1OUT  = 0; P1DIR  = 0xFF;
    P2OUT  = 0; P2DIR  = 0xFF;
    P3OUT  = 0; P3DIR  = 0xFF;
    P4OUT  = 0; P4DIR  = 0xFF;
    P5OUT  = 0; P5DIR  = 0xFF;
    P6OUT  = 0; P6DIR  = 0xFF;
    P7OUT  = 0; P7DIR  = 0xFF;
    P8OUT  = 0; P8DIR  = 0xFF;
    PJOUT  = 0; PJDIR  = 0xFFFF;

    /* Disable the GPIO power-on default high-impedance mode. */
    PM5CTL0 &= ~LOCKLPM5;

    /* Prep P5.6 for Button Press to start zeroize */
    /* This is just a button that can be checked with (P5IN & BIT6) */
    P5OUT |= BIT6;
    P5REN |= BIT6;
    P5DIR = 0xff ^ BIT6;

}

void initClocks(void)
{
    /* Clock System Setup, MCLK = SMCLK = DCO (1MHz), ACLK = VLOCLK */
    CSCTL0_H = CSKEY >> 8;
    CSCTL1 = DCOFSEL_3; //_3: 8MHz, _0: 1MHz
    CSCTL2 = SELA__VLOCLK | SELS__DCOCLK | SELM__DCOCLK;
    CSCTL3 = DIVA__1 | DIVS__1 | DIVM__1;
    CSCTL0_H = 0;
}

void initAdcMonitor(void)
{
    /* Initialize timer for ADC trigger. */
    TA0CCR0 = (SMCLK_FREQUENCY/ADC_MONITOR_FREQUENCY);
    TA0CCR1 = TA0CCR0/2;
    TA0CCTL1 = OUTMOD_3;
    TA0CTL = TASSEL__SMCLK | MC__UP;

    /*
     * Configure internal 2.0V reference.
     * Reference Options:
     * REFVSEL_0 = 1.2V
     * REFVSEL_1 = 2.0V -- default
     * REFVSEL_2 = 2.5V
     * REFVSEL_3 = 2.5V (same as REFVSEL_2)
     */
    while(REFCTL0 & REFGENBUSY);
    REFCTL0 |= REFVSEL_1 | REFON;
    while(!(REFCTL0 & REFGENRDY));

    /*
     * Initialize ADC12_B window comparator using the battery monitor.
     * The monitor will first enable the high side to the monitor voltage plus
     * 0.1v to make sure the voltage is sufficiently above the threshold. When
     * the high side is triggered the interrupt service routine will switch to
     * the low side and wait for the voltage to drop below the threshold. When
     * the voltage drops below the threshold voltage the device will invoke the
     * compute through power loss shutdown function to save the application
     * state and enter complete device shutdown.
     */
    ADC12CTL0 = ADC12SHT0_2 | ADC12ON;
    ADC12CTL1 = ADC12SHS_1 | ADC12SSEL_0 | ADC12CONSEQ_2 | ADC12SHP;
    ADC12CTL3 = ADC12BATMAP;
    ADC12MCTL0 = ADC12INCH_31 | ADC12VRSEL_1 | ADC12WINC;
    ADC12HI = (uint16_t)(4096*((ADC_MONITOR_THRESHOLD+0.1)/2)/(2.0));
    ADC12LO = (uint16_t)(4096*(ADC_MONITOR_THRESHOLD/2)/(2.0));
    ADC12IFGR2 &= ~(ADC12HIIFG | ADC12LOIFG);
    ADC12IER2 = ADC12HIIE;
    ADC12CTL0 |= ADC12ENC;
}


/*Benchmark var and functions*/

#define  POLYNOMIAL_32    0xEDB88320

// Holds a crc32 lookup table
#ifdef NONSECURE_BENCH
__attribute__ ((section (".nonsecure")))
#else
__attribute__ ((section (".secure")))
#endif
unsigned long crc32Table[256];
// Global flag indicating that the crc32 lookup table has been initialized
unsigned int crc32TableInit = 0;

const unsigned long CRC_Init = 0xFFFFFFFF;

#ifdef NONSECURE_BENCH
__attribute__ ((section (".nonsecure")))
#else
__attribute__ ((section (".secure")))
#endif
unsigned short CRC_Input[] =
{
   0x0000, 0x7707, 0xee2c, 0x51ba, 0x076d, 0x708f,
   0xa535, 0x9e64, 0x0e32, 0xb8a4, 0xe0d5, 0x9788,
   0x4c2b, 0x7eb1, 0xe707, 0x1d91, 0x1db7, 0x6af2,
   0x7148, 0x84be, 0x1a7d, 0xe4eb, 0xf4d4, 0x83c7,
   0x9856, 0x646b, 0xfd7a, 0xc9ec, 0x1401, 0x63d9,
   0x3d63, 0x8d08, 0x3bc8, 0x105e, 0xd560, 0xa272,
   0xe4d1, 0x4b04, 0xd2fd, 0xb56b, 0x35b5, 0x426c,
   0xc9d6, 0xacbc, 0x32e3, 0x5c75, 0xdcd6, 0xab59,
   0x30ac, 0x51de, 0xc880, 0x6116, 0x21b4, 0x5623,
   0x9599, 0xb8bd, 0x289e, 0x8808, 0xc60c, 0xb124,
   0x7c87, 0x5868, 0xc1ab, 0x2d3d, 0x76dc, 0x0106,
   0x20bc, 0xefd5, 0x7189, 0xb51f, 0x9fbf, 0xe833,
   0xc9a2, 0x0f00, 0x968e, 0x9818, 0x7f6a, 0x082d,
   0x6c97, 0xe663, 0x6bf4, 0x6162, 0x8565, 0xf24e,
   0x95ed, 0x1b01, 0x82c1, 0xc457, 0x65b0, 0x1250,
   0xb8ea, 0xfcb9, 0x62df, 0x2d49, 0x8cd3, 0xfb65,
   0x6158, 0x3ab5, 0xa374, 0x30e2, 0x4adf, 0x3dd7,
   0xc46d, 0xd3d6, 0x436a, 0xd9fc, 0xad67, 0xdad0,
   0x2d73, 0x3303, 0xaa5f, 0x7cc9, 0x5005, 0x27aa,
   0x1010, 0xc90c, 0x5725, 0x85b3, 0xb966, 0xce9f,
   0xf90e, 0x29d9, 0xb022, 0xa8b4, 0x59b3, 0x2e81,
   0x5c3b, 0xc0ba, 0xed20, 0xb3b6, 0x03b6, 0x749a,
   0x4739, 0x9dd2, 0x0415, 0x1683, 0xe363, 0x9484,
   0x6a3e, 0x7a6a, 0xe40b, 0xff9d, 0x0a00, 0x7db1,
   0x9344, 0x8708, 0x1e68, 0xc2fe, 0xf762, 0x80cb,
   0x3671, 0x6e6b, 0xfe76, 0x2be0, 0x10da, 0x67cc,
   0xdf6f, 0x8ebe, 0x1743, 0x8ed5, 0xd6d6, 0xa17e,
   0xc2c4, 0x4fdf, 0xd1f1, 0x5767, 0x3fb5, 0x484b,
   0x2bda, 0xaf0a, 0x36f6, 0x7a60, 0xdf60, 0xa855,
   0x8eef, 0x4669, 0xcb8c, 0x831a, 0x256f, 0x5236,
   0x7795, 0xbb0b, 0x22b9, 0x262f, 0xc5ba, 0xb228,
   0x5a92, 0x5cb3, 0xc2a7, 0xcf31, 0x2cd9, 0x5b1d,
   0xc2b0, 0xec63, 0x759c, 0x930a, 0x9c09, 0xeb3f,
   0x6785, 0x0500, 0x9582, 0x7a14, 0x7bb1, 0x0c38,
   0x8e9b, 0xe5d5, 0x7cb7, 0xdf21, 0x86d3, 0xf142,
   0xb3f8, 0x1fda, 0x81cd, 0x265b, 0x6fb0, 0x1877,
   0x5ae6, 0xff0f, 0x66ca, 0x0b5c, 0x8f65, 0xf869,
   0xffd3, 0x166c, 0xa078, 0xd2ee, 0x4e04, 0x39c2,
   0x2661, 0xd060, 0x494d, 0x77db, 0xaed1, 0xd9dc,
   0x0b66, 0x37d8, 0xa953, 0x9ec5, 0x47b2, 0x30e9,
   0xf21c, 0xcaba, 0x5330, 0xa3a6, 0xbad0, 0xcd93,
   0x5729, 0x23d9, 0xb32e, 0x4ab8, 0x5d68, 0x2a94,
   0xbe37, 0xc30c, 0x5a1b, 0xef8d
};
//{
//    0xc00f, 0x9610, 0x5042, 0x0010,         // 16 random 16-bit numbers
//    0x7ff7, 0xf86a, 0xb58e, 0x7651,         // these numbers can be
//    0x8b88, 0x0679, 0x0123, 0x9599,         // modified if desired
//    0xc58c, 0xd1e2, 0xe144, 0xb691
//};

// Holds results obtained through the CRC32 module
#ifdef NONSECURE_BENCH
__attribute__ ((section (".nonsecure")))
#else
__attribute__ ((section (".secure")))
#endif
unsigned long CRC32_Result;

// Holds results obtained through SW
#ifdef NONSECURE_BENCH
__attribute__ ((section (".nonsecure")))
#else
__attribute__ ((section (".secure")))
#endif
unsigned long SW_CRC32_Results;



int benchmark(void)
{
    unsigned int i;

#ifdef CRC32_HW

    // First, use the CRC32 hardware module to calculate the CRC...
    CRC32INIRESW0 = CRC_Init & 0x0000FFFF;      // Init CRC32 HW module
    CRC32INIRESW1 = CRC_Init >> 16;             // Init CRC32 HW module

    for(i = 0; i < (sizeof(CRC_Input) >> 1); i=i+2)
    {
        // Input values into CRC32 Hardware
        CRC32DIW0 = (unsigned int) CRC_Input[i + 0];
        CRC32DIW1 = (unsigned int) CRC_Input[i + 1];
    }

    // Save the CRC32 result
    CRC32_Result = ((unsigned long) CRC32RESRW0 << 16);
    CRC32_Result = ((unsigned long) CRC32RESRW1 & 0x0000FFFF) | CRC32_Result;

#else


    // Now use a software routine to calculate the CRC32...

    // Init SW CRC32
    SW_CRC32_Results = CRC_Init;

    for(i = 0; i < (sizeof(CRC_Input) >> 1); i++)
    {
        // Calculate the CRC32 on the low-byte first
        SW_CRC32_Results = updateSwCrc32(SW_CRC32_Results, (CRC_Input[i] & 0xFF));

        // Calculate the CRC on the high-byte
        SW_CRC32_Results = updateSwCrc32(SW_CRC32_Results, (CRC_Input[i] >> 8));
    }
#endif
}

#ifdef CRC32_SW

// Calculate the SW CRC32 byte-by-byte
unsigned long updateSwCrc32( unsigned long crc, char c )
{
    unsigned long tmp, long_c;

    long_c = 0x000000ffL & (unsigned long) c;

    if (!crc32TableInit)
    {
        initSwCrc32Table();
    }

    tmp = crc ^ long_c;
    crc = (crc >> 8) ^ crc32Table[ tmp & 0xff ];

    return crc;
}

// Initializes the CRC32 table
void initSwCrc32Table(void)
{
    int i, j;
    unsigned long crc;

    for (i = 0; i < 256; i++)
    {
        crc = (unsigned long) i;

        for (j = 0; j < 8; j++)
        {
            if ( crc & 0x00000001L )
            {
                crc = ( crc >> 1 ) ^ POLYNOMIAL_32;
            }
            else
            {
                crc =   crc >> 1;
            }
        }
        crc32Table[i] = crc;
    }

    // Set flag that the CRC32 table is initialized
    crc32TableInit = 1;
}
#endif
