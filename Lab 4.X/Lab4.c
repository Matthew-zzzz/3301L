/*
 * File:   lab4.c
 * Author: Matthew Zalavarria
 *
 * Lab 4 - Traffic Light Controller
 *         (FSM + Safety Mode + Pedestrian Crossing)
 * ECE 3301L - Introduction to Microcontrollers Laboratory
 *
 * Description:
 *   Formal FSM-based traffic light controller with:
 *   - All-Red flashing safety mode (on-board button S2 = RC5)
 *   - Pedestrian crossing with 9-to-0 countdown on a 7-segment display
 *     (external SPST button on RE0)
 *   The pedestrian request is LATCHED and serviced after the EW Yellow
 *   phase. During the countdown, NS has Green and EW has Red.
 *
 * Pin Assignments:
 *   PORTD - South (RD5=G, RD4=Y, RD3=R) and North (RD2=G, RD1=Y, RD0=R)
 *   PORTB - East  (RB5=G, RB4=Y, RB3=R) and West  (RB2=G, RB1=Y, RB0=R)
 *           RB6/RB7 preserved (PGC/PGD for LVP programming)
 *   PORTC - 7-Segment Display (common anode, LOW = ON)
 *           RC0=a, RC1=b, RC2=c, RC3=d, RC4=e, RC6=f, RC7=g
 *           RC5 = On-board button S2 (safety, active-low)
 *   PORTE - RE0 = External pedestrian button (active-low, 10k pull-up)
 *   PORTA - RA4 = On-board LED (pedestrian request indicator)
 *
 * FSM States:
 *   EW_GREEN      - EW Green,  NS Red         (6 s)   -> EW_YELLOW
 *   EW_YELLOW     - EW Yellow, NS Red         (3 s)   -> PED_COUNTDOWN or NS_GREEN
 *   PED_COUNTDOWN - NS Green,  EW Red + 7-seg (10 s)  -> NS_GREEN
 *   NS_GREEN      - NS Green,  EW Red         (6 s)   -> NS_YELLOW
 *   NS_YELLOW     - NS Yellow, EW Red         (3 s)   -> EW_GREEN
 *   ALL_RED_FLASH - All Reds flashing (safety)        -> EW_GREEN on S2 press
 */


#include <xc.h>
#include <stdint.h>
#include "Configuration.h"
 
#define _XTAL_FREQ 16000000UL

#define REDS 0x09 //(start NS RED / EW 0x24)
#define YELLOWS 0x12
#define GREENS 0x24
#define LIGHTS_OFF 0x00

typedef enum {
    EW_GREEN,
    EW_YELLOW,
    NS_GREEN,
    NS_YELLOW,
    PED_COUNTDOWN,
    ALL_RED_FLASH
} TrafficState;

/* 7-Segment Patterns for PORTC (Common Anode: 0=ON, 1=OFF)
 *
 *   Bit:   7  6  5  4  3  2  1  0
 *   Seg:   g  f  -  e  d  c  b  a     (bit 5 = RC5 button, always mask it)
 *
 *        aaa
 *       f   b
 *        ggg
 *       e   c
 *        ddd
 */
static const uint8_t sevenSeg[10] = {
    0x5F, // 0: a,b,c,d,e,f
    0x06, // 1: b,c
    0x9B, // 2: a,b,d,e,g
    0x8F, // 3: a,b,c,d,g
    0xC6, // 4: b,c,f,g
    0xCD, // 5: a,c,d,f,g
    0xDD, // 6: a,c,d,e,f,g
    0x07, // 7: a,b,c
    0xDF, // 8: all segments
    0xCF  // 9: a,b,c,d,f,g
};

#define SEG_BLANK 0x00          /* All segments OFF (preserves RC5 bit) */
static void seg_blank(void)
{
    LATC &= 0x20;
}

/* Pedestrian request flag (latched) */
static uint8_t pedRequest = 0;

static void set_lights(unsigned char rd_val, unsigned char rb_val) {
    // TODO: Write rd_val to LATD and rb_val to LATB, masking so that
    //       only bits 0-5 change. Hint: (LATD & 0xC0) | (rd_val & 0x3F)
    LATD =(LATD & 0xC0) | (rd_val & 0x3F);
    LATB = (LATB & 0xC0) | (rb_val & 0x3F);
}

static void init(void) {
    // TODO: Oscillator - 16 MHz HFINTOSC (IRCF/SCS)
     OSCCON = 0b01110010;
     OSCTUNEbits.PLLEN = 0;
     
    /// TODO: Make PORTA/B/C/D/E digital (ANSELx = 0x00)
   ANSELB = 0;
   ANSELD = 0;
   
   ANSELA = 0;
   ANSELC = 0;
   ANSELE = 0;
 
 // TODO: PORTD RD0-RD5 outputs, PORTB RB0-RB5 outputs (preserve bits 6-7)
   TRISB &= 0xC0;
   TRISD &= 0xC0;
   
   TRISC = 0x20; // seven segment output
   
   TRISEbits.TRISE0 = 1; // for pedestrian button
   LATAbits.LATA4 = 0; //RA4 LED output
   TRISAbits.TRISA4 = 0;
   
   // Blank display
    LATC = (LATC & 0x20) | (SEG_BLANK & 0xDF);
   
    // TODO: Start with all traffic LEDs off (again, preserve bits 6-7).
   set_lights(REDS, REDS);
}


/** Show one digit (0-9) on the 7-segment display, preserving RC5. */
static void seg_show(uint8_t digit)
{
    if (digit <= 9) {
        LATC = (LATC & 0x20) |
               (sevenSeg[digit] & 0xDF);
    }
}


/**
 * Delay in ~10 ms slices while polling the buttons, so a press is never
 * missed during a long state delay.
 * Returns early if the safety button (S2) is pressed.
 */
static uint8_t delay_and_poll(uint16_t ms) {
    // TODO: loop in small __delay_ms(10) steps:
    uint16_t RUN = 0;
    while (RUN < ms) {
    //   - if RE0 pressed -> latch pedRequest = 1 and light RA4
        if (PORTEbits.RE0 == 0) { 
            pedRequest = 1;
            LATAbits.LATA4 = 1;
        }
        //   - if RC5 (S2) pressed -> return 1 immediately (enter safety mode)
    if (PORTCbits.RC5 == 0) {
        return 1;
    }
    __delay_ms(10);
    RUN += 10;
    }
    return 0;
    }


void main(void) {
    
    init(); 
    TrafficState state = EW_GREEN;


    while (1) {
        // TODO: Implement the FSM with a switch(state):
        //
        switch(state) {
        //   EW_GREEN:      lights, delay_and_poll(6000), -> EW_YELLOW
        case EW_GREEN:
            set_lights(REDS, GREENS);
            
            if (delay_and_poll(6000)) {
                state = ALL_RED_FLASH;
            }
            else {
                state = EW_YELLOW;
            }
            break;
            
            
        //   EW_YELLOW:     lights, delay_and_poll(3000),
        //                  -> PED_COUNTDOWN if pedRequest else NS_GREEN
        case EW_YELLOW:
            set_lights(YELLOWS, YELLOWS);
            
            if (delay_and_poll(3000)) {
                state = ALL_RED_FLASH;
            }
            else if (pedRequest) {
                   state = PED_COUNTDOWN;
               }
               else {
                   state = NS_GREEN;
               }
               break;
            
            
            case PED_COUNTDOWN:
            //   PED_COUNTDOWN: NS green / EW red; count 9..0 on the 7-seg,
        //                  1 second per digit; clear pedRequest and RA4;
        //                  blank display; -> NS_GREEN
                set_lights(GREENS, REDS);
                
                for (int8_t digit = 9; digit >= 0; digit--) {
                    seg_show((uint8_t) digit);
                    
                    if (delay_and_poll(1000)) {
                        state = ALL_RED_FLASH;
                        break;
                    }
                }
                
                if (state != ALL_RED_FLASH) {
                    pedRequest = 0;
                    LATAbits.LATA4 = 0;
                    seg_blank();
                    state = NS_GREEN;
                }
                break;
                //   NS_GREEN:      lights, delay_and_poll(6000), -> NS_YELLOW
       case NS_GREEN:
           set_lights(GREENS, REDS);
           
           if (delay_and_poll(6000)) {
               state = ALL_RED_FLASH;
           } 
           else {
               state = NS_YELLOW;
           }
           break;
 //   NS_YELLOW:     lights, delay_and_poll(3000), -> EW_GREEN
       case NS_YELLOW:
       {
           set_lights(YELLOWS, YELLOWS);
           
           if (delay_and_poll(3000)) {
               state = ALL_RED_FLASH;
           }
           else if (pedRequest) {
               state = PED_COUNTDOWN;
           }
           else {
               state = EW_GREEN;
           }
       }
        break;
        
        //   ALL_RED_FLASH: flash both red pairs at ~1 Hz until S2 is
        //                  pressed again, then -> EW_GREEN
        case ALL_RED_FLASH:
            // Wait for the first press to be released
            while (PORTCbits.RC5 == 0) {
                __delay_ms(10);
            }

            // Keep flashing until S2 is pressed again
            while (1) {
                // All reds on
                set_lights(REDS, REDS);

                if (delay_and_poll(500)) {
                    break;
                }

                // All lights off
                set_lights(LIGHTS_OFF, LIGHTS_OFF);

                if (delay_and_poll(500)) {
                    break;
                }
            } // End flashing loop

            // Wait for the second press to be released
            while (PORTCbits.RC5 == 0) {
                __delay_ms(10);
            }

            set_lights(REDS, REDS);
            state = EW_GREEN;
            break;
           //
    
               // Any state: if delay_and_poll() reports S2, go to ALL_RED_FLASH.
      default:
          set_lights(REDS,REDS);
          state = EW_GREEN;
          break;
    }
}
} 