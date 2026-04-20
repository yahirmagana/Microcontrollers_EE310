//-----------------------------
// Title: Photoresistor‑Based Combination Lock System (Security Box)
//-----------------------------
// Purpose:
//      This program implements a 2‑stage security lock using two
//      photoresistors (PR1 and PR2) as input sensors. Each sensor
//      increments a counter (0–3) when triggered, and the user
//      switches from PR1 and PR2 using a Select button.
//
//      When both counters match the predefined secret combination
//      (SECRET_PR1, SECRET_PR2), the relay activates for 2 seconds,
//      unlocking the system. An emergency condition triggers a
//      dedicated alert melody and overrides normal operation.
//
// Features:
//      • 7‑segment display output using SEGMENT_MAP
//      • Debounced LDR (photoresistor) transitions
//      • Select button for digit switching
//      • Emergency override with audible alert
//      • Relay‑controlled unlocking mechanism
//      • Error detection and automatic reset
//
// Dependencies:
//      ConfigFile.h
//      initSystem.h
//      helperFunctions.h
//
// Compiler / Environment:
//      MPLAB X IDE, XC8 v6.30 Compiler
//      Target MCU: PIC18F47K42 Curiosity Nano
//
// I/O Summary:
//      INPUTS:
//          – PR1_PIN : Photoresistor 1 digital threshold input
//          – PR2_PIN : Photoresistor 2 digital threshold input
//          – SELECT_BTN : Digit‑change button
//          – emergency_triggered : Global emergency flag
//
//      OUTPUTS:
//          – DISPLAY_PORT : 7‑segment display output
//          – STATE_LED : System status LED
//          – RELAY_PIN : Drive relay
//
// Author: Yahir Magana
//
// Versions:
//      V1.0: 04/19/2026 – Fully functional lock system with emergency mode
//-----------------------------

#include <xc.h>
#include <stdint.h>
#include "ConfigFile.h"
#include "initSystem.h"
#include "helperFunctions.h"

// --- Main Routine ---
void main() {
    init_system();

    uint8_t state = 0;       // 0: Reading PR1, 1: Reading PR2, 2: Unlocked
    uint8_t count1 = 0;
    uint8_t count2 = 0;
    
    uint8_t last_pr1 = 0;
    uint8_t last_pr2 = 0;
    uint8_t last_sel = 0;

    DISPLAY_PORT = SEGMENT_MAP[0]; 
    
    // System is now running, turn the LED ON continuously
    STATE_LED = 1; 

    while(1) {
    if (emergency_triggered) {
            play_emergency_melody(); 
    }
   
   

        // ---------------------------------------------------------
        // STATE 0: Reading Photoresistor 1
        // ---------------------------------------------------------
        if (state == 0) {
            // Read PR1
            uint8_t curr_pr1 = PR1_PIN;
            if (curr_pr1 && !last_pr1) {
                __delay_ms(150); // Increased Debounce for slow LDR transitions
                if (PR1_PIN) {
                    count1++;
                    if (count1 > 3) {
                        count1 = 0; // Wrap around to 0 instead of triggering an error
                    }
                    DISPLAY_PORT = SEGMENT_MAP[count1];
                }
            }
            last_pr1 = curr_pr1;

            // Read Select Button (Switch to PR2)
            uint8_t curr_sel = SELECT_BTN;
            if (curr_sel && !last_sel) {
                __delay_ms(30); // Button Debounce
                    if (SELECT_BTN) {
                        __delay_ms(50); // Small wait to see if the signal is real
                        if (PORTCbits.RC5 == 1) { 
                            state = 1; 
                            DISPLAY_PORT = SEGMENT_MAP[0];
                        } else {
                            state = 0; // It was just noise, clear the flag and continue
                        }
                    }
            }
            last_sel = curr_sel;
        }

        // ---------------------------------------------------------
        // STATE 1: Reading Photoresistor 2
        // ---------------------------------------------------------
        else if (state == 1) {
            // Read PR2
            uint8_t curr_pr2 = PR2_PIN;
            if (curr_pr2 && !last_pr2) {
                __delay_ms(150); // Increased Debounce for slow LDR transitions
                if (PR2_PIN) {
                    count2++;
                    if (count2 > 3) {
                        count2 = 0; // Wrap around to 0 instead of triggering an error
                    }
                    DISPLAY_PORT = SEGMENT_MAP[count2];
                }
            }
            last_pr2 = curr_pr2;

            // Read Select Button (Evaluate the lock)
            uint8_t curr_sel = SELECT_BTN;
            if (curr_sel && !last_sel) {
                __delay_ms(30); // Button Debounce
                if (SELECT_BTN) {
                        __delay_ms(50); // Small wait to see if the signal is real
                        if (PORTCbits.RC5 == 1) { 
                            if (count1 == SECRET_PR1 && count2 == SECRET_PR2) {
                                RELAY_PIN = 1;
                                __delay_ms(20000);
                                RELAY_PIN = 0;
                                state = 2;
                            }
                            else {
                                trigger_error_reset(&state, &count1, &count2);
                        }
                    }
                }
            }
            last_sel = curr_sel;
        }

        // ---------------------------------------------------------
        // STATE 2: Unlocked
        // ---------------------------------------------------------
        else if (state == 2) {
            // System stays unlocked. Relay is driven high.
            // Power cycle or emergency button will break this state.
        }
    }
}
