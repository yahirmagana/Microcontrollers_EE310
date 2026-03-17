//-----------------------------
// Title: Assignment 4, Temperature Comparator
//-----------------------------
// Purpose: This program compares two given values (refTemp and measuredTemp) 
// and based on if the measuredTemp is lower or higher it turns on PORTD1 or 
// PORTD2 respectively.  
// Dependencies: AssemblyConfig.inc pic18f46k42.inc xc.inc
// Compiler: MPLAB X IDE v6.30
// Author: Yahir Magana 
// OUTPUTS: PORTD1 or PORTD2 turned on 
// INPUTS: measuredTemp and refTemp 
// Versions:
//      V1.0: 03/07/2026 - First version 
//      V1.2: 03/11/2026 - Add hex to decimal converter.
//-----------------------------

;----------------
; PROGRAM INPUTS
;----------------
; Define two constant temperature values
#define  measuredTemp       5 ; this is the input value
#define  refTemp        15 ; this is the input value

;---------------------
; Definitions
;---------------------
; Symbolic names for PORTD bits
#define SWITCH    LATD,2  
#define LED0      PORTD,0
#define LED1      PORTD,1
    
;---------------------
; Program Constants
;---------------------
; Register allocations
REG10   equ     10h   
REG11   equ     11h
REG01   equ     1h
REG20   EQU 0x20
REG21   EQU 0x21
contReg EQU 0x22
REG60   EQU  0x60    
REG61   EQU  0x61    
REG62   EQU  0x62    
REG70   EQU  0x70    
REG71   EQU  0x71    
REG72   EQU  0x72
TEMP    EQU  0x63
   
#include <pic18f46k42.inc>
#include <xc.inc>
    
ORG 0x20

MAIN: 
    CALL    HEXTOBINARY        ; Convert both temps into tens/ones digits

    MOVLW   refTemp            ; Load reference temperature
    MOVWF   REG20              ; Store in REG20

    MOVLW   measuredTemp       ; Load measured temperature
    MOVWF   REG21              ; Store in REG21
    
    CLRF    STATUS             ; Clear status flags

    MOVLW   0x00               ; Load zero
    ADDLW   measuredTemp       ; Add measuredTemp (effectively W = measuredTemp)
    BN      LED_HEATER         ; If negative (never here), go to heater
    
    MOVLW   refTemp            ; Load reference temp
    CPFSEQ  REG21              ; Compare measuredTemp == refTemp
    GOTO    COMPARATOR         ; If not equal, go compare
    CALL    LED_OFF            ; If equal, turn off LEDs
    GOTO    MAIN               ; Loop back
    
COMPARATOR:
    CPFSGT  REG21              ; Skip next if REG21 > W (refTemp)
    GOTO    LED_HEATER         ; If measured < ref → heater ON
    GOTO    LED_COOLER         ; Else → cooler ON
    
ORG 0x80
LED_OFF:
    BCF     LATD, 0             ; Turn off heater LED
    BCF     LATD, 1             ; Turn off cooler LED
    CLRF    contReg            ; Clear control register
    RETURN
    
LED_COOLER:
    BSF     LATD, 1             ; Turn ON cooler LED
    BCF     LATD, 0             ; Turn OFF heater LED
    MOVLW   0x2                ; Set mode = cooler
    MOVWF   contReg
    GOTO    MAIN
    
LED_HEATER:
    BSF     LATD, 0             ; Turn ON heater LED
    BCF     LATD, 1             ; Turn OFF cooler LED
    MOVLW   0x1                ; Set mode = heater
    MOVWF   contReg
    GOTO    MAIN
    
org 300
    
HEXTOBINARY:
        ; Convert refTemp into tens (REG60) and ones (REG61)
        CLRF    0x60            ; Clear tens digit
        MOVLW   refTemp
        MOVWF   TEMP            ; TEMP = refTemp
        MOVF    TEMP, W
        MOVWF   0x61            ; Ones register initially holds full value
        
    DIVIDEBY10REF:
        MOVLW   10
        SUBWF   0x61, F         ; Subtract 10 from ones
        BNC     STORERESULTREF  ; If borrow → stop

        INCF    0x60, F         ; Increment tens digit
        BRA     DIVIDEBY10REF   ; Repeat

    STORERESULTREF:
        MOVLW   10
        ADDWF   0x61, F         ; Restore last subtraction
    
        ; Convert measuredTemp into tens (REG70) and ones (REG71)
        CLRF    0x70            ; Clear tens digit
        MOVLW   measuredTemp
        MOVWF   TEMP
        MOVF    TEMP, W
        MOVWF   0x71            ; Ones register initially holds full value
        
    DIVIDEBY10MEAS:
        MOVLW   10
        SUBWF   0x71, F         ; Subtract 10
        BNC     STORERESULTMEAS ; Stop if borrow

        INCF    0x70, F         ; Increment tens
        BRA     DIVIDEBY10MEAS

    STORERESULTMEAS:
        MOVLW   10
        ADDWF   0x71, F         ; Restore ones digit

    ; Only expect inputs < 100, so clear hundreds registers
    MOVLW   0
    MOVWF   REG62
    MOVWF   REG72
    RETURN
