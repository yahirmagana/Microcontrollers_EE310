// 7-segment hex codes for numbers 0 to 3 (Common Cathode / Active High)
const uint8_t SEGMENT_MAP[4] = {
    0x3F, // 0
    0x06, // 1
    0x5B, // 2
    0x4F  // 3
};

// --- Global Variables ---
volatile uint8_t emergency_triggered = 0;

// --- Interrupt Service Routine ---
void __interrupt(irq(default)) ISR(void) {
    // Check if Interrupt-on-Change flag for RB1 is set (Emergency Button)
    if (PIE0bits.IOCIE && PIR0bits.IOCIF && IOCBFbits.IOCBF1) {
        IOCBFbits.IOCBF1 = 0; // Clear the specific pin flag
        PIR0bits.IOCIF = 0;   // Clear the global IOC flag
        emergency_triggered = 1; 
    }
}

void trigger_error_reset(uint8_t *state, uint8_t *count1, uint8_t *count2) {
    BUZZER_PIN = 1;
    RELAY_PIN = 0; 
    // State indicator remains ON because the system is still running
    
    // Wait 2 seconds, checking emergency flag
    for(int i = 0; i < 20; i++) {
        __delay_ms(100);
        if (emergency_triggered) break;
    }
    
    BUZZER_PIN = 0;
    
    // Reset system logic
    *state = 0;
    *count1 = 0;
    *count2 = 0;
    DISPLAY_PORT = SEGMENT_MAP[0];
}

void play_emergency_melody() {
    RELAY_PIN = 0; 
    // State indicator remains ON because the system is still powered/running
    DISPLAY_PORT = 0x00; 
    int count = 0;
    // Complete system halt loop
    while(count < 1000) {
        BUZZER_PIN = 1;
        __delay_ms(150);
        BUZZER_PIN = 0;
        __delay_ms(50);
        BUZZER_PIN = 1;
        __delay_ms(150);
        BUZZER_PIN = 0;
        __delay_ms(400);
        count++;
    }
}
