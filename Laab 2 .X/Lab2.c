#include <xc.h>
#include "Configuration.h"
#define _XTAL_FREQ 64000000
#include <stdlib.h>

const unsigned char dice[6] = {
    
    0x08,
    0x22,
    0x2A,
    0x55,
    0x5D,
    0x77,
};

void main(void) {
    
    unsigned char d1; //dice1
    unsigned char d2; //dice2
    
    OSCCON = 0b01110000;
    OSCTUNE= 0b01000000;
    
    ANSELBbits.ANSB4 =0; //digital
    TRISBbits.TRISB4 =1; //input
    
   ANSELC = 0;
   ANSELD = 0;
   TRISC = 0;
   TRISD = 0;
   
          LATC = 0;
          LATD = 0;
while (1)  {  
     if (PORTBbits.RB4 == 0) //button pressed 
      {
          d1 = (rand() % 6);
          d2 = (rand() % 6);

          LATC = dice[d1];
          LATD = dice[d2];
          
          __delay_ms(3000);
          
          LATC = 0;
          LATD = 0;
     }
}
}