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
    //initialise_benchmark();

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
typedef union
{
  float value;
  uint32_t word;
} ieee_float_shape_type;

#define GET_FLOAT_WORD(i,d)                 \
  ieee_float_shape_type gf_u;                   \
  gf_u.value = (d);                     \
  (i) = gf_u.word;

#define SET_FLOAT_WORD(d,i)                 \
  ieee_float_shape_type sf_u;                   \
  sf_u.word = (i);                      \
  (d) = sf_u.value;

#define FLT_UWORD_IS_FINITE(x) ((x)<0x7f800000L)
#define FLT_UWORD_IS_NAN(x) ((x)>0x7f800000L)
#define FLT_UWORD_IS_INFINITE(x) ((x)==0x7f800000L)
#define FLT_UWORD_IS_ZERO(x) ((x)==0)
#define FLT_UWORD_IS_SUBNORMAL(x) ((x)<0x00800000L)

static  const float one = 1.0, tiny=1.0e-30;

    float __ieee754_sqrtf(float x)
{
    float z;
    uint32_t r,hx;
    int32_t ix,s,q,m,t,i;

    GET_FLOAT_WORD(ix,x);
    hx = ix&0x7fffffff;

    /* take care of Inf and NaN */
    if(!FLT_UWORD_IS_FINITE(hx))
        return x*x+x;       /* sqrt(NaN)=NaN, sqrt(+inf)=+inf
                       sqrt(-inf)=sNaN */
    /* take care of zero and -ves */
    if(FLT_UWORD_IS_ZERO(hx)) return x;/* sqrt(+-0) = +-0 */
    if(ix<0) return (x-x)/(x-x);        /* sqrt(-ve) = sNaN */

    /* normalize x */
    m = (ix>>23);
    if(FLT_UWORD_IS_SUBNORMAL(hx)) {        /* subnormal x */
        for(i=0;(ix&0x00800000L)==0;i++) ix<<=1;
        m -= i-1;
    }
    m -= 127;   /* unbias exponent */
    ix = (ix&0x007fffffL)|0x00800000L;
    if(m&1) /* odd m, double x to make it even */
        ix += ix;
    m >>= 1;    /* m = [m/2] */

    /* generate sqrt(x) bit by bit */
    ix += ix;
    q = s = 0;      /* q = sqrt(x) */
    r = 0x01000000L;        /* r = moving bit from right to left */

    while(r!=0) {
        t = s+r;
        if(t<=ix) {
        s    = t+r;
        ix  -= t;
        q   += r;
        }
        ix += ix;
        r>>=1;
    }

    /* use floating add to find out rounding direction */
    if(ix!=0) {
        z = one-tiny; /* trigger inexact flag */
        if (z>=one) {
            z = one+tiny;
        if (z>one)
            q += 2;
        else
            q += (q&1);
        }
    }
    ix = (q>>1)+0x3f000000L;
    ix += (m <<23);
    SET_FLOAT_WORD(z,ix);
    return z;
}

/* This scale factor will be changed to equalise the runtime of the
   benchmarks. */
#define SCALE_FACTOR    (REPEAT_FACTOR >> 0)

/* Tell the compiler not to optimize out calls in BENCHMARK. */
#ifdef NONSECURE_BENCH
__attribute__ ((section (".nonsecure")))
#else
__attribute__ ((section (".secure")))
#endif
volatile float result[6];

#ifdef NONSECURE_BENCH
__attribute__ ((section (".nonsecure")))
#else
__attribute__ ((section (".secure")))
#endif
static int a, b, c, d, e, f;

int benchmark (void)
{
  result[0] = __ieee754_sqrtf(a);
  result[1] = __ieee754_sqrtf(b);
  result[2] = __ieee754_sqrtf(c);
  result[3] = __ieee754_sqrtf(d);
  result[4] = __ieee754_sqrtf(e);
  result[5] = __ieee754_sqrtf(f);
  return 0;
}

void initialise_benchmark() {
  a = 2;
  b = 3;
  c = 5;
  d = 6;
  e = 7;
  f = 8;
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

