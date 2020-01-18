/*
Create by:      Alwin Rodewijk
Student nr:     635653
Class:          D-B1-ELTa
Subject:        INMC
Teacher:        Ewout Boks
Date:           18-1-2020
*/

#define F_CPU 16000000UL
#define BAUD 9600
#define MYUBRR F_CPU/16/BAUD-1

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdlib.h>
#include <string.h>

#include "mastermind.h"

#define SetBit(a, b)	(a |= (1<<b))
#define ClearBit(a, b)	(a &= ~(1<<b))

#define PB0_LED_ON     SetBit(PORTB,PORTB0)
#define PB0_LED_OFF    ClearBit(PORTB,PORTB0)
#define PB0_LED_TOGGLE SetBit(PINB,PINB0)

#define PB1_LED_ON     SetBit(PORTB,PORTB1)
#define PB1_LED_OFF    ClearBit(PORTB,PORTB1)
#define PB1_LED_TOGGLE SetBit(PINB,PINB1)

#define PB2_LED_ON     SetBit(PORTB,PORTB2)
#define PB2_LED_OFF    ClearBit(PORTB,PORTB2)
#define PB2_LED_TOGGLE SetBit(PINB,PINB2)

#define PB3_LED_ON     SetBit(PORTB,PORTB3)
#define PB3_LED_OFF    ClearBit(PORTB,PORTB3)
#define PB3_LED_TOGGLE SetBit(PINB,PINB3)

#define PB4_LED_ON     SetBit(PORTB,PORTB4)
#define PB4_LED_OFF    ClearBit(PORTB,PORTB4)
#define PB4_LED_TOGGLE SetBit(PINB,PINB4)

#define PB5_LED_ON     SetBit(PORTB,PORTB5)
#define PB5_LED_OFF    ClearBit(PORTB,PORTB5)
#define PB5_LED_TOGGLE SetBit(PINB,PINB5)

#define SWITCH_PRESSED !(PINB & (1<<PINB7))

/* UART Buffer Defines */
#define UART_RX_BUFFER_SIZE 32 /* 2,4,8,16,32,64,128 or 256 bytes */
#define UART_TX_BUFFER_SIZE 32

#define UART_RX_BUFFER_MASK (UART_RX_BUFFER_SIZE - 1)
#if (UART_RX_BUFFER_SIZE & UART_RX_BUFFER_MASK)
#error RX buffer size is not a power of 2
#endif

#define UART_TX_BUFFER_MASK (UART_TX_BUFFER_SIZE - 1)
#if (UART_TX_BUFFER_SIZE & UART_TX_BUFFER_MASK)
#error TX buffer size is not a power of 2
#endif

/* Static Variables */
static char UART_RxBuf[UART_RX_BUFFER_SIZE];
static volatile char UART_RxHead;
static volatile char UART_RxTail;

static char UART_TxBuf[UART_TX_BUFFER_SIZE];
static volatile char UART_TxHead;
static volatile char UART_TxTail;

/* Prototypes */
void InitUART(unsigned int ubrr_val);
char ReceiveByte(void);
void TransmitByte(char data);
unsigned char nUnreadBytes(void);
void ReceiveString(char *str);
void TransmitString(char *str);

/* game variables */
uint8_t showSecretCode = 0;		// 1 to show & 0 to hide  !!!! CHEAT MODE !!!!

static mm_result_t mm_result;
uint8_t turn = 0;
#define MAX_TURNS 12
#define BUFFER_SIZE 30
uint8_t lightLost = 0;
uint8_t lightWon = 0;
uint8_t lightCorrectNum[4] = {0, 0, 0, 0};
uint8_t lightCorrectNumLEDstate[4] = {0, 0, 0, 0};
uint8_t resetGame = 0;
uint16_t countForRandomSeed = 0;
uint8_t showLEDs = 0;
char buffer[BUFFER_SIZE];
enum gameState 
{welcome, newGame, turnX, askInput, checkInput, showResults, won, lost, waiting}
currentGameState;


char generate_Random_Number(){
	char returnValue[2];
	itoa(((rand() % 6) + 1), returnValue, 10);
	return returnValue[0];
}

ISR(TIMER0_COMPA_vect){
	static uint8_t countA = 0;
	static uint8_t countB = 0;
	
	countA++;
	countB++;

	/* make 40Hz into 20Hz */
	if(countA >= 2){
		countA = 0;
		if(lightLost){
			PB5_LED_TOGGLE;
		}
	}

	/* make 40Hz into 5Hz */
	if(countB >= 8){
		countB = 0;

		if(lightCorrectNum[0] == 1 && showLEDs == 1){
			if(lightCorrectNumLEDstate[0] == 0){
				PB0_LED_ON;
				lightCorrectNumLEDstate[0] = 1;
				} else {
				PB0_LED_OFF;
				lightCorrectNumLEDstate[0] = 0;
			}
		}
		if(lightCorrectNum[1] == 1){
			if(lightCorrectNumLEDstate[1] == 0){
				PB1_LED_ON;
				lightCorrectNumLEDstate[1] = 1;
				} else {
				PB1_LED_OFF;
				lightCorrectNumLEDstate[1] = 0;
			}
		}
		if(lightCorrectNum[2] == 1){
			if(lightCorrectNumLEDstate[2] == 0){
				PB2_LED_ON;
				lightCorrectNumLEDstate[2] = 1;
				} else {
				PB2_LED_OFF;
				lightCorrectNumLEDstate[2] = 0;
			}
		}
		if(lightCorrectNum[3] == 1){
			if(lightCorrectNumLEDstate[3] == 0){
				PB3_LED_ON;
				lightCorrectNumLEDstate[3] = 1;
				} else {
				PB3_LED_OFF;
				lightCorrectNumLEDstate[3] = 0;
			}
		}
	}
}

/* Setup Timer 0 with a frequency of 40 Hz */
void init_Timer0()
{
	// - CTC mode of operation with OCR0A as TOP
	SetBit(TCCR0A, WGM01);
	
	// set prescaler to 1024
	TCCR0B |= (1<<CS00) | (1<<CS02);
	
	// enable OCR0A on timer 0
	TIMSK0 |= (1<<OCIE0A);
	
	// set output compare A to 194
	OCR0A = 194;		//40 = 16MHz / (2 * 1024 * (194 + 1) )
}

/* Setup Timer 1 with a frequency of 1 Hz */
void init_Timer1()
{
	// - CTC mode of operation with OCR1A as TOP
	SetBit(TCCR1B, WGM12);

	// - prescaler 256
	SetBit(TCCR1B, CS02);

	// enable OCR0A on timer 1
	SetBit(TIMSK1, OCIE1A);
	
	// Set output compare value for channel A
	OCR1A =  31249;		// 1 = 16MHz / ( 2 * 256 * (31249 + 1) )
}

ISR(TIMER1_COMPA_vect){
	if(lightWon){
		PB5_LED_TOGGLE;
	}
}

void init(){
	// initialise UART
	InitUART(MYUBRR);
	
	// set PB0-5 to output & PB6-7 as input
	DDRB = 0b00111111;

	// set interupt on Port B
	PCMSK0 |= (1<<DDB7);
	PCICR |= (1<<PCIE0);
	// initialise the times
	init_Timer1();
	init_Timer0();
}

ISR(PCINT0_vect){
	 if (SWITCH_PRESSED)
	 {
		 resetGame = 1;
		 TransmitString("The game has been reset.\n");
	 }
 }

void setup_New_Game(){
	turn = 0;
	lightLost = 0;
	lightWon = 0;
	mm_result.correct_num = 0;
	mm_result.correct_num_and_pos = 0;
	lightCorrectNum[0] = 0;
	lightCorrectNum[1] = 0;
	lightCorrectNum[2] = 0;
	lightCorrectNum[3] = 0;
	PB5_LED_OFF;
	PB4_LED_OFF;
	PB3_LED_OFF;
	PB2_LED_OFF;
	PB1_LED_OFF;
	PB0_LED_OFF;	
}

int main(void)
{
    unsigned char secret_code[MM_DIGITS];
    unsigned char user_code[MM_DIGITS];
        
    // Initialize
	init();
	sei();
	currentGameState = welcome;

    while (1) 
    {
		// If the reset flag is set the gamestate will finish 
        if(resetGame) { 
			currentGameState = newGame;
			resetGame = 0;
		}

		switch(currentGameState){

		/*		Welcome text for the start of the game		*/
		case welcome:
		// Welcome text
		TransmitString("Welcome to the awesome game MASTERMIND!\n");
		// This is so a random amount of time has passed for the generation of numbers
		TransmitString("You will send you guess using the terminal. Try it!\n");
		while (ReceiveByte() != '\n'){
			;
		}
		srand(countForRandomSeed);
		TransmitString("Good job!\n");
		_delay_ms(500);
		TransmitString("You will have to guess the correct combination of ");
		itoa(MM_DIGITS, buffer, 10);
		TransmitString(buffer);
		TransmitString(" numbers between 1 and 6.\nYou will have ");
		itoa(MAX_TURNS, buffer, 10);
		TransmitString(buffer);
		TransmitString(" turns to find the correct combination.\n");
		currentGameState = newGame;
		break;

		/*			Start a new game			*/
		case newGame:
		setup_New_Game();
		// Set the secret code
		for(uint8_t i = 0; i < MM_DIGITS; i++){
			secret_code[i] = generate_Random_Number();
		}
		set_secret_code(secret_code);

		// Cheat mode
		if (showSecretCode){
			for (uint8_t i = 0; i < MM_DIGITS; i++){
				buffer[i] = secret_code[i];
			}
			buffer[MM_DIGITS + 1] = '\0';
			TransmitString(buffer);
			TransmitString(" is the secret code.\n");
		}
		
		currentGameState = turnX;
		break;

		/*			Check if the max turns have been passed				*/
		case turnX:
		if(turn >= MAX_TURNS){
			currentGameState = lost;
		} else {
			currentGameState = askInput;
			turn++;
		}
		break;

		/*			Ask for input			*/
		case askInput:
		TransmitString("You have reached turn ");
		itoa(turn, buffer, 10);
		TransmitString(buffer);
		TransmitString(".\n");
		uint8_t askingForInput = 1;
		mm_result.correct_num = 0;
		mm_result.correct_num_and_pos = 0;
		// ask for input until the input is between 1 and 6
		while(askingForInput){
			// ask for numbers
			TransmitString("Please enter your guess like this: '1234'\n");
			_delay_ms(100);
			ReceiveString(buffer);

			// check if the input is accepted
			for(uint8_t i = 0; i < MM_DIGITS; i++){
				if (buffer[i] >= '1' && buffer[i] <= '6'){
					askingForInput = 0;
				} else {
					TransmitString("Your guess has to be between 1 and 6, try again.\n");

					askingForInput = 1;
					break;
				}
			}
		}

		for(uint8_t i = 0; i < MM_DIGITS; i++){
			user_code[i] = buffer[i];
		}
		currentGameState = checkInput;
		break;

		/*			Check the input			*/
		case checkInput:
		showLEDs = 0;
		lightCorrectNum[0] = 0;
		lightCorrectNum[1] = 0;
		lightCorrectNum[2] = 0;
		lightCorrectNum[3] = 0;
		PB0_LED_OFF;
		PB1_LED_OFF;
		PB2_LED_OFF;
		PB3_LED_OFF;
		// Check the user code against the secret code
		mm_result = check_secret_code(user_code);
		if(mm_result.correct_num_and_pos >= 4){
			currentGameState = won;
		} else {
			currentGameState = showResults;
		}
		break;

		/*		Show results to player		 */
		case showResults:
		TransmitString("You have guessed ");
		itoa(mm_result.correct_num_and_pos, buffer, 10);
		TransmitString(buffer);
		TransmitString(" with the correct number at the correct position.\n");
		TransmitString("You have guessed ");
		itoa(mm_result.correct_num, buffer, 10);
		TransmitString(buffer);
		TransmitString(" with the correct number.\n");

		for(uint8_t i = 0; i < MM_DIGITS; i++){
			if(mm_result.correct_num_and_pos > i){
				switch(i){
				case 0:
				PB0_LED_ON;
				break;
				case 1:
				PB1_LED_ON;
				break;
				case 2:
				PB2_LED_ON;
				break;
				case 3:
				PB3_LED_ON;
				break;
				default:
				break;
				}
			} else {
				if(mm_result.correct_num > i - mm_result.correct_num_and_pos){
					lightCorrectNum[i] = 1;
				}
			}
		}
		showLEDs = 1;
		currentGameState = turnX;
		break;

		/*			Won text			*/
		case won:
		lightCorrectNum[0] = 0;
		lightCorrectNum[1] = 0;
		lightCorrectNum[2] = 0;
		lightCorrectNum[3] = 0;
		PB0_LED_OFF;
		PB1_LED_OFF;
		PB2_LED_OFF;
		PB3_LED_OFF;
		lightWon = 1;
		TransmitString("You WON! Try again by pressing the reset button.\n");
		currentGameState = waiting;
		break;

		/*			Lost text			*/
		case lost:
		lightCorrectNum[0] = 0;
		lightCorrectNum[1] = 0;
		lightCorrectNum[2] = 0;
		lightCorrectNum[3] = 0;
		PB0_LED_OFF;
		PB1_LED_OFF;
		PB2_LED_OFF;
		PB3_LED_OFF;
		lightLost = 1;
		TransmitString("You lost. Try again by pressing the reset button.\n");
		currentGameState = waiting;
		break;

		/*			Wait for a reset flag			*/
		case waiting:
		default:
		_delay_ms(500);
		break;
		}
    }
}

/* Initialize UART */
void InitUART(unsigned int ubrr_val)
{
    char x;
    
    /* Set the baud rate */
    UBRR0H = (unsigned char)(ubrr_val>>8);
    UBRR0L = (unsigned char)ubrr_val;
    
    /* Enable UART receiver and transmitter */
    UCSR0B = ((1<<RXEN0) | (1<<TXEN0) | (1<<RXCIE0));
    
    /* Flush receive buffer */
    x = 0;
    UART_RxTail = x;
    UART_RxHead = x;
    UART_TxTail = x;
    UART_TxHead = x;
}

/* Interrupt handlers */
ISR(USART_RX_vect)
{
    char data;
    unsigned char tmphead;
    
    /* Read the received data */
    data = UDR0;
    
    /* Calculate buffer index */
    tmphead = (UART_RxHead + 1) & UART_RX_BUFFER_MASK;
    
    /* Store new index */
    UART_RxHead = tmphead;
    
    if (tmphead == UART_RxTail)
    {
        /* ERROR! Receive buffer overflow */
    }
    
    /* Store received data in buffer */
    UART_RxBuf[tmphead] = data;
}

ISR(USART_UDRE_vect)
{
    unsigned char tmptail;
    
    /* Check if all data is transmitted */
    if (UART_TxHead != UART_TxTail)
    {
        /* Calculate buffer index */
        tmptail = ( UART_TxTail + 1 ) & UART_TX_BUFFER_MASK;
        
        /* Store new index */
        UART_TxTail = tmptail;
        
        /* Start transmission */
        UDR0 = UART_TxBuf[tmptail];
    }
    else
    {
        /* Disable UDRE interrupt */
        UCSR0B &= ~(1<<UDRIE0);
    }
}

char ReceiveByte(void)
{
    unsigned char tmptail;
    
    /* Wait for incoming data */
    while (UART_RxHead == UART_RxTail){
	    countForRandomSeed += 1;
	    if(countForRandomSeed > 65534){
		    countForRandomSeed = 0;
	    }
    }
    
    /* Calculate buffer index */
    tmptail = (UART_RxTail + 1) & UART_RX_BUFFER_MASK;
    
    /* Store new index */
    UART_RxTail = tmptail;
    
    /* Return data */
    return UART_RxBuf[tmptail];
}

void TransmitByte(char data)
{
    unsigned char tmphead;
    
    /* Calculate buffer index */
    tmphead = (UART_TxHead + 1) & UART_TX_BUFFER_MASK;
    
    /* Wait for free space in buffer */
    while (tmphead == UART_TxTail);
    
    /* Store data in buffer */
    UART_TxBuf[tmphead] = data;
    
    /* Store new index */
    UART_TxHead = tmphead;
    
    /* Enable UDRE interrupt */
    UCSR0B |= (1<<UDRIE0);
}

/*
 * This function returns the number of unread bytes in the receive buffer
 */
unsigned char nUnreadBytes(void)
{
    if(UART_RxHead == UART_RxTail)
        return 0;
    else if(UART_RxHead > UART_RxTail)
        return UART_RxHead - UART_RxTail;
    else
        return UART_RX_BUFFER_SIZE - UART_RxTail + UART_RxHead;
}

/*
 * This function gets a string of characters from the USART.
 * The string is placed in the array pointed to by str.
 *
 * - This function uses the function ReceiveByte() to get a byte
 *   from the UART.
 * - If the received byte is equal to '\n' (Line Feed),
 *   the function returns.
 * - The array is terminated with ´\0´.
 */
void ReceiveString(char *str)
{
    uint8_t t = 0;
    
    while ((str[t] = ReceiveByte()) != '\n')
    {
        t++;
    }
    str[t++] = '\n';
    str[t] = '\0';
}

/*
 * Transmits a string of characters to the USART.
 * The string must be terminated with '\0'.
 *
 * - This function uses the function TransmitByte() to
 *   transmit a byte via the UART
 * - Bytes are transmitted until the terminator
 *   character '\0' is detected. Then the function returns.
 */
void TransmitString(char *str)
{
	_delay_ms(50);
    while(*str)
    {
        TransmitByte(*str++);
    }
	_delay_ms(50);
}