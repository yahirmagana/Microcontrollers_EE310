// --- Pin Definitions ---
// Inputs (Active High)
#define PR1_PIN         PORTCbits.RC2
#define PR2_PIN         PORTCbits.RC3
#define SELECT_BTN      PORTCbits.RC5
#define EMERGENCY_BTN   PORTBbits.RB1

// Outputs (Active High)
#define RELAY_PIN       LATCbits.LATC4
#define RELAY_CLOSE_PIN LATBbits.LATB0
#define BUZZER_PIN      LATBbits.LATB2
#define STATE_LED       LATDbits.LATD4
#define DISPLAY_PORT    LATA

// --- Constants ---
#define SECRET_PR1 3
#define SECRET_PR2 2

// --- Helper Functions ---
void init_system() {
    OSCCON1 = 0x60; // Set to HFINTOSC
    OSCFRQ = 0x08;  // Set frequency to 64MHz

    // Disable Analog features on all used ports
    ANSELA = 0x00;
    ANSELB = 0x00;
    ANSELC = 0x00;
    ANSELD = 0x00;

    // Set Input/Output Directions (0 = Output, 1 = Input)
    TRISA = 0x80; // RA0-RA6 outputs (7-seg)
    TRISB = 0x02; // RB1 input (Emergency), RB2 output (Buzzer)
    TRISC = 0x2C; // RC2(PR1), RC3(PR2), RC5(Select) inputs; RC4(Relay) output
    TRISDbits.TRISD4 = 0; // RD4 output (State LED)

    // Initialize Outputs to LOW
    LATA = 0x00;
    LATB = 0x00;
    LATC = 0x00;
    LATD = 0x00;

    // Setup Interrupt-on-Change (IOC) for Emergency Button
    IOCBPbits.IOCBP1 = 1;  // Trigger interrupt on Positive edge (Active High)
    PIE0bits.IOCIE = 1;    // Enable IOC interrupts globally
    
    INTCON0bits.IPEN = 0; 
    INTCON0bits.GIE = 1;  
}
