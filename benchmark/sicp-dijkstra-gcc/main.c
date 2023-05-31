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
    initialise_benchmark();

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
#define NUM_NODES                          10
#define NONE                               9999

struct _NODE
{
   int iDist;
   int iPrev;
};
typedef struct _NODE NODE;

struct _QITEM
{
   int iNode;
   int iDist;
   int iPrev;
   struct _QITEM *qNext;
};
typedef struct _QITEM QITEM;

#ifdef NONSECURE_BENCH
__attribute__ ((section (".nonsecure")))
#else
__attribute__ ((section (".secure")))
#endif
QITEM *qHead = NULL;

#ifdef NONSECURE_BENCH
__attribute__ ((section (".nonsecure")))
#else
__attribute__ ((section (".secure")))
#endif
int AdjMatrix[NUM_NODES][NUM_NODES] = {{32,  32,  54,  12,  52,  56,  8,   30,  44,  94},
   {76,  54,  65,  14,  89,  69,  4,   16,  24,  47},
   {38,  31,  75,  40,  61,  21,  84,  51,  86,  41},
   {80,  16,  53,  14,  94,  29,  77,  99,  16,  29},
   {59,  7,   14,  78,  79,  45,  54,  83,  8,   94},
   {94,  41,  3,   61,  27,  19,  33,  35,  78,  38},
   {3,   55,  41,  76,  49,  68,  83,  23,  67,  15},
   {68,  28,  47,  12,  82,  6,   26,  96,  98,  75},
   {7,   1,   46,  39,  12,  68,  41,  28,  31,  0},
   {82,  97,  72,  61,  39,  48,  11,  99,  38,  49}};


#ifdef NONSECURE_BENCH
__attribute__ ((section (".nonsecure")))
#else
__attribute__ ((section (".secure")))
#endif
int g_qCount = 0;


#ifdef NONSECURE_BENCH
__attribute__ ((section (".nonsecure")))
#else
__attribute__ ((section (".secure")))
#endif
NODE rgnNodes[NUM_NODES];

#ifdef NONSECURE_BENCH
__attribute__ ((section (".nonsecure")))
#else
__attribute__ ((section (".secure")))
#endif
int ch;


#ifdef NONSECURE_BENCH
__attribute__ ((section (".nonsecure")))
#else
__attribute__ ((section (".secure")))
#endif
int iPrev, iNode;

#ifdef NONSECURE_BENCH
__attribute__ ((section (".nonsecure")))
#else
__attribute__ ((section (".secure")))
#endif
int i, iCost, iDist;


/* BEEBS heap is just an array */

//#define QHEAP_SIZE (8192 / sizeof (QITEM))
#define QHEAP_SIZE (4096 / sizeof (QITEM))
static QITEM qheap[QHEAP_SIZE];
static QITEM *qfree_list;

/* Initialize the BEEBS heap.

   Because we know the size of objects is always the same, we have a
   specially hacked version of malloc */

static void qinit_heap (void)
{
  qfree_list = NULL;
  int  i;

  for (i = 0; i < QHEAP_SIZE; i++)
    {
      qheap[i].qNext = qfree_list;
      qfree_list = &(qheap[i]);
    }
}

/* BEEBS version of malloc.

   This is primarily to reduce library and OS dependencies. This is unusual
   for an embedded program in its dynamic use of malloc and free.  However we
   know that malloc is always used with the same sized object, so we cheat. */

static void * qmalloc_beebs (size_t size)
{
  if ((size != sizeof(QITEM)) || (0 == size))
    return NULL;

  QITEM *new_ptr = qfree_list;

  if (new_ptr != NULL)
    {
      qfree_list = new_ptr->qNext;
    }

  return (void *) new_ptr;
}

/* BEEBS version of free.

   Even our simplified version has to work correctly, or we'll run out of
   memory. Assume we are given a valid pointer to a QITEM - there is no way
   to check. */

static void qfree_beebs (void *ptr)
{
  QITEM *qptr = (QITEM *) ptr;

  qptr->qNext = qfree_list;
  qfree_list = qptr;
}

void enqueue (int iNode, int iDist, int iPrev)
{
   QITEM *qNew = (QITEM *) qmalloc_beebs(sizeof(QITEM));
   QITEM *qLast = qHead;

   qNew->iNode = iNode;
   qNew->iDist = iDist;
   qNew->iPrev = iPrev;
   qNew->qNext = NULL;

   if (!qLast)
   {
      qHead = qNew;
   }
   else
   {
      while (qLast->qNext) qLast = qLast->qNext;
      qLast->qNext = qNew;
   }
   g_qCount++;
}
QITEM* dequeue (int *piNode, int *piDist, int *piPrev)
{
   QITEM *qKill = qHead;

   if (qHead != 0)
   {
      *piNode = qHead->iNode;
      *piDist = qHead->iDist;
      *piPrev = qHead->iPrev;
      qHead = qHead->qNext;
      g_qCount--;
      return qKill;
   }
   return 0;
}
int qcount (void)
{
   return(g_qCount);
}
int dijkstra(int chStart, int chEnd)
{
   for (ch = 0; ch < NUM_NODES; ch++)
   {
      rgnNodes[ch].iDist = NONE;
      rgnNodes[ch].iPrev = NONE;
   }

   if (chStart == chEnd)
   {
      return 0;
   }
   else
   {
      rgnNodes[chStart].iDist = 0;
      rgnNodes[chStart].iPrev = NONE;

      enqueue (chStart, 0, NONE);

      while (qcount() > 0)
      {
         QITEM *tmp = dequeue (&iNode, &iDist, &iPrev);
         if(tmp != 0)
            qfree_beebs(tmp);
         for (i = 0; i < NUM_NODES; i++)
         {
            iCost = AdjMatrix[iNode][i];
            if (iCost != NONE)
            {
               if ((NONE == rgnNodes[i].iDist) ||
                     (rgnNodes[i].iDist > (iCost + iDist)))
               {
                  rgnNodes[i].iDist = iDist + iCost;
                  rgnNodes[i].iPrev = iNode;
                  enqueue (i, iDist + iCost, iNode);
               }
            }
         }
      }
   }

   return rgnNodes[chEnd].iDist;
}

int output[NUM_NODES * NUM_NODES];
int output_count = 0;

void initialise_benchmark (void)
{
  qinit_heap ();        /* Set up the BEEBS QITEM heap */
}



int benchmark() {
   int i,j;

   output_count = 0;

   /* finds 10 shortest paths between nodes */
   for(j = 0; j < NUM_NODES; j++) {
      for (i=0; i < NUM_NODES; i++) {
         output[output_count] = dijkstra(i,j);
         output_count++;
      }
   }

   return 0;
}

int verify_benchmark(int unused) {
   int expected[] = {0, 7, 38, 23, 14, 36, 3, 29, 7, 14, 28, 0, 31, 16, 7, 34,
   31, 28, 1, 39, 39, 25, 0, 32, 14, 3, 32, 9, 26, 43, 12, 14, 40, 0, 21, 43,
   15, 12, 15, 26, 40, 36, 48, 28, 0, 27, 43, 33, 12, 39, 36, 22, 21, 29, 29,
   0, 29, 6, 23, 40, 8, 4, 35, 20, 11, 33, 0, 26, 5, 11, 30, 16, 47, 32, 23,
   35, 23, 0, 17, 34, 28, 24, 55, 16, 8, 35, 31, 28, 0, 38, 23, 19, 41, 16, 8,
   35, 15, 28, 0, 0};
   int i;
   for (i=0; i<NUM_NODES*NUM_NODES; i++) {
      if (output[i] != expected[i]) {
         return 0;
      }
//      printf("%d, ", output[i]);
   }
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

