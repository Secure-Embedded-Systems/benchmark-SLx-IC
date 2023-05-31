/* --COPYRIGHT--,BSD_EX
 * Copyright (c) 2015, Texas Instruments Incorporated
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
//  MSP430FR5x9x Demo - MPU Write protection violation - Interrupt notification
//
//  Description: The MPU segment boundaries are defined by:
//  Border 1 = 0x6000 [MPUSEGB1 = 0x0600]
//  Border 2 = 0x8000 [MPUSEGB2 = 0x0800]
//  Segment 1 = 0x4400 - 0x5FFF - Execute, Read
//  Segment 2 = 0x6000 - 0x7FFF - Execute, Read
//  Segment 3 = 0x8000 - 0x13FFF - Execute, Read
//  Segment 2 is write protected. Any write to an address in the segment 2 range
//  causes a PUC. The LED toggles after accessing SYS NMI ISR.
//
//  ACLK = n/a, MCLK = SMCLK = default DCO
//
//
//           MSP430FR5994
//         ---------------
//     /|\|               |
//      | |               |
//      --|RST            |
//        |               |
//        |           P1.0|-->LED
//
//   William Goh
//   Texas Instruments Inc.
//   October 2015
//   Built with IAR Embedded Workbench V6.30 & Code Composer Studio V6.1
//******************************************************************************
#include <msp430.h>
#include <driverlib.h>

unsigned char SYSNMIflag = 1;
unsigned int *ptr = 0;
unsigned int Data = 0;

__attribute__ ((section (".ipesignature")))
const uint16_t IPE_signatures[] = {0xAAAA, 0x00D00};

//2. IPE Init data structures definition, reusable for ALL projects
#define IPE_MPUIPLOCK 0x0080
#define IPE_MPUIPENA 0x0040
#define IPE_MPUIPPUC 0x0020
#define IPE_SEGREG(a) (a >> 4)
#define IPE_BIP(a,b,c) (a ^ b ^ c ^ 0xFFFF)
#define IPE_FILLSTRUCT(a,b,c) {a,IPE_SEGREG(b),IPE_SEGREG(c),IPE_BIP(a,IPE_SEGREG(b),IPE_SEGREG(c)))}

typedef struct IPE_Init_Structure {
unsigned int MPUIPC0 ;
unsigned int MPUIPSEGB2 ;
unsigned int MPUIPSEGB1 ;
unsigned int MPUCHECK ;
} IPE_Init_Structure; // this struct should be placed inside IPB1/IPB2 boundaries

// This is the project dependant part
#define IPE_START 0x0D000 // This defines the Start of the IP protected area
#define IPE_END 0x0F000 // This defines the End of the IP protected area

__attribute__ ((section (".ipestruct")))
volatile const IPE_Init_Structure ipe_configStructure = {.MPUIPC0 = 0x00C0, .MPUIPSEGB2 = IPE_END>>4, .MPUIPSEGB1 = IPE_START>>4, .MPUCHECK = (0x0C00 ^ 0x0D000>>4 ^ 0x0F000>>4 ^ 0xFFFF)};


__attribute__ ((section (".ipe_vars")))
uint8_t IPE_encapsulatedCount, IPE_i;

__attribute__ ((section (".ipe_const")))
const uint16_t IPE_encapsulatedKeys[] = {0x0123, 0x4567, 0x89AB, 0xCDEF,
                                         0xAAAA, 0xBBBB, 0xCCCC, 0xDDDD};
volatile uint8_t myArray[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

//_attribute__ ((persistent))
uint8_t otherFramVar, FramVar;


__attribute__ ((section (".ipe_code"))) void IPE_encapsulatedInit(void);
__attribute__ ((section (".ipe_code"))) void IPE_encapsulatedBlink(void);
__attribute__ ((section (".ipe_code"))) void IPE_encapsulatedLEDon(void);
void otherFunction(void);


int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;               // Stop WDT

    // Configure GPIO
    P1DIR |= BIT0;                          // Configure P1.0 for LED

    // Disable the GPIO power-on default high-impedance mode to activate
    // previously configured port settings
    PM5CTL0 &= ~LOCKLPM5;
    IPE_encapsulatedInit();

    otherFramVar = 0;
    FramVar = 28;

    while(1)
    {
        otherFramVar++;        

        myArray[0]++;

        IPE_encapsulatedBlink();
    }

}

//void IPE_encapsulatedLEDon(void)
void otherFunction()
{
    if(FramVar < 100)
        FramVar++;
    else
        FramVar = 1;
    IPE_encapsulatedLEDon();

}

/*
 * IPE_encapsulatedLEDon() turns on both the LEDs
 */

void IPE_encapsulatedLEDon(void)
{
    P1DIR |= (BIT0+BIT1);
    P1OUT |= (BIT0+BIT1);
    __delay_cycles(1000);
    P1OUT&=~(BIT0+BIT1);
    __delay_cycles(1000);
}


void IPE_encapsulatedInit(void)
{
    /*
     * IPE_encapsulatedCount initialized to 1 for 1 blink on first execution of
     * IPE_encapsulatedBlink()
     */
    IPE_encapsulatedCount = 1;
}

void IPE_encapsulatedBlink(void)
{
    /*
     * Initialize GPIOs used by IPE code.
     * Set P4.6 up for the red LED output. Starts out low.
     */
    P1OUT &= ~BIT0;
    P1DIR |= BIT0;

    /*
     * Initialize the timer for setting the delay between red LED toggles
     * 50000/~1000000 = 50ms delay (20 times per second)
     */
    TA0CCTL0 = CCIE;
    TA0CCR0 = 50000;

    /*
     * Loading AES keys from IPE const section
     */
    for(IPE_i = 0; IPE_i < 8; IPE_i++)
        AESAKEY = IPE_encapsulatedKeys[IPE_i];

    /*
     * Red LED toggles for the number of times stored in the
     * IPE_encapsulatedCount variable
     */
    for(IPE_i = 0; IPE_i < IPE_encapsulatedCount; IPE_i++)
    {
        P1OUT |= BIT0;                          //Turn red LED on
		__delay_cycles(200);
        __no_operation();
        P1OUT &= ~BIT0;                         //Turn red LED off
		__delay_cycles(200);
        __no_operation();
    }
//    for(IPE_i = 0; IPE_i < IPE_encapsulatedCount; IPE_i++)
//    {
//        P1OUT |= BIT0;                          //Turn red LED on
//        TA0CTL = TASSEL__SMCLK| MC__UP | TACLR; //Start timer
//        __bis_SR_register(LPM0_bits + GIE);     //Sleep until timer int (50ms)
//        __no_operation();
//        P1OUT &= ~BIT0;                         //Turn red LED off
//        __bis_SR_register(LPM0_bits + GIE);     //Sleep until timer int (50ms)
//        __no_operation();
//        TA0CTL = 0;                             //Stop timer
//    }
//
    /*
     * Increment IP_encapsulatedCount to toggle 1 more time on next call of
     * IPE_encapsulatedBlink(), up to 5 times max
     */
    if(IPE_encapsulatedCount < 5)
        IPE_encapsulatedCount++;
    else
        IPE_encapsulatedCount = 1;

    /*
     * Reset AES module so that keys and settings will be cleared, so it can't
     * be used by any code outside of the IPE area
     */
    AESACTL0 = AESSWRST;

    /*
     * Clear all registers used (IPE best practice to conceal what
     * IPE code does)
     * Clear the timer registers and port registers in this case
     */
    TA0CCTL0 = 0;
    TA0CCR0 = 0;
    TA0CCR1 = 0;
    TA0CCR2 = 0;
    TA0CTL = 0;
    TA0R = 0;
    P1DIR &= ~BIT0;
    P1OUT &= ~BIT0;

    /*
     * Clear General purpose CPU registers R4-R15
     *
     * Note: if passing parameters back to code outside IPE, you may need to
     * preserve some CPU registers used for this. See www.ti.com/lit/pdf/slau132
     * MSP430 C/C++ compiler guide section on How a Function Makes a Call and
     * How a Called Function Responds for more information about the registers
     * that the compiler uses for these parameters, R12-R15. Note that if too
     * many arguments are passed, the stack will be used as well.
     *
     * In this case the function is declared void(void), so there is no issue.
     */
    __asm(" mov.w #0, R4");
    __asm(" mov.w #0, R5");
    __asm(" mov.w #0, R6");
    __asm(" mov.w #0, R7");
    __asm(" mov.w #0, R8");
    __asm(" mov.w #0, R9");
    __asm(" mov.w #0, R10");
    __asm(" mov.w #0, R11");
    __asm(" mov.w #0, R12");
    __asm(" mov.w #0, R13");
    __asm(" mov.w #0, R14");
    __asm(" mov.w #0, R15");

    /*
     * At this point, it may also be desired to clear any RAM that was allocated
     * in the course of the function, because RAM is not cleared on
     * de-allocation. However, this is completely application-dependent and care
     * would need to be taken not to corrupt the stack that is needed by the
     * application for proper functioning. It may be better in cases like this
     * to either:
     * 1. Use no local variables, and therefore only use variables in
     *    FRAM IPE area, to have less concern (though there could still be some
     *    stack usage leaving things in RAM after execution).
     *    This is what is done in this example (see how even the loop counter
     *    IPE_i is placed in IPE rather than a local variable).
     * 2. If there is real concern about what could be placed in RAM, write the
     *    IPE routines in assembly so that the user can completely control RAM
     *    and stack usage.
     */
}

//__attribute__ ((section (".ipe:_isr")))
void __attribute__ ((interrupt(TIMER0_A0_VECTOR))) TIMER0_A0_ISR (void)
{
    __bic_SR_register_on_exit(LPM0_bits); //Wake the device on ISR exit
    __no_operation();
}

