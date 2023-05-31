//******************************************************************************
//  CTPL FRAM Write Test
//
//  Wait for a powerloss interrupt from ADC12B (similar to TIs ex4 from the ctpl
//  library) and then try to write to as much FRAM as possible.
//
//  Initialize the FRAM to a pattern (0s) and blink and LED when ready.
//
//  Be aware that the mspdebug 'reset' function initializes FRAM to 0xff
//
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

void initGpio(void);
void initClocks(void);
void initAdcMonitor(void);

void initialise_benchmark();
int benchmark();
int verify_benchmark(int unused);

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
//          benchmark();
//          ctpl_checkpointOnly();

          sic_refresh();
          sic_restore();

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
#ifdef NONSECURE_BENCH
__attribute__ ((section (".nonsecure")))
#else
__attribute__ ((section (".secure")))
#endif
int array[100] = {14, 66, 12, 41, 86, 69, 19, 77, 68, 38, 26, 42, 37, 23, 17, 29, 55, 13,
  90, 92, 76, 99, 10, 54, 57, 83, 40, 44, 75, 33, 24, 28, 80, 18, 78, 32, 93, 89, 52, 11,
  21, 96, 50, 15, 48, 63, 87, 20, 8, 85, 43, 16, 94, 88, 53, 84, 74, 91, 67, 36, 95, 61,
  64, 5, 30, 82, 72, 46, 59, 9, 7, 3, 39, 31, 4, 73, 70, 60, 58, 81, 56, 51, 45, 1, 6, 49,
  27, 47, 34, 35, 62, 97, 2, 79, 98, 25, 22, 65, 71, 0};

volatile int found = 0;

int benchmark()
{
  volatile int cnt=0;
  int tmp, index, i;

  index = 0;
  for(i=0; i< 100; i++) {
    tmp = array[i];
    SGLIB_ARRAY_BINARY_SEARCH(int, array, 0, i, tmp, SGLIB_NUMERIC_COMPARATOR, found, index);
    cnt += index;
  }

  return cnt;
}

int verify_benchmark(int r) {
  int expected = 2455;
  if (r != expected)
    return 0;
  return 1;
}
/* Interrupt Service Routine for Powerloss Detection */
void __attribute__ ((interrupt(ADC12_VECTOR))) ADC12_ISR(void)
{
    switch(__even_in_range(ADC12IV, ADC12IV_ADC12LOIFG)) {
        case ADC12IV_NONE:        break;        // Vector  0: No interrupt
        case ADC12IV_ADC12OVIFG:  break;        // Vector  2: ADC12MEMx Overflow
        case ADC12IV_ADC12TOVIFG: break;        // Vector  4: Conversion time overflow
        case ADC12IV_ADC12HIIFG:                // Vector  6: Window comparator high side
            /* Disable the high side and enable the low side interrupt. */
            ADC12IER2 &= ~ADC12HIIE;
            ADC12IER2 |= ADC12LOIE;
            ADC12IFGR2 &= ~ADC12LOIFG;
            break;
        case ADC12IV_ADC12LOIFG:                // Vector  8: Window comparator low side
            /* Execute secure wipe with residual power */
            /* Zeroizes SRAM and .secure section */
            ctpl_secureShutdown(CTPL_SHUTDOWN_TIMEOUT_512_MS);
//            ctpl_enterShutdown(CTPL_SHUTDOWN_TIMEOUT_64_MS);

            /* Disable the low side and enable the high side interrupt. */
            ADC12IER2 &= ~ADC12LOIE;
            ADC12IER2 |= ADC12HIIE;
            ADC12IFGR2 &= ~ADC12HIIFG;
            break;
        default: break;
    }
}

void __attribute__ ((interrupt(DMA_VECTOR))) DMA_ISR (void)

{
  switch(__even_in_range(DMAIV,16))
  {
    case 0: break;
    case 2:                                 // DMA0IFG = DMA Channel 0
        __bic_SR_register_on_exit(LPM1_bits );
      break;
    case 4: break;                          // DMA1IFG = DMA Channel 1
    case 6: break;                          // DMA2IFG = DMA Channel 2
    case 8: break;                          // DMA3IFG = DMA Channel 3
    case 10: break;                         // DMA4IFG = DMA Channel 4
    case 12: break;                         // DMA5IFG = DMA Channel 5
    case 14: break;                         // DMA6IFG = DMA Channel 6
    case 16: break;                         // DMA7IFG = DMA Channel 7
    default: break;
  }
}

