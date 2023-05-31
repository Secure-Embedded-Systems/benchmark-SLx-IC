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

//#include "aes.h"
//#include "modes.h"

#include "sglib.h"

/* ctpl defines */
#define ADC_MONITOR_THRESHOLD   3.0
#define ADC_MONITOR_FREQUENCY   1000

#define MCLK_FREQUENCY          1000000
#define SMCLK_FREQUENCY         1000000

/* test code defines */
#define SEC_MEM_TEST_SIZE       0x40
#define SEC_TIMER_RESULTS_LEN   32

/* AES module defines */
#define INPUT_LENGTH            128

//#define NONSECURE_BENCH //Comment to place benchmark variables in secure memory


void initGpio(void);
void initClocks(void);
void initAdcMonitor(void);

int main(void)
{
    volatile uint8_t ret_val = 200;
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
//  while ((P5IN & BIT6) == BIT6) {}
    P1OUT &= ~BIT1;
    P1OUT &= ~BIT0;

    /* Blink LED when ready for power loss */
    /* Test EAX chaining */
    i = 0;
    P1OUT |= BIT0;
    P7OUT |= BIT1;

//    //EAX test vector 1
////    uint8_t key[16] = {0x23,0x39,0x52,0xDE,0xE4,0xD5,0xED,0x5F,0x9B,0x9C,0x6D,0x6F,0xF8,0x0F,0xF4,0x78};
////    uint8_t nonce[16] = {0x62,0xEC,0x67,0xF9,0xC3,0xA4,0xA4,0x07,0xFC,0xB2,0xA8,0xC4,0x90,0x31,0xA8,0xB3};
////    uint8_t header[8] = {0x6B,0xFB,0x91,0x4F,0xD0,0x7E,0xAE,0x6B};
////    uint8_t msg[16] = {};
    //EAX test vector 2
//    uint8_t key[16] = {0x7C,0x77,0xD6,0xE8,0x13,0xBE,0xD5,0xAC,0x98,0xBA,0xA4,0x17,0x47,0x7A,0x2E,0x7D};
//    uint8_t nonce[16] = {0x1A,0x8C,0x98,0xDC,0xD7,0x3D,0x38,0x39,0x3B,0x2B,0xF1,0x56,0x9D,0xEE,0xFC,0x19};
//    uint8_t header[8] = {0x65,0xD2,0x01,0x79,0x90,0xD6,0x25,0x28};
//    uint8_t msg[16] = {0x8B,0x0A,0x79,0x30,0x6C,0x9C,0xE7,0xED,0x99,0xDA,0xE4,0xF8,0x7F,0x8D,0xD6,0x16,0x36};

    //AEAD test vector 3
    uint8_t key[16] = {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F};
    uint8_t nonce[16] = {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F};
    uint8_t header[16] = {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F};
    uint8_t msg[16] = {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F};


    uint8_t cipher[32] = {};
    uint8_t plain[16] = {};
    uint8_t tag[16] = {};
    cf_aes_context ctx;
    cf_aes_init(&ctx, key, sizeof(key));
    cf_eax_encrypt(&cf_aes, &ctx,
                   msg, 16,
                   header, 8,
                   nonce, 16,
                   cipher,
                   tag, 16);

   cf_eax_decrypt(&cf_aes, &ctx,
                   cipher, 16,
                   header, 8,
                   nonce, 16,
                   tag, 16,
                   plain);


  while (1) {

      benchmark();
      P1OUT &= ~BIT0;
      P7OUT &= ~BIT1;
//      for(i=0; i<10;i++)
      {
          /* Function to be benchmarked */
//          benchmark();
//          ctpl_checkpointOnly();
          sic_refresh();

          sic_restore();

//          ctpl_secureCheckpoint();
         //      ctpl_checkpointOnly();
      }


      P7OUT |= BIT1;
      P1OUT |= BIT0;
      __delay_cycles(MCLK_FREQUENCY/10);

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
    CSCTL1 = DCOFSEL_3; //_3: 8MHz, _1: 1MHz
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


///*Benchmark var and functions*/
//#ifdef NONSECURE_BENCH
//__attribute__ ((section (".nonsecure")))
//#else
//__attribute__ ((section (".secure")))
//#endif
//int array[100] = {14, 66, 12, 41, 86, 69, 19, 77, 68, 38, 26, 42, 37, 23, 17, 29, 55, 13,
//  90, 92, 76, 99, 10, 54, 57, 83, 40, 44, 75, 33, 24, 28, 80, 18, 78, 32, 93, 89, 52, 11,
//  21, 96, 50, 15, 48, 63, 87, 20, 8, 85, 43, 16, 94, 88, 53, 84, 74, 91, 67, 36, 95, 61,
//  64, 5, 30, 82, 72, 46, 59, 9, 7, 3, 39, 31, 4, 73, 70, 60, 58, 81, 56, 51, 45, 1, 6, 49,
//  27, 47, 34, 35, 62, 97, 2, 79, 98, 25, 22, 65, 71, 0};
//void benchmark()
//{
//        array[0]++;
//}

//Benchmark functions
#define FLT_UWORD_IS_NAN(x) ((x) > 0x7f800000L)
#define FLT_UWORD_IS_INFINITE(x) ((x) == 0x7f800000L)
#define FLT_UWORD_LOG_MAX 0x42b17217
#define FLT_UWORD_LOG_MIN 0x42cff1b5

/* A union which permits us to convert between a float and a 32 bit
   int.  */

typedef union {
  float value;
  uint32_t word;
} ieee_float_shape_type;

/* Get a 32 bit int from a float.  */

#define GET_FLOAT_WORD(i, d)                                                   \
  ieee_float_shape_type gf_u;                                                  \
  gf_u.value = (d);                                                            \
  (i) = gf_u.word;

/* Set a float from a 32 bit int.  */

#define SET_FLOAT_WORD(d, i)                                                   \
  ieee_float_shape_type sf_u;                                                  \
  sf_u.word = (i);                                                             \
  (d) = sf_u.value;

#ifdef NONSECURE_BENCH
__attribute__ ((section (".nonsecure")))
#else
__attribute__ ((section (".secure")))
#endif
static const float one = 1.0,
                   halF[2] =
                       {
                           0.5,
                           -0.5,
},
                   huge = 1.0e+30,
                   twom100 = 7.8886090522e-31, /* 2**-100=0x0d800000 */
    ln2HI[2] =
        {
            6.9313812256e-01, /* 0x3f317180 */
            -6.9313812256e-01,
}, /* 0xbf317180 */
    ln2LO[2] =
        {
            9.0580006145e-06, /* 0x3717f7d1 */
            -9.0580006145e-06,
},                             /* 0xb717f7d1 */
    invln2 = 1.4426950216e+00, /* 0x3fb8aa3b */
    P1 = 1.6666667163e-01,     /* 0x3e2aaaab */
    P2 = -2.7777778450e-03,    /* 0xbb360b61 */
    P3 = 6.6137559770e-05,     /* 0x388ab355 */
    P4 = -1.6533901999e-06,    /* 0xb5ddea0e */
    P5 = 4.1381369442e-08;     /* 0x3331bb4c */

float __ieee754_expf(float x) /* default IEEE double exp */
{
  float y, hi, lo, c, t;
  int32_t k = 0, xsb, sx;
  uint32_t hx;

  GET_FLOAT_WORD(sx, x);
  xsb = (sx >> 31) & 1; /* sign bit of x */
  hx = sx & 0x7fffffff; /* high word of |x| */

  /* filter out non-finite argument */
  if (FLT_UWORD_IS_NAN(hx))
    return x + x; /* NaN */
  if (FLT_UWORD_IS_INFINITE(hx))
    return (xsb == 0) ? x : 0.0; /* exp(+-inf)={inf,0} */
  if (sx > FLT_UWORD_LOG_MAX)
    return huge * huge; /* overflow */
  if (sx < 0 && hx > FLT_UWORD_LOG_MIN)
    return twom100 * twom100; /* underflow */

  /* argument reduction */
  if (hx > 0x3eb17218) {   /* if  |x| > 0.5 ln2 */
    if (hx < 0x3F851592) { /* and |x| < 1.5 ln2 */
      hi = x - ln2HI[xsb];
      lo = ln2LO[xsb];
      k = 1 - xsb - xsb;
    } else {
      k = invln2 * x + halF[xsb];
      t = k;
      hi = x - t * ln2HI[0]; /* t*ln2HI is exact here */
      lo = t * ln2LO[0];
    }
    x = hi - lo;
  } else if (hx < 0x31800000) { /* when |x|<2**-28 */
    if (huge + x > one)
      return one + x; /* trigger inexact */
  }

  /* x is now in primary range */
  t = x * x;
  c = x - t * (P1 + t * (P2 + t * (P3 + t * (P4 + t * P5))));
  if (k == 0)
    return one - ((x * c) / (c - (float)2.0) - x);
  else
    y = one - ((lo - (x * c) / ((float)2.0 - c)) - hi);
  if (k >= -125) {
    uint32_t hy;
    GET_FLOAT_WORD(hy, y);
    SET_FLOAT_WORD(y, hy + (k << 23)); /* add k to y's exponent */
    return y;
  } else {
    uint32_t hy;
    GET_FLOAT_WORD(hy, y);
    SET_FLOAT_WORD(y, hy + ((k + 100) << 23)); /* add k to y's exponent */
    return y * twom100;
  }
}

/* This scale factor will be changed to equalise the runtime of the
   benchmarks. */
#define SCALE_FACTOR (REPEAT_FACTOR >> 0)

/* Tell the compiler not to optimize out calls in BENCHMARK. */
//#ifdef NONSECURE_BENCH
//__attribute__ ((section (".nonsecure")))
//#else
//__attribute__ ((section (".secure")))
//#endif
volatile float result = 0;

/* This benchmark does not support verification */

int verify_benchmark(int res __attribute((unused))) { return -1; }

void initialise_benchmark(void) {}

 int benchmark(void) {
  result = __ieee754_expf(1);
  result = __ieee754_expf(2);
  result = __ieee754_expf(3);
  result = __ieee754_expf(4);
  result = __ieee754_expf(5);
  return 0;
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

