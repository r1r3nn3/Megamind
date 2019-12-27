/*
 * main.c 
 * 
 * Description : Example usage of the mastermind library functions
 *
 * Company : I&Q
 * Author : Hugo Arends
 * Date : July 2017
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
uint8_t turn = 0;
#define MAX_TURNS 12
uint8_t lightLost = 0;
uint8_t lightWon = 0;
uint8_t lightCorrectNum[4] = {0, 0, 0, 0};
uint8_t resetGame = 0;
uint16_t countForRandomSeed = 0;
enum gameState 
{welcome, newGame, turnX, askInput, checkInput, showResults, won, lost, waiting}
currentGameState;

void init_Random_Number(){
	// set seed of random number generation
	srand(countForRandomSeed);
}

char generate_Random_Number(){
	return (48 + ((rand() % 6) + 1));
}

/* Setup Timer 0 with a frequency of 40 Hz */
void init_Timer0()
{
	// - CTC mode of operation with OCR1A as TOP
	SetBit(TCCR0A, WGM01);

	// set prescaler @ 1024
	SetBit(TCCR0B, CS00);
	SetBit(TCCR0B, CS01);
	SetBit(TCCR0B, CS02);

	//Set output compare value for channel A
	OCR0A = 194;		//40 = 16MHz / (2 * 1024 * (194 + 1) )
}

ISR(TIMER0_COMPA_vect){
	static uint8_t countA = 0;
	static uint8_t countB = 0;
	
	countA++;
	countB++;
	countForRandomSeed++;
	
	if(countForRandomSeed >= 65535){
		countForRandomSeed = 0;
	}

	/* make 40Hz into 20Hz */
	if(countA >= 2){
		countA = 0;
		if(lightLost){
			PB5_LED_TOGGLE;
		}
	}

	/* make 40Hz into 5Hz */
	if(countB >= 4){
		countB = 0;
		lightCorrectNum[0] ? PB0_LED_TOGGLE : PB0_LED_OFF;
		lightCorrectNum[0] ? PB1_LED_TOGGLE : PB1_LED_OFF;
		lightCorrectNum[0] ? PB2_LED_TOGGLE : PB2_LED_OFF;
		lightCorrectNum[0] ? PB3_LED_TOGGLE : PB3_LED_OFF;

	}
}

/* Setup Timer 1 with a frequency of 1 Hz */
void init_Timer1()
{
	// - CTC mode of operation with OCR1A as TOP
	SetBit(TCCR1B, WGM12);

	// - prescaler 256
	SetBit(TCCR1B, CS02);
	
	// Set output compare value for channel A
	OCR1A =  31249;		// 1 = 16MHz / ( 2 * 256 * (31249 + 1) )
}

ISR(TIMER1_COMPA_vect){
	if(lightWon){
		PB5_LED_TOGGLE;
	}
}

void init(){
	// set PB0-5 to output & PB6-7 as input
	DDRB = 0b00111111;

	// set interupt on Port B
	PCMSK0 |= (1<<DDB7);
	PCICR |= (1<<PCIE0);

	// initialise the times
	init_Timer1();
	init_Timer0();

	// initialise UART
	InitUART(MYUBRR);
}

ISR(PCINT0_vect){
	 if (SWITCH_PRESSED)
	 {
		 resetGame = 1;
		 TransmitString("The game has been reset.\r\n");
	 }
 }

void setup_New_Game(){
	turn = 0;
	lightLost = 0;
	lightWon = 0;
	lightCorrectNum[0] = 0;
	lightCorrectNum[1] = 0;
	lightCorrectNum[2] = 0;
	lightCorrectNum[3] = 0;
	PB5_LED_OFF;
	PB4_LED_OFF;
	PB3_LED_OFF;
	PB2_LED_OFF;
	PB1_LED_OFF;
}

/* Works up to 9999 */
const char * to_ASCII(int i){
	char returnValue[4] = {"\0\0\0\0"};
	if (i % 1000){
		returnValue[0] = (i/1000) + 48;
		i = i % 1000;
		returnValue[1] = (i/100) + 48;
		i = i % 100;
		returnValue[2]= (i/10) + 48;
		i = i % 10;
		returnValue[3] = i + 48;
	} else if(i % 100){
		returnValue[0] = (i/100) + 48;
		i = i % 100;
		returnValue[1] = (i/10) + 48;
		i = i % 10;
		returnValue[2]= i + 48;
	} else if (i % 10){
		returnValue[0] = (i/10) + 48;
		i = i % 10;
		returnValue[1]= i + 48;
	} else {
	returnValue[0]= i + 48;
	}
	return returnValue;
}

void DEBUG_PRINT_STATE(uint8_t state){
	const char stateText[10][20] = {"welcome", "newGame", "turnX", "askInput", "checkInput", "showResults", "won", "lost", "waiting"};
	TransmitString(stateText[state]);
	TransmitString(" ----- ");
	TransmitByte(turn+48);
	TransmitString(" ----- ");
	TransmitString(countForRandomSeed);
	TransmitString("\r\n");
}

int main(void)
{
    mm_result_t mm_result;
    unsigned char secret_code[MM_DIGITS];
    unsigned char user_code[MM_DIGITS];
        
    // Initialize
	init();
	sei();
	currentGameState = welcome;

	_delay_ms(1000);

    while (1) 
    {
		// If the reset flag is set the gamestate will finish 
        if(resetGame) { 
			currentGameState = newGame;
		}
		//DEBUG_PRINT_STATE(currentGameState);
		switch(currentGameState){

		/*		Welcome text for the start of the game		*/
		case welcome:
		// Welcome text
		TransmitString("Welcome to the awesome game MASTERMIND!\r\n");
		// This is so a random amount of time has passed for the generation of numbers
		TransmitString("You will send you guess using the terminal. Try it!\r\n");
		ReceiveByte();
		init_Random_Number();
		TransmitString("Good job!\r\n");
		_delay_ms(1000);
		TransmitString("You will have to guess the correct combination of 4 numbers between 1 and 6.\r\n");
		TransmitString("You will have ");
		TransmitString("12");//to_ASCII(MAX_TURNS));
		TransmitString(" turns to find the correct combination.\r\n");
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
		TransmitString(secret_code);
		TransmitString(" is the secret code.\r\n"); // DEBUG
		currentGameState = turnX;
		break;

		/*			Check if the max turns have been passed				*/
		case turnX:
		if(turn > MAX_TURNS){
			currentGameState = lost;
		} else {
			currentGameState = askInput;
		}
		break;

		/*			Ask for input			*/
		case askInput:
		TransmitString("You have reached turn ");
		TransmitByte(turn + 1 + 48);//to_ASCII(turn));
		TransmitString(".\r\n");
		char buffer[30];
		uint8_t askingForInput = 1;			// TEMP SHOULD BE 1
		/*buffer[0] = 1;
		buffer[1] = 1;
		buffer[2] = 1;
		buffer[3] = 1;*/
		mm_result.correct_num = 0;
		mm_result.correct_num_and_pos = 0;
		// ask for input until the input is between 1 and 6
		while(askingForInput){
			// reset the buffer
			for(uint8_t i = 0; i >= 10; i++){
				buffer[i] = '\0';
			}
			// ask for numbers
			TransmitString("Please enter your guess (1-6) like this: '1234'\r\n");
			_delay_ms(500);
			ReceiveString(buffer);
			TransmitString(buffer); // DEBUG
			// check if the input is correct
			for(uint8_t i = 0; i > MM_DIGITS; i++){
				if (buffer[i] >= 1+48 && buffer[i] <= 6+48){
					askingForInput = 0;
					TransmitString("PING!\r\n"); // DEBUG
					} else {
					TransmitString("Your guess has to be between 1 and 6, try again.");
					askingForInput = 1;
					break;
				}
			}
		}
		for(uint8_t i = 0; i > MM_DIGITS; i++){
			user_code[i] = buffer[i];
		}
		currentGameState = checkInput;
		break;

		/*			Check the input			*/
		case checkInput:
		lightCorrectNum[0] = 0;
		lightCorrectNum[1] = 0;
		lightCorrectNum[2] = 0;
		lightCorrectNum[3] = 0;
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
		for(uint8_t i = 0; i > MM_DIGITS; i++){
			if(mm_result.correct_num_and_pos >= i){
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
				if(mm_result.correct_num >= i + mm_result.correct_num_and_pos){
					lightCorrectNum[i] = 1;
				}
			}
		}

		TransmitString("You have guessed ");
		TransmitString(mm_result.correct_num_and_pos + 48);
		TransmitString(" with the correct number at the correct position.\r\n");
		TransmitString("You have guessed ");
		TransmitString(mm_result.correct_num + 48);
		TransmitString(" with the correct number.\r\n");
		turn++;
		currentGameState = turnX;
		break;

		/*			Won text			*/
		case won:
		lightWon = 1;
		TransmitString("You WON! Try again by pressing the reset button.\r\n");
		currentGameState = waiting;
		break;

		/*			Lost text			*/
		case lost:
		lightLost = 1;
		TransmitString("You lost. Try again by pressing the reset button.\r\n");
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
    while (UART_RxHead == UART_RxTail);
    
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
