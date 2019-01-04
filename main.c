/*
 * File:   main.c
 * Author: siuyin
 *
 * Created on 3 January, 2019, 4:38 PM
 */

// PIC16F886 Configuration Bit Settings

// 'C' source line config statements

// CONFIG1
#pragma config FOSC = INTRC_NOCLKOUT// Oscillator Selection bits (INTOSCIO oscillator: I/O function on RA6/OSC2/CLKOUT pin, I/O function on RA7/OSC1/CLKIN)
#pragma config WDTE = OFF       // Watchdog Timer Enable bit (WDT disabled and can be enabled by SWDTEN bit of the WDTCON register)
#pragma config PWRTE = OFF      // Power-up Timer Enable bit (PWRT disabled)
#pragma config MCLRE = OFF      // RE3/MCLR pin function select bit (RE3/MCLR pin function is digital input, MCLR internally tied to VDD)
#pragma config CP = OFF         // Code Protection bit (Program memory code protection is disabled)
#pragma config CPD = OFF        // Data Code Protection bit (Data memory code protection is disabled)
#pragma config BOREN = OFF      // Brown Out Reset Selection bits (BOR disabled)
#pragma config IESO = OFF       // Internal External Switchover bit (Internal/External Switchover mode is disabled)
#pragma config FCMEN = OFF      // Fail-Safe Clock Monitor Enabled bit (Fail-Safe Clock Monitor is disabled)
#pragma config LVP = OFF        // Low Voltage Programming Enable bit (RB3 pin has digital I/O, HV on MCLR must be used for programming)

// CONFIG2
#pragma config BOR4V = BOR40V   // Brown-out Reset Selection bit (Brown-out Reset set to 4.0V)
#pragma config WRT = OFF        // Flash Program Memory Self Write Enable bits (Write protection off)

// #pragma config statements should precede project file includes.
// Use project enums instead of #define for ON and OFF.


#include <xc.h>
#include <pic16f886.h>

#define _XTAL_FREQ 4000000

volatile unsigned short tick = 0;

void main(void) {

    // initialize port B
    PORTB = 0;
    ANSELH = 0;
    TRISB = 0xFC; // 1111 1100

    nRBPU = 0; // enable weak pull-ups on port B

    // initialize timer 0
    PSA = 0; // assign prescaler to timer 0
    PS0 = 0;
    PS1 = 1; // Fosc/4 * 8 = 8us period
    PS2 = 0; // 1ms = 8us * 125
    T0CS = 0; // timer 0 clock source is internal
    T0IE = 1; // enable timer 0 interrupt on 0xFF -> 0.
    T0IF = 0; // clear timer 0 interrupt flag
    TMR0 = 131; // 256 - 125 (approx 1ms to rollover)

    // enable interrupts
    GIE = 1;
    PEIE = 1;

    // adc config
    TRISA0 = 1; // configure RA0 as an input
    ANS0 = 1; // turn on analog function
    ADON = 1; // turn on the ADC
    ADCS0 = 1; // Fosc/8 ADC clock to 2us conversion an 4MHz Fosc
    ADFM = 1; // 2 most significant bits in ADRESH

    unsigned short toggle_start = tick;
    unsigned short switch_start = tick;
    unsigned char switch_state = 0;
    unsigned char n = 0;
    unsigned char adc_state = 0;
    unsigned short adc_start = tick;
    
    for (;;) {

        // flash LED
        if (tick - toggle_start >= 1000) {
            toggle_start = tick;
            RB0 ^= 1;
            RB1 = ~RB0;
        }
        // debounce switch and toggle LED
        if (tick - switch_start > 5) { // 10 ms check interval
            switch_start = tick;
            switch (switch_state) {
                case 0:
                    if (RB2 == 0) {
                        switch_state = 1; // possible press
                        break;
                    }
                    switch_state = 0;
                    break;
                case 1:
                    if (RB2 == 0) {
                        switch_state = 2; // pressed
                        break;
                    }
                    switch_state = 0; // released
                    break;
                case 2:
                    if (RB2 == 0) {
                        switch_state = 2; // still pressed
                        break;
                    }
                    switch_state = 3; // possible release
                    break;
                case 3:
                    if (RB2 == 0) {
                        switch_state = 2; // pressed
                        break;
                    }
                    switch_state = 0; // released
                    n++;
                    //RB0 = n&1;
                    //RB1 = (n&2)>>1;
                    break;
            }
        }
        // process ADC
        if (tick - adc_start >= 1) {
            adc_start = tick;
            switch (adc_state) {
                case 0:
                    GO = 1; // start ADC
                    adc_state = 1;
                    break;
                case 1:
                    //RB0 = ADRESH & 1;
                    //RB1 = (ADRESH & 2) >> 1;
                    adc_state = 0;
                    break;
            }
        }

    }

}

void __interrupt() isr(void) {
    // 1ms tick timer
    if (TMR0IF) {
        TMR0IF = 0;
        TMR0 = 131;
        tick++;
    }
    return;
}
