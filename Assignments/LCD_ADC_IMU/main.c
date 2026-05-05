//-----------------------------
// Title: Accelerometer-Based Tilt and Shake Detection System
//-----------------------------
// Purpose:
//      This program interfaces a PIC18F47K42 microcontroller with an
//      analog accelerometer (IMU) and a 16x2 LCD. It reads the analog
//      voltage, calculates the physical acceleration in m/s^2, and
//      determines the sensor's physical state (Flat, Tilt Left, 
//      Tilt Right, or Shaking). It also features an Interrupt-On-Change
//      (IOC) digital input that triggers a visual LED alert sequence.
//
// Features:
//      • Analog-to-digital conversion using the internal ADCRC
//      • Math conversion from raw IMU voltage to acceleration (m/s^2)
//      • Differential delta-tracking for physical shake detection
//      • State-trapping loops for continuous LCD tilt state updates
//      • 16x2 character LCD interface (8-bit data bus)
//      • Interrupt-On-Change (IOC) with LED response
//
// Dependencies:
//      <xc.h>
//      <stdint.h>
//      <stdio.h>
//      <stdlib.h>
//
// Compiler / Environment:
//      MPLAB X IDE, XC8 Compiler
//      Target MCU: PIC18F47K42
//
// I/O Summary:
//      INPUTS:
//          – RA1 : Analog input from accelerometer (IMU)
//          – RC1 : Digital input for Interrupt-On-Change trigger
//
//      OUTPUTS:
//          – PORTB (RB0-RB7) : 8-bit Data Bus for LCD
//          – RD0 : LCD Register Select (RS) pin
//          – RD1 : LCD Enable (EN) pin
//          – RE0 : System indicator LED (triggered by interrupt)
//
// Author: Yahir Magana
//
// Versions:
//      V1.0: 05/02/2026 – Fully functional tilt and shake system with IOC
//      V1.1: 05/05/2026 - Fixed shaking precision.  
//-----------------------------

#include <xc.h>
#include <stdint.h>
#include <stdio.h>

// --- Configuration Bits Setup ---
#pragma config FEXTOSC = OFF    // External Oscillator not enabled
#pragma config RSTOSC = HFINTOSC_1MHZ // Boot up using High-Frequency Internal Osc
#pragma config WDTE = OFF       // WDT Disabled
#pragma config MCLRE = EXTMCLR  // MCLR pin is active
#pragma config MVECEN = OFF // Multi-vector interrupts disabled (Legacy mode)

// Set the compiler delay frequency to 4 MHz
#define _XTAL_FREQ 4000000
// --- LCD Pin Definitions ---
#define RS LATDbits.LATD0       // Register Select on RD0
#define EN LATDbits.LATD1       // Enable on RD1
#define LCD_DATA LATB           // 8-bit Data Bus on PORTB
#define SHAKE_THRESHOLD 150  // ADC tick difference required to count as a "jolt"
#define SHAKE_COUNT_MIN 3    // Number of jolts needed to declare a "shake"
#define RESET_TIME_MS 200    // Window of time to count jolts before resetting

// --- Function Prototypes ---
void LCD_Command(unsigned char cmd);
void LCD_Char(unsigned char dat);
void LCD_Init(void);
void LCD_String(const char *msg);
void ADC_Init(void);
void IOC_RC1_Init(void);
void __interrupt() Interrupt_Handler(void);
uint16_t ADC_Read(void);

void __interrupt() Interrupt_Handler(void) {
    
    // 1. Check if the interrupt was caused by the IOC peripheral
    if (PIR0bits.IOCIF == 1) {
        
        // 2. Check if specifically pin RC1 triggered it
        if (IOCCFbits.IOCCF1 == 1) {
            int count = 0;
            while (count < 25){
                PORTEbits.RE0 = 1;
                __delay_ms(100);
                count++;
                PORTEbits.RE0 = 0;
                __delay_ms(100);
                count++;
            }
       
            // 3. CLEAR THE FLAG (CRITICAL)
            IOCCFbits.IOCCF1 = 0; 
        }
    }
}

void IOC_RC1_Init(void) {
    // 1. Configure RC1 as a Digital Input
    TRISCbits.TRISC1 = 1;     // Set RC1 as input
    ANSELCbits.ANSELC1 = 0;   // CRITICAL: Set RC1 as DIGITAL
    TRISEbits.TRISE0 = 0;
    LATEbits.LATE0 = 0;
    
    // 2. Select the Trigger Edge
    IOCCPbits.IOCCP1 = 1;     // Enable Positive edge trigger
    // IOCCNbits.IOCCN1 = 1;  // Uncomment if you also want Negative edge

    // 3. Clear the specific RC1 flag before turning it on
    IOCCFbits.IOCCF1 = 0;     

    // 4. Enable the Interrupts
    PIE0bits.IOCIE = 1;       // Enable the master IOC peripheral
    INTCON0bits.GIE = 1;      // Enable Global Interrupts
}

// --- Main Program ---
void main(void) {
    uint16_t adc_raw_value;
    float voltage;
    float acc;
    char display_buffer[17]; // 16 characters + null terminator

    // 1. Configure Global I/O Default States
    ANSELB = 0x00;          // Set entire PORTB as digital (for LCD data)
    ANSELDbits.ANSELD0 = 0; // Set RD0 as digital (for LCD RS)
    ANSELDbits.ANSELD1 = 0; // Set RD1 as digital (for LCD EN)
    
    TRISB = 0x00;           // Set entire PORTB as output
    TRISDbits.TRISD0 = 0;   // Set RD0 as output (RS)
    TRISDbits.TRISD1 = 0;   // Set RD1 as output (EN)
    
    LATB = 0x00;            // Initialize PORTB low
    RS = 0;
    EN = 0;

    // 2. Initialize Peripherals
    LCD_Init();             
    ADC_Init();
    IOC_RC1_Init();
            
    int16_t current_reading = 0;
    int16_t previous_reading = ADC_Read();
    int16_t delta = 0;
    
    uint8_t jolt_counter = 0;
    uint16_t time_elapsed = 0;

    // 4. Infinite Loop for Real-Time Reading
    while(1) {
        
        // 1. Take a SINGLE new reading for this loop cycle
        current_reading = (int16_t)ADC_Read();

        // 2. Calculate the absolute difference for shake detection
        delta = abs(current_reading - previous_reading);

        // 3. Check if the movement was violent enough
        if (delta > SHAKE_THRESHOLD) {
            jolt_counter++;
            time_elapsed = 0;
        }

        // 4. Manage the time window
        time_elapsed += 20;    // We are looping every 20ms
        if (time_elapsed >= RESET_TIME_MS) {
            jolt_counter = 0;  // Time window expired, reset the count
            time_elapsed = RESET_TIME_MS;
        }

        // 5. Save current reading for the next loop's delta math
        previous_reading = current_reading;

        // 6. Evaluate state and update LCD
        if (jolt_counter >= SHAKE_COUNT_MIN) {
            // --- SHAKING STATE ---
            LCD_Command(0x01); // Clear screen
            LCD_Command(0x80); // Move cursor to Line 1
            LCD_String("Shaking!        ");
            
            jolt_counter = 0;  // Reset counters
            time_elapsed = 0;
            __delay_ms(1000);  // Pause to display the message
            LCD_Command(0x01); // Clear screen before returning to tilt mode
            
            // Re-sync previous reading so the 1-second pause doesn't trigger a false delta
            previous_reading = ADC_Read(); 
            
        } else {
            // --- TILT STATES ---
            // Convert the raw digital value to voltage and acceleration
            voltage = (float)current_reading * 3.3 / 4096.0;
            acc = 9.80665 * ((voltage - 1.65) / 0.3);

            // Format Line 2
            sprintf(display_buffer, "a = %.3f m/s^2  ", acc);
            LCD_Command(0xC0);      
            LCD_String(display_buffer);

            // Format Line 1 based on voltage (padded with spaces to clear old text)
            LCD_Command(0x80);      
            if (voltage < 1.7 && voltage > 1.6){
                LCD_String("Flat          ");
            }
            else if (voltage >= 1.7){
                LCD_String("Tilt Left     ");
            }
            else if (voltage <= 1.6){
                LCD_String("Tilt Right    ");       
            }
        }

        // Loop delay determines sample rate
        __delay_ms(20);
    }
}

void Interrupt_Pins_Init(void){
    TRISCbits.TRISC1 = 1;
    ANSELC1 = 1;
}

// --- ADC Function Implementations ---

void ADC_Init(void) {
    // 1. Force the ADCC into Basic Mode 
    ADCON0 = 0x00;       
    ADCON1 = 0x00;       
    ADCON2 = 0x00;       
    ADCON3 = 0x00;       

    // 2. Configure RA1 as an Analog Input (CHANGED)
    TRISAbits.TRISA1 = 1;     // Set RA1 as an input
    ANSELAbits.ANSELA1 = 1;   // Enable analog functionality on RA1

    // 3. Select Channel ANA1 / RA1 (CHANGED)
    // In the K42 family, ANA0 is 0x00, ANA1 is 0x01, ANA2 is 0x02, etc.
    ADPCH = 0x01;             

    // 4. Use Dedicated Internal ADC RC Oscillator (ADCRC)
    ADCON0bits.CS = 1;

    // 5. Right-justify the 12-bit result
    ADCON0bits.FM = 1;

    // 6. Set Voltage References (VREF+ = VDD, VREF- = GND)
    ADREF = 0x00;

    // 7. Set Acquisition Time
    ADACQ = 0x20;

    // 8. Turn on the ADC
    ADCON0bits.ON = 1;
}

uint16_t ADC_Read(void) {
    // Start conversion
    ADCON0bits.GO = 1;        
    
    // Wait for the conversion to finish
    // Because we are using the ADCRC, this will NEVER freeze.
    while (ADCON0bits.GO);    
    
    // Return the 12-bit result combined from High and Low registers
    return ((uint16_t)((ADRESH << 8) | ADRESL)); 
}

// --- LCD Function Implementations ---

void LCD_Command(unsigned char cmd) {
    RS = 0;             
    LCD_DATA = cmd;     
    EN = 1;             
    __delay_us(50);     
    EN = 0;             
    __delay_ms(2);      
}

void LCD_Char(unsigned char dat) {
    RS = 1;             
    LCD_DATA = dat;     
    EN = 1;             
    __delay_us(50);     
    EN = 0;             
    __delay_us(100);    
}

void LCD_Init(void) {
    __delay_ms(20);     // Boot-up delay
    LCD_Command(0x38);  // 8-bit mode, 2 lines, 5x7 font
    LCD_Command(0x0C);  // Display ON, Cursor OFF
    LCD_Command(0x06);  // Auto-increment cursor
    LCD_Command(0x01);  // Clear Display
    __delay_ms(5);      
}

void LCD_String(const char *msg) {
    while(*msg) {           
        LCD_Char(*msg++);   
    }
}
