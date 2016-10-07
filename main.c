

#include <xc.h>
#include <sys/attribs.h>
#include "lcd.h"
#include "timer.h"
#include "config.h"
#include "interrupt.h"
#include "keypad.h"


// ******************************************************************************************* //

typedef enum stateEnum { //all states are for debouncing; states handle LEDs.
    prompt,
    checkDigit,
    writeDigit,
    checkPasscode,
    badPasscode,
    goodPasscode,
    invalidPasscode,
    validPasscode,
    waitPress,
    readKey,
    waitRelease,
    debounceRelease,     
} stateType;

typedef enum modeEnum { 
   modeCheck,
   modeSet   
} modeType;

volatile stateType state = prompt;

volatile modeType mode = modeCheck;

int main(void)
{
    char key;               //Current key being registered
    int i, j;               
    int keys[4];            //Array of keys on screen
    int passcodes[10][4];
    int passcodeCount = 0;  //counts how many passcodes are stored. Max 10
    int keyCount;           //counts how many keys have been pressed (on the screen)
    
    SYSTEMConfigPerformance(10000000);
    enableInterrupts();
    
    initLCD();
    initKeypad();
    
    for(i = 0; i < 10; ++i) {           //Init passwords to invalid values
        for(j = 0; j < 4; ++j) {
            passcodes[i][j] = -1;
        }
     }
    
    /*void startSequence(modeType mode) {
        keyCount = 0;
        for(i = 0; i < 4; ++i) {
            keys[i] = -1;
        }
        clearLCD();
        moveCursorLCD(0,1);
        switch(mode) {
            case modeCheck:
                printStringLCD("Enter");
                break;
            case modeSet:
                printStringLCD("Set Mode");
                break;
        }
        moveCursorLCD(0,2);
    }*/
    
    while(1) {
        switch(state) {
            case prompt:
                keyCount = 0;
                for(i = 0; i < 4; ++i) {
                    keys[i] = -1;
                }
                clearLCD();
                moveCursorLCD(0,1);
                switch(mode) {
                    case modeCheck:
                        printStringLCD("Enter");
                        break;
                    case modeSet:
                        printStringLCD("Set Mode");
                        break;
                }
                moveCursorLCD(0,2);  
                state = waitPress;
                break;
            case waitPress: //wait for keypress
                break;
            case readKey:
                key = scanKeypad();
                state = waitRelease;
                break;
            case waitRelease:
                break;
            case debounceRelease:
                delayUs(5000);
                state = checkDigit;
                break;  
            case checkDigit:
                if(key == -1) {
                    state = waitPress;
                    break;
                }
                keys[keyCount] = key;
                ++keyCount;
                
                switch(mode) {
                    case modeCheck:
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
                            mode = modeSet;
                            state = prompt;
                        }
                        break;
                    case modeSet:
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
                }
                break;
                
            case writeDigit:
                printCharLCD(key);
                state = waitPress;
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
                mode = modeCheck;
                state = prompt;
                break;
                
            case goodPasscode:
                clearLCD();
                moveCursorLCD(0,1);
                printStringLCD("Good");
                for(i = 0; i < 2000; ++i) {
                    delayUs(1000);
                }
                mode = modeCheck;
                state = prompt;
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
                mode = modeCheck;
                state = prompt;
                break;
                
            case invalidPasscode:
                clearLCD();
                moveCursorLCD(0,1);
                printStringLCD("Invalid");
                for(i = 0; i < 2000; ++i) {
                    delayUs(1000);
                }
                mode = modeCheck;
                state = prompt;
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
    
    if(state == waitPress) {
        state = readKey;
    }
    else if (state == waitRelease && COL_P3 && COL_P1 && COL_P5 ) {
        state = debounceRelease;
    }

    
    
    
} 