/*******************************************************************************
 * File:   main.c
 * Author: Yahir Magana
 * Date:   May 2026
 *
 * Description: 
 * This program interfaces a PIC18F47K42 microcontroller with an analog 
 * accelerometer, a 16x2 LCD, and a digital input button/sensor. 
 * * Features:
 * 1. Reads analog voltage from an IMU on pin RA1.
 * 2. Converts voltage to acceleration (m/s^2) assuming a 3.3V, 300mV/g sensor.
 * 3. Displays the current tilt state (Flat, Tilt Left, Tilt Right, Shake) 
 * and acceleration on the LCD.
 * 4. Uses an Interrupt-On-Change (IOC) on pin RC1 to detect a digital signal,
 * which triggers an LED blink sequence on pin RE0.
 ******************************************************************************/

#include <xc.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h> // Required for abs()

// --- Configuration Bits Setup ---
#pragma config FEXTOSC = OFF    // External Oscillator not enabled
#pragma config RSTOSC = HFINTOSC_1MHZ // Boot up using High-Frequency Internal Osc
#pragma config WDTE = OFF       // WDT Disabled
#pragma config MCLRE = EXTMCLR  // MCLR pin is active
#pragma config MVECEN = OFF     // Multi-vector interrupts disabled (Legacy mode)

// Set the compiler delay frequency to 4 MHz for __delay_ms()
#define _XTAL_FREQ 4000000

// --- LCD Pin Definitions ---
#define RS LATDbits.LATD0       // Register Select on RD0
#define EN LATDbits.LATD1       // Enable on RD1
#define LCD_DATA LATB           // 8-bit Data Bus on PORTB

// --- Shake Detection Thresholds ---
#define SHAKE_THRESHOLD 300  // ADC tick difference required to count as a "jolt"
#define SHAKE_COUNT_MIN 3    // Number of jolts needed to declare a "shake"
#define RESET_TIME_MS 500    // Window of time to count jolts before resetting

// --- Function Prototypes ---
void LCD_Command(unsigned char cmd);
void LCD_Char(unsigned char dat);
void LCD_Init(void);
void LCD_String(const char *msg);
void ADC_Init(void);
void IOC_RC1_Init(void);
void Interrupt_Pins_Init(void);
void __interrupt() Interrupt_Handler(void);
uint16_t ADC_Read(void);

// ============================================================================
// Interrupt Service Routine (ISR)
// ============================================================================
void __interrupt() Interrupt_Handler(void) {
    
    // 1. Check if the interrupt was caused by the IOC peripheral
    if (PIR0bits.IOCIF == 1) {
        
        // 2. Check if specifically pin RC1 triggered it
        if (IOCCFbits.IOCCF1 == 1) {
            int count = 0;
            
            // Blink the LED on RE0 25 times (On/Off = 2 counts)
            while (count < 25){
                PORTEbits.RE0 = 1; // Turn LED ON
                __delay_ms(100);
                count++;
                
                PORTEbits.RE0 = 0; // Turn LED OFF
                __delay_ms(100);
                count++;
            }
       
            // 3. CLEAR THE FLAG (CRITICAL to prevent infinite interrupt loops)
            IOCCFbits.IOCCF1 = 0; 
        }
    }
}

// ============================================================================
// Initialization Functions
// ============================================================================
void IOC_RC1_Init(void) {
    // 1. Configure RC1 as a Digital Input for the interrupt
    TRISCbits.TRISC1 = 1;     // Set RC1 as input
    ANSELCbits.ANSELC1 = 0;   // CRITICAL: Set RC1 as DIGITAL
    
    // Configure RE0 as a Digital Output for the indicator LED
    TRISEbits.TRISE0 = 0;     // Set RE0 as output
    LATEbits.LATE0 = 0;       // Initialize RE0 low (OFF)
    
    // 2. Select the Trigger Edge for RC1
    IOCCPbits.IOCCP1 = 1;     // Enable Positive edge trigger (Low to High)
    // IOCCNbits.IOCCN1 = 1;  // Uncomment if you also want Negative edge

    // 3. Clear the specific RC1 flag before turning it on
    IOCCFbits.IOCCF1 = 0;     

    // 4. Enable the Interrupts
    PIE0bits.IOCIE = 1;       // Enable the master IOC peripheral
    INTCON0bits.GIE = 1;      // Enable Global Interrupts
}

void Interrupt_Pins_Init(void){
    // Redundant init function (handled in IOC_RC1_Init)
    TRISCbits.TRISC1 = 1;
    ANSELC1 = 1; // Note: For IOC, this should actually be 0 (Digital)
}

// ============================================================================
// Main Program
// ============================================================================
void main(void) {
    // Variables for ADC and math
    uint16_t adc_raw_value;
    float voltage;
    float acc;
    char display_buffer[17]; // 16 characters + null terminator

    // --- 1. Configure Global I/O Default States ---
    ANSELB = 0x00;          // Set entire PORTB as digital (for LCD data)
    ANSELDbits.ANSELD0 = 0; // Set RD0 as digital (for LCD RS)
    ANSELDbits.ANSELD1 = 0; // Set RD1 as digital (for LCD EN)
    
    TRISB = 0x00;           // Set entire PORTB as output
    TRISDbits.TRISD0 = 0;   // Set RD0 as output (RS)
    TRISDbits.TRISD1 = 0;   // Set RD1 as output (EN)
    
    LATB = 0x00;            // Initialize PORTB low
    RS = 0;
    EN = 0;

    // --- 2. Initialize Peripherals ---
    LCD_Init();             
    ADC_Init();
    IOC_RC1_Init();
            
    // Variables for shake detection logic
    int16_t current_reading = 0;
    int16_t previous_reading = ADC_Read();
    int16_t delta = 0;
    uint8_t jolt_counter = 0;
    uint16_t time_elapsed = 0;

    // --- 3. Infinite Loop for Real-Time Reading ---
    while(1) {
        
        // --- SHAKE DETECTION BLOCK ---
        current_reading = ADC_Read();
        delta = abs(current_reading - previous_reading); // Calc voltage swing

        if (delta > SHAKE_THRESHOLD) {
            jolt_counter++; // Movement was violent enough to count as a jolt
        }

        if (jolt_counter >= SHAKE_COUNT_MIN) {
            LCD_Command(0x01); // Clear screen
            LCD_String("Shaking");
            
            jolt_counter = 0;  // Reset counter
            __delay_ms(1000);  // Pause to display the message
            LCD_Command(0x01); // Clear screen again
        }

        // Manage the time window for shake detection
        time_elapsed += 20;    // We are looping every 20ms
        if (time_elapsed >= RESET_TIME_MS) {
            jolt_counter = 0;  // Time window expired, reset the count
            time_elapsed = 0;
        }
        previous_reading = current_reading; // Save state for next loop

        __delay_ms(20); // Base loop delay
        
        // --- ACCELEROMETER READING & DISPLAY BLOCK ---
        
        // Read the 12-bit digital value from RA1 (ADC configured to RA1)
        adc_raw_value = ADC_Read();
        
        // Convert to voltage (assuming 12-bit resolution and 3.3V reference)
        voltage = (float)adc_raw_value * 3.3 / 4096.0;
        
        // Convert voltage to acceleration (Assuming 1.65V bias, 300mV/g sensitivity)
        acc = 9.80665 * ((voltage - 1.65) / 0.3);

        // State 1: Sensor is Flat (Voltage between 1.6V and 1.7V)
        if (voltage < 1.7 && voltage > 1.6) {
            __delay_ms(20);
            
            // Trap execution here as long as the sensor remains flat
            while(voltage < 1.7 && voltage > 1.6) {
                adc_raw_value = ADC_Read();
                voltage = (float)adc_raw_value * 3.3 / 4096.0;
                acc = 9.80665 * ((voltage - 1.65) / 0.3);

                sprintf(display_buffer, "a = %.3f m/s^2      ", acc);

                LCD_Command(0xC0);      // Line 2
                LCD_String(display_buffer);

                LCD_Command(0x80);      // Line 1
                LCD_String("Flat");
                __delay_ms(100);
            }
        }
        // State 2: Sensor is Tilted Left (Voltage >= 1.7V)
        else if (voltage >= 1.7) {
            __delay_ms(20);
            
            // Trap execution here as long as the sensor remains tilted left
            while(voltage >= 1.7) {
                adc_raw_value = ADC_Read();
                voltage = (float)adc_raw_value * 3.3 / 4096.0;
                acc = 9.80665 * ((voltage - 1.65) / 0.3);
                
                sprintf(display_buffer, "a = %.3f m/s^2      ", acc);

                LCD_Command(0xC0);      // Line 2
                LCD_String(display_buffer);

                LCD_Command(0x80);      // Line 1
                LCD_String("Tilt Left");
                __delay_ms(100);
            }
        }
        // State 3: Sensor is Tilted Right (Voltage < 1.6V)
        else if (voltage < 1.6) {
            __delay_ms(20);
            
            // Trap execution here as long as the sensor remains tilted right
            while(voltage < 1.6) {
                adc_raw_value = ADC_Read();
                voltage = (float)adc_raw_value * 3.3 / 4096.0;
                acc = 9.80665 * ((voltage - 1.65) / 0.3);
                
                sprintf(display_buffer, "a = %.3f m/s^2      ", acc);

                LCD_Command(0xC0);      // Line 2
                LCD_String(display_buffer);

                LCD_Command(0x80);      // Line 1
                LCD_String("Tilt Right");       
                __delay_ms(100); 
            }
        }
        // Fallback State (Should technically be unreachable given the logic above, 
        // but acts as a failsafe to display voltage)
        else {
            adc_raw_value = ADC_Read();
            voltage = (float)adc_raw_value * 3.3 / 4096.0;
            
            sprintf(display_buffer, "V = %.3f V      ", voltage);

            LCD_Command(0xC0);      // Line 2
            LCD_String(display_buffer);

            LCD_Command(0x80);      // Line 1
            LCD_String("Shake");
            
            __delay_ms(200);
        }
        
        LCD_Command(0x01); // Clear display before looping back
    }
}

// ============================================================================
// ADC Function Implementations
// ============================================================================

/**
 * Initializes the Analog-to-Digital Converter (ADC).
 * Configures pin RA1 for analog input and sets up the internal RC oscillator.
 */
void ADC_Init(void) {
    // 1. Force the ADCC into Basic Mode 
    ADCON0 = 0x00;       
    ADCON1 = 0x00;       
    ADCON2 = 0x00;       
    ADCON3 = 0x00;       

    // 2. Configure RA1 as an Analog Input
    TRISAbits.TRISA1 = 1;     // Set RA1 as an input
    ANSELAbits.ANSELA1 = 1;   // Enable analog functionality on RA1

    // 3. Select Channel ANA1 / RA1
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

/**
 * Triggers an ADC conversion on the currently selected channel and 
 * waits for the result.
 * @return 12-bit ADC conversion value
 */
uint16_t ADC_Read(void) {
    // Start conversion
    ADCON0bits.GO = 1;        
    
    // Wait for the conversion to finish (ADCRC ensures it won't hang)
    while (ADCON0bits.GO);    
    
    // Return the 12-bit result combined from High and Low registers
    return ((uint16_t)((ADRESH << 8) | ADRESL)); 
}

// ============================================================================
// LCD Function Implementations
// ============================================================================

/**
 * Sends a command byte to the LCD (e.g., clear screen, move cursor).
 * @param cmd The 8-bit command hex code
 */
void LCD_Command(unsigned char cmd) {
    RS = 0;             // Select Command Register
    LCD_DATA = cmd;     // Put data on the bus
    EN = 1;             // Pulse Enable pin High
    __delay_us(50);     
    EN = 0;             // Pulse Enable pin Low
    __delay_ms(2);      // Allow LCD time to process
}

/**
 * Sends a single character byte to the LCD to be printed.
 * @param dat The ASCII character to print
 */
void LCD_Char(unsigned char dat) {
    RS = 1;             // Select Data Register
    LCD_DATA = dat;     // Put data on the bus
    EN = 1;             // Pulse Enable pin High
    __delay_us(50);     
    EN = 0;             // Pulse Enable pin Low
    __delay_us(100);    
}

/**
 * Sends the initialization sequence required by standard HD44780 LCDs.
 */
void LCD_Init(void) {
    __delay_ms(20);     // Boot-up delay to let LCD power stabilize
    LCD_Command(0x38);  // Set 8-bit mode, 2 lines, 5x7 font
    LCD_Command(0x0C);  // Display ON, Cursor OFF
    LCD_Command(0x06);  // Auto-increment cursor
    LCD_Command(0x01);  // Clear Display
    __delay_ms(5);      
}

/**
 * Prints an entire string array to the LCD.
 * @param msg Pointer to the character array (string)
 */
void LCD_String(const char *msg) {
    while(*msg) {           // Loop until the null terminator is reached
        LCD_Char(*msg++);   // Print character and increment pointer
    }
}
