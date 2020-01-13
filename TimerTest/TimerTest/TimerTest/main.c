/*
 * TimerTest.c
 *
 * Created: 13-1-2020 20:01:00
 * Author : Alwin
 */ 
 #define F_CPU 16000000UL // 16 MHz

 #include <avr/io.h>
 #include <util/delay.h>
 #include <avr/interrupt.h>

 #define LED_ON_PB0     PORTB |=  (1<<PORTB0)
 #define LED_OFF_PB0    PORTB &= ~(1<<PORTB0)
 #define LED_TOGGLE_PB0 PINB   =  (1<<PINB0)

  ISR(TIMER0_COMPA_vect)
  {
	LED_TOGGLE_PB0;
	}

void init_Timer0()
{
	TCCR0A |= (1<<WGM01);
	
	// set prescaler to 64
	TCCR0B |= (1<<CS00) | (1<<CS02);
	
	// enable OCR0A on timer 0
	TIMSK0 |= (1<<OCIE0A);
	
	// set output compare A to 249
	OCR0A = 194;
/*
	// - CTC mode of operation with OCR0A as TOP
	SetBit(TCCR0A, WGM01);
	
	// set prescaler to 1024
	TCCR0B |= (1<<CS00) | (1<<CS02);// | (1<<CS02);
	
	// enable OCR0A on timer 0
	TIMSK0 |= (1<<OCIE0A);
	
	// set output compare A to 249
	OCR0A = 194;
	
	/*
	// set prescaler @ 1024
	SetBit(TCCR0B, CS00);
	SetBit(TCCR0B, CS01);
	SetBit(TCCR0B, CS02);

	// enable OCR0A on timer 0
	SetBit(TIMSK0, OCIE0A);
	//Set output compare value for channel A
	OCR0A = 194;		//40 = 16MHz / (2 * 1024 * (194 + 1) )*/
}

int main(void)
{
	DDRB = 0b00111111;
	init_Timer0();

	sei();

    while (1) 
    {
		
    }
}

