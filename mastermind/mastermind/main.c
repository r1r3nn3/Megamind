/*
 * main.c 
 * 
 * Description : Example usage of the mastermind library functions
 *
 * Company : I&Q
 * Author : Hugo Arends
 * Date : July 2017
 */ 

#include <avr/io.h>

#include "mastermind.h"

#define SetBit(a,b)      (a|=((1<<b)))
#define ClearBit(a,b)   (a&=(~(1<<b)))

#define LED_INIT   SetBit(DDRB,DDB5)
#define LED_ON     SetBit(PORTB,PORTB5)
#define LED_OFF    ClearBit(PORTB,PORTB5)

#define LED_TOGGLE SetBit(PINB,PINB5)

int main(void)
{
    mm_result_t mm_result;
    unsigned char secret_code[MM_DIGITS];
    unsigned char user_code[MM_DIGITS];
        
    // Initialize the LED    
    LED_INIT;
    LED_OFF;    

    // Set the secret code
    // TODO: Just one secret code is not very challenging
    secret_code[0] = 1;
    secret_code[1] = 1;
    secret_code[2] = 2;
    secret_code[3] = 2;
    
    set_secret_code(secret_code);
    
    // Just as an example, the user code is set statically
    // TODO: Get the code from the user
    user_code[0] = 1;
    user_code[1] = 2;
    user_code[2] = 3;
    user_code[3] = 4;
    
    // Check the user code against the secret code
    mm_result = check_secret_code(user_code);
    
    // Turn on the LED if the user code matches the secret code
    // TODO: Implement more meaningful feedback for the user
    if(mm_result.correct_num_and_pos == 4)
    {
      LED_ON;
    }    
    
    // Stop executing
    // TODO: Give the user 12 opportunities
    // TODO: Start over once a game has finished
    while (1) 
    {
        ; // Do nothing
    }
}
