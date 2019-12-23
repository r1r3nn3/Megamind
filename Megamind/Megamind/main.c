/*
 * Megamind.c
 *
 * Created: 23-12-2019 15:32:34
 * Author : Alwin
 */ 

 /* initialise */

#include "avr/io.h"
#include "avr/interrupt.h"
#include "string.h"

#define F_CPU 16000000UL
#include "util/delay.h"

#define BAUD 9600
#define MYUBRR F_CPU/16/BAUD-1

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

/* Prototypes Mastermind */
int generateCode(void);


/* Variables for Mastermind */
#define MAX_TURNS 12
unsigned int turns = 0;
char sercretCode[4] = "0000"; // code can be 4 x 0-6

/* Mastermind text */
const char textLost[] = "You lost.. \nPlay again? 'y' for yes, 'n' for no \r\n";
const char textWin[] = "You WON! \nPlay again? send 'y' to do so\r\n";
const char textEnterCode[] = "Please enter your guess ";
const char textExplaination[] = "";
const char textWrongEntry[] = "";

/* timer init for 20 Hz */
void timer0Init(){

}

/* timer init for 1 Hz */
void timer1Init(){

}

/* timier init for 5 Hz */
void timer2Init(){

}

int main(void)
{
    /* Initialize the UART */
    InitUART(MYUBRR);
    sei();
    
    while(1)
    {
        // CPU is busy doing something else...
        _delay_ms(1000);
        
        // Check for unread bytes in the receive buffer
        unsigned char nBytes = nUnreadBytes();
        
        // If there are unread bytes, receive them all and echo back
        // (Note: Make sure LF is enabled in Terminal Window)
        if(nBytes > 0)
        {
            char str[30];
            ReceiveString(str);
			TransmitString(str);
        }
    }

    return 0;
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

/* This function returns the number of unread bytes in the receive buffer*/

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
    while(*str)
    {
        TransmitByte(*str++);
    }
}