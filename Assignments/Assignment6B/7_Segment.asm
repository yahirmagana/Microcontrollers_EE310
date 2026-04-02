//-----------------------------
// Title: 7‑Segment Display Controller (UP/DOWN Counter)
//-----------------------------
// Purpose: This program drives a 7‑segment display on PORTB using a lookup
// table stored in RAM. Two input buttons on PORTA (RA0 = UP, RA1 = DOWN)
// increment or decrement the displayed hexadecimal value (0–F).  
// If both buttons are pressed, the display resets to 0.
//
// The program uses FSR0 as a pointer to the segment‑pattern table and
// updates the display based on button activity. A delay loop is included
// to debounce transitions.
//
// Dependencies: AssemblyConfig.inc, xc.inc
// Compiler: MPLAB X IDE v6.30
// Author: Yahir Magana
//
// OUTPUTS: 7‑segment pattern on PORTB  
// INPUTS: RA0 (UP), RA1 (DOWN)
//
// Versions:
//      V1.0: 04/01/2026 – Fully working version
//-----------------------------
    
;---------------------
; Initialization
;---------------------
#include "AssemblyConfig.inc"
#include <xc.inc>

Inner_loop  equ 0XFF
Outer_loop  equ 0XFF

;---------------------------------------------------------
; These registers are used for the delay loop
;---------------------------------------------------------
REG10 EQU 10H
REG11 EQU 11H

NUM0 EQU 0x00
NUM1 EQU 0x01
NUM2 EQU 0x02
NUM3 EQU 0x03
NUM4 EQU 0x04
NUM5 EQU 0x05
NUM6 EQU 0x06
NUM7 EQU 0x07
NUM8 EQU 0x08
NUM9 EQU 0x09
NUMA EQU 0x0A
NUMB EQU 0x0B
NUMC EQU 0x0C
NUMD EQU 0x0D
NUME EQU 0x0E
NUMF EQU 0x0F

;---------------------------------------------------------
; Reset vector
;---------------------------------------------------------
    PSECT absdata,abs,ovrld
    ORG 0
    GOTO _setRegs

    ORG 0x20

;---------------------------------------------------------
; Setup segment table and pointer
;---------------------------------------------------------
_setRegs:

    ;-----------------------------------------------------
    ; These patterns are for COMMON‑CATHODE (1 = ON)
    ;-----------------------------------------------------
    MOVLW 0b0111111 ; 0
    MOVWF NUM0
    MOVWF PORTB     

    MOVLW 0b0000110 ; 1
    MOVWF NUM1

    MOVLW 0b1011011 ; 2
    MOVWF NUM2

    MOVLW 0b1001111 ; 3
    MOVWF NUM3

    MOVLW 0b1100110 ; 4
    MOVWF NUM4

    MOVLW 0b1101101 ; 5
    MOVWF NUM5

    MOVLW 0b1111101 ; 6
    MOVWF NUM6

    MOVLW 0b0000111 ; 7
    MOVWF NUM7

    MOVLW 0b1111111 ; 8
    MOVWF NUM8

    MOVLW 0b1101111 ; 9
    MOVWF NUM9

    MOVLW 0b1110111 ; A
    MOVWF NUMA

    MOVLW 0b1111100 ; b
    MOVWF NUMB

    MOVLW 0b0111001 ; C
    MOVWF NUMC

    MOVLW 0b1011110 ; d
    MOVWF NUMD

    MOVLW 0b1111001 ; E
    MOVWF NUME

    MOVLW 0b1110001 ; F
    MOVWF NUMF

    ;-----------------------------------------------------
    ; Set FSR0 to point to NUM0
    ;-----------------------------------------------------
    LFSR 0, NUM0

    GOTO _setup

;---------------------------------------------------------
; Configure ports
;---------------------------------------------------------
_setup:
    RCALL _setupPortB
    RCALL _setupPortA

    ; Display initial value
    MOVFF INDF0, PORTB   

    GOTO _main

;---------------------------------------------------------
; Main loop
;---------------------------------------------------------
_main:
    RCALL _check_UP      

;---------------------------------------------------------
; PORTB setup (7‑segment)
;---------------------------------------------------------
_setupPortB:
    BANKSEL PORTB
    CLRF PORTB           ; clear port

    BANKSEL LATB
    CLRF LATB            ; clear latch

    BANKSEL ANSELB
    CLRF ANSELB          ; digital mode

    BANKSEL TRISB
    MOVLW 0b00000000     ; all RB outputs
    MOVWF TRISB

    CLRF WREG
    RETURN

;---------------------------------------------------------
; PORTA setup (buttons)
;---------------------------------------------------------
_setupPortA:
    BANKSEL PORTA
    CLRF PORTA

    BANKSEL LATA
    CLRF LATA

    BANKSEL ANSELA
    CLRF ANSELA          ; digital

    BANKSEL TRISA
    MOVLW 0b11111111     ; ALL inputs (RA0/RA1 used)
    MOVWF TRISA

    CLRF WREG
    RETURN

;---------------------------------------------------------
; Button check logic 
;---------------------------------------------------------
_check_UP:
    BTFSS PORTA,0        ; if RA0 == 1 skip
    GOTO _check_DOWN     ; if RA0 == 0 (pressed), go DOWN check
    GOTO _check_BOTH     ; if RA0 == 1 (not pressed), go BOTH check

;---------------------------------------------------------
; DOWN check 
;---------------------------------------------------------
_check_DOWN:
    BTFSS PORTA,1        ; if RA1 == 1 skip
    GOTO _check_UP       ; if RA1 == 0 (pressed), go UP check (loop!)
    GOTO DOWN            ; if RA1 == 1 (not pressed), go DOWN

;---------------------------------------------------------
; UP routine 
;---------------------------------------------------------
UP:
    MOVFF POSTINC0, WREG ; increments pointer, loads old value

    CPFSEQ NUMF          ; compares W to NUMF CONTENT, not pointer
    GOTO SET_PINS
    GOTO WRAP_DOWN

WRAP_DOWN:
    LFSR 0, NUM0
    GOTO SET_PINS

;---------------------------------------------------------
; DOWN routine 
;---------------------------------------------------------
DOWN:
    MOVFF POSTDEC0, WREG ; decrements pointer

    CPFSEQ NUM0          ; compares W to NUM0 CONTENT, not pointer
    GOTO SET_PINS
    GOTO WRAP_UP

WRAP_UP:
    LFSR 0, NUMF
    GOTO SET_PINS

;---------------------------------------------------------
; BOTH pressed
;---------------------------------------------------------
_check_BOTH:
    BTFSS PORTA,1
    GOTO UP
    GOTO BOTH

BOTH:
    LFSR 0, NUM0
    GOTO _main

;---------------------------------------------------------
; Output to display
;---------------------------------------------------------
SET_PINS:
    CALL loopDelay
    MOVFF INDF0, PORTB   
    GOTO _main

;---------------------------------------------------------
; Delay
;---------------------------------------------------------
loopDelay:
    MOVLW Inner_loop
    MOVWF REG10
    MOVLW Outer_loop
    MOVWF REG11

_loop1:
    DECF REG10,1
    BNZ _loop1
    MOVLW Inner_loop
    MOVWF REG10
    DECF REG11,1
    BNZ _loop1
    RETURN

END
