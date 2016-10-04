

#include <xc.h>
#include <sys/attribs.h>
#include "lcd.h"
#include "timer.h"
#include "config.h"
#include "interrupt.h"
#include "keypad.h"


// ******************************************************************************************* //

typedef enum stateEnum { //all states are for debouncing; states handle LEDs.
    enterPasscode,
    enterWait,
    checkDigit,
    writeDigit,
    checkPasscode,
    badPasscode,
    goodPasscode,
    reset,
    writeSetMode,
    setMode,
    setWait,
    checkDigitSet,
    invalidPasscode,
    validPasscode,
    waitPress,
    readKey,
    waitRelease,
    debouncePress,
    debounceRelease,     
} stateType;

typedef enum modeEnum { 
   modeCheck,
   modeSet   
} modeType;

volatile stateType state = enterPasscode;

volatile modeType mode = modeCheck;

int main(void)
{
    char key;
    int i, j;
    int keys[4];
    int passcodes[10][4];
    int passcodeCount = 0;
    int keyCount;
    
    SYSTEMConfigPerformance(10000000);
    enableInterrupts();
    
    initLCD();
    initKeypad();
    
    for(i = 0; i < 10; ++i) {
        for(j = 0; j < 4; ++j) {
            passcodes[i][j] = -1;
        }
     }
    
    while(1) {
        switch(state) {
            case enterPasscode:
                clearLCD();
                moveCursorLCD(0,1);
                printStringLCD("Enter");
                moveCursorLCD(0,2);
                keyCount = 0;
                for(i = 0; i < 4; ++i) {
                    keys[i] = -1;
                }
                
                mode = modeCheck;
                state = enterWait;
                break;
            case enterWait:
                break;
            case readKey:
                key = scanKeypad();
                state = waitRelease;
                break;
            case waitRelease:
                break;
            case debounceRelease:
                delayUs(5000);
                switch(mode) {
                    case modeCheck:
                        state = checkDigit;
                        break;
                    case modeSet:
                        state = checkDigitSet;
                        break;
                }
                break;  
            case checkDigit:
                if(key == -1) {
                    state = enterWait;
                    break;
                }
                keys[keyCount] = key;
                ++keyCount;
                
                if(keyCount == 4 && key != '*' && key != '#') {
                    state = checkPasscode;
                }
                
                if(keyCount < 4 && key != '#' && (keyCount == 1 || key != '*')){
                    state = writeDigit;
                }               
                if(key == '#') {
                    state = badPasscode;
                }
                else if (key == '*' && keyCount > 1 && !(keys[0] == '*' && keys[1] == '*')) {
                    state = badPasscode;
                }
                else if (key != '*' && keys[0] == '*'){
                    state = badPasscode;
                }
                if(keys[0] == '*' && keys[1] == '*') {
                    state = setMode;
                }
                break;
                
            case writeDigit:
                printCharLCD(key);
                if(mode == modeCheck) {
                    state = enterWait;
                }
                else if (mode == modeSet) {
                    state = setWait;
                }
                break;
                
            case checkPasscode:
                for(i = 0; i < 10; ++i) {
                    for(j = 0; j < 4; ++j) {
                        if(keys[j] != passcodes[i][j]){
                            break;
                        }
                    }
                    if(j == 4) {
                        state = goodPasscode;
                        break;
                    }
                }
                if(i == 10){ 
                    state = badPasscode;
                }
                break;
                
            case badPasscode:
                clearLCD();
                moveCursorLCD(0,1);
                printStringLCD("Bad");
                for(i = 0; i < 2000; ++i) {
                    delayUs(1000);
                }
                state = enterPasscode;
                break;
                
            case goodPasscode:
                clearLCD();
                moveCursorLCD(0,1);
                printStringLCD("Good");
                for(i = 0; i < 2000; ++i) {
                    delayUs(1000);
                }
                state = enterPasscode;
                break;
                
            case setMode:
                mode = modeSet;
                for(i = 0; i < 4; ++i) {
                    keys[i] = -1;                  
                }
                keyCount = 0;
                clearLCD();
                moveCursorLCD(0,1);
                printStringLCD("Set Mode");
                moveCursorLCD(0,2);
                state = setWait;
                break;
                
            case setWait:
                break;
                
            case checkDigitSet:
                if(key == -1) {
                    state = setWait;
                    break;
                }
                keys[keyCount] = key;
                ++keyCount;
                if(passcodeCount == 10) {   //sets max passcode limitation
                    state = invalidPasscode;
                    break;
                }
                if(keyCount < 4) {
                    state = writeDigit;
                }
                if(keyCount == 4) {
                    state = validPasscode;
                }
                if(key == '*' || key == '#') {
                    state = invalidPasscode;
                }
                break;
                
            case validPasscode:
                for(i = 0; i < 4; ++i) {
                    passcodes[passcodeCount][i] = keys[i];
                }
                ++passcodeCount;
                clearLCD();
                moveCursorLCD(0,1);
                printStringLCD("Valid");
                for(i = 0; i < 2000; ++i) {
                    delayUs(1000);
                }
                state = enterPasscode;
                break;
                
            case invalidPasscode:
                clearLCD();
                moveCursorLCD(0,1);
                printStringLCD("Invalid");
                for(i = 0; i < 2000; ++i) {
                    delayUs(1000);
                }
                state = enterPasscode;
                break;
        }
    }
}
                
            
             


/*void __ISR(_TIMER_1_VECTOR, IPL7SRS) _T1Interrupt(void) {
    IFS0bits.T1IF = 0; //lower the flag 
    if(state == run)
    {
        currentTime += 1; //add 1 ms if we are running, otherwise ignore!
    }
}*/

void __ISR(_CHANGE_NOTICE_VECTOR, IPL7SRS) _CNInterrupt(void) {
    IFS1bits.CNCIF = 0;
    IFS1bits.CNDIF = 0;
    IFS1bits.CNEIF = 0;
    
    int x, y, z;
    x = PORTC; // clearing the mismatch condition
    y = PORTD;
    z = PORTE;
    
    if(state == enterWait || state == setWait) {
        state = readKey;
    }
    else if (state == waitRelease && COL_P3 && COL_P1 && COL_P5 ) {
        state = debounceRelease;
    }

    
    
    
} 