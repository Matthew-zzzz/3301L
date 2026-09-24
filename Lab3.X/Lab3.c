/*
 * File:   lab3.c
 * Author: Matthew Zalavarria
 *
 * Lab 3 - Traffic Light Controller
 * ECE 3301L - Introduction to Microcontrollers Laboratory
 *
 * Pin Assignments:
 *   PORTD - South (RD5=G, RD4=Y, RD3=R) and North (RD2=G, RD1=Y, RD0=R)
 *   PORTB - East  (RB5=G, RB4=Y, RB3=R) and West  (RB2=G, RB1=Y, RB0=R)
 *   RB6/RB7 must be left untouched (PGC/PGD for programming with LVP=ON)
 *
 * Traffic Sequence:
 *   State A: EW Green,  NS Red     - 6 seconds
 *   State B: EW Yellow, NS Red     - 3 seconds
 *   State C: NS Green,  EW Red     - 6 seconds
 *   State D: NS Yellow, EW Red     - 3 seconds
 */

#include <xc.h>
#include <stdint.h>
#include "Configuration.h"
 
#define _XTAL_FREQ 64000000UL

#define REDS 0x09 //(start NS RED / EW 0x24)
#define YELLOWS 0x12
#define GREENS 0x24

static void set_lights(unsigned char rd_val, unsigned char rb_val) {
    // TODO: Write rd_val to LATD and rb_val to LATB, masking so that
    //       only bits 0-5 change. Hint: (LATD & 0xC0) | (rd_val & 0x3F)
    LATD =(LATD & 0xC0) | (rd_val & 0x3F);
    LATB = (LATB & 0xC0) | (rb_val & 0x3F);
}

static void init(void) {
    // TODO: Configure the oscillator for 64 MHz
    //       (16 MHz HFINTOSC via OSCCONbits.IRCF/SCS + 4x PLL, same as Lab 2)
     OSCCON = 0b01110000;
     OSCTUNE = 0b01000000;
     
    // TODO: Make PORTB and PORTD digital (ANSELB / ANSELD).
    //       Critical on the K22: PBADEN=ON makes PORTB analog at reset!
   ANSELB = 0;
   ANSELD = 0;
   
 
    // TODO: Make RB0-RB5 and RD0-RD5 outputs WITHOUT touching bits 6-7.
    //       Hint: TRISB &= 0xC0 clears only the lower six bits.
   TRISB &= 0xC0;
   TRISD &= 0xC0;
   
    // TODO: Start with all traffic LEDs off (again, preserve bits 6-7).
   set_lights(REDS, REDS);
}

/**
 * Update traffic lights using only the lower 6 bits of each port.
 * Upper 2 bits (RB6/RB7, RD6/RD7) must be preserved.
 */


void main(void) {
    init();

    while (1) {
        // TODO: Implement the four-state sequence with set_lights() and
        //       __delay_ms(). Work out the PORTD/PORTB bit patterns for
        //       each state from the pin assignment table above.
        //
        // State A: EW Green,  NS Red  - 6 s


  
        set_lights(REDS, GREENS);
       __delay_ms(3000);
       __delay_ms(3000);
                // State B: EW Yellow, NS Red  - 3 s
        set_lights(YELLOWS, YELLOWS);
        __delay_ms(3000);
                // State C: NS Green,  EW Red  - 6 s
        set_lights(GREENS, REDS);
        __delay_ms(3000);
        __delay_ms(3000);
              // State D: NS Yellow, EW Red  - 3 s
        set_lights(YELLOWS, YELLOWS);
        __delay_ms(3000);
    }
}