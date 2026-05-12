/*
* The purpose of this program is to generate a PWM with different pulse width 
* The output of the PWM will be on RB2
* myLED is connected to RB0 and it toggles very slowly
* In order to change the PULSE period and width you need to do the following: 

 * PWM Period=
 * [T2PR+1]*4*Tosc*PreScale

 * Pulse Width=
 * Tosc*Prescale*CCPR2

 * Duty Cycle Ratio %=
 * CCPR2 / [4*(T2PR+1)]

 * The presale value for the timer is defined in T2CON register. 
 * The ACTUAL value of CCP2 MUST be varied by changing
 * PWM2_INITIALIZE_DUTY_VALUE set to the equivalent decimal value for CCPR2

 * Author: Farid Farahmand
 */




#include <xc.h> // must have this
#include "PWM.h" // must have this
#include "ConfigFile.h" // must have this -  XC8_ConfigFile.h


#define _XTAL_FREQ 4000000      // Fosc  frequency for _delay()  library
#define FCY    _XTAL_FREQ/4     // system clock is 1 usec

#define myLED  PORTBbits.RB0
#define PWM2_INITIALIZE_DUTY_VALUE 100
#define SERVO_MAX_DUTY 120
#define SERVO_MIN_DUTY 20

#define upButton PORTAbits.RA0
#define downButton PORTAbits.RA1


uint16_t checkdutyCycle;
uint16_t current_duty;

char preScale;
_Bool pwmStatus;

void main (void) {
    OSCSTATbits.HFOR =1; // enable  HFINTOSC Oscillator (see clock schematic))
    OSCFRQ=0x02; // 00=1 MHZ, 02=4MHZ internal - see page 106 of data sheet
    
    ANSELB = 0b00000000;    
    TRISB= 0b00000000;//sets PORTB as all outputs 
    PORTB= 0b00000000;//turns off PORTB outputs so that the LED is initially off
    
    ANSELA = 0;    
    TRISA= 0xFF; 
    PORTA= 0x00;
    
    TMR2_Initialize();
    TMR2_StartTimer();        
    
    PWM_Output_D8_Enable();
    PWM2_Initialize();
    PWM2_LoadDutyValue(PWM2_INITIALIZE_DUTY_VALUE ); // initialize CCPR2H/L
   // PWM_Output_D8_Disable();
   // TMR2_StopTimer();  

    // Duty Cycle in percentage 
    checkdutyCycle =(uint16_t)((100UL*PWM2_INITIALIZE_DUTY_VALUE)/(4*(T2PR+1)));
    // binary value of Register T2CON.PRESCALE
    preScale = ((T2CON >> 4) & (0x0F)); 

    while (1) {
        //pwmStatus = PWM2_OutputStatusGet();
        //PORTBbits.RB2 = pwmStatus;
        //T2CON=0xB0; // stop the timer & do what you have to do
        if (upButton){
            __delay_ms(20);
            while (upButton){
                if (current_duty < SERVO_MAX_DUTY) {
                    current_duty++;
                    PWM2_LoadDutyValue(current_duty);
                }
                myLED ^= 1; 
                __delay_ms(15);
            }
        }
        else if (downButton){
            __delay_ms(20);
            while (downButton){
                if (current_duty > SERVO_MIN_DUTY) {
                    current_duty--;
                    PWM2_LoadDutyValue(current_duty);
                }
                
                myLED ^= 1; 
                __delay_ms(15);
            }
        }
    }
}
