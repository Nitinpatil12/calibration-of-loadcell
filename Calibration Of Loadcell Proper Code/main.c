#define F_CPU 4000000UL
#include <avr/io.h>
#include <util/delay.h>
#include "avr/interrupt.h"
#include "UART_1_AVR128DA64.h"
#include "util/delay.h"
#include "ADC_AVR128DA64.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#define _60ml_occlusion  1800
#define _20ml_occlusion  1100
// #define _10ml_occlusion  500
// #define _5ml_occlusion  200

float syringe_dia = 0.0;
float sum;
int adc;
int syringe= 0;
int syringe_detected;

bool first = true;
unsigned long  Loadcell_Value=0;
float M_Weight = 0.0;
float wt = 0.0;
unsigned long adc0 = 0;

/************************************************************************/
/* Loadcell */
/************************************************************************/

unsigned long ReadCount(void)
{
	unsigned long Count;
	unsigned char i;

	PORTA.OUT &= ~(1 << 3); // SCK AS LOW
	Count=0;
	while((PORTA.IN & (1 << 2)));

	for (i=0;i<24;i++)
	{
		PORTA.OUT |=(1 << 3);
		_delay_us(5);
		Count=Count<<1;

		PORTA.OUT &= ~(1 << 3);
		_delay_us(5);

		if((PORTA.IN & (1 << 2)))
		{
			Count++;
		}
	}
	PORTA.OUT |= (1 << 3);
	Count=Count^0x800000;
	PORTA.OUT &= ~(1 << 3);
	return(Count);
}

/************************************************************************/
/* Motor driving functions                                             */
/************************************************************************/
void motor_single_step(void)
{
	PORTA.OUTSET = PIN4_bm;
	_delay_ms(50);
	PORTA.OUTCLR = PIN4_bm;
	_delay_ms(50);
}

void syringeforward(void)
{
	PORTA.OUTCLR |= PIN6_bm;
	PORTA.OUTSET |=(1<<4);
	_delay_ms(20);
	PORTA.OUTCLR |=(1<<4);
	_delay_ms(20);
}

void Syringeback(void)
{
	PORTA.OUTSET |= PIN6_bm ;
	PORTA.OUTSET |=(1<<4);
	_delay_ms(20);
	PORTA.OUTCLR |=(1<<4);
	_delay_ms(20);
}

void motoroff(void)
{
	PORTA.OUTCLR &= ~PIN5_bm;
}
		

 ISR(PORTC_PORT_vect)
 {
 	if(PORTC.INTFLAGS & (1 << 7)) // 
 	{
 		if (syringe_detected)
 		{
 			syringe_detected = false;
 			USART1_sendString("Waiting for Syringe\n");
 		}
 		else
 		{
			syringe_detected = true;
			//syringe();
		}
		PORTC.INTFLAGS |= (1<<7);
	}
}
 
void syringe_check_init(void) // first check syringe connected or not
 {
	if(PORTC.IN & PIN7_bm)
	{
		syringe_detected = false;
	}
	else if (!(PORTC.IN & PIN7_bm))
	{
		syringe_detected = true;
	}
	USART1_sendInt(syringe_detected);
 }
 

 void ok_syringe(void) //which syringe is detected 60ml,20ml,10ml,5ml
 {

	 for(int i=1; i<=10; i++)
	 {
		 adc = ADC0_read(channel_3);
		 sum=sum+adc;
	 }
	 sum = sum/10.00;
	 syringe_dia = 0.00848527*(sum)+7.717863;

	 if (syringe_dia < 12.6)
	 {
		 USART1_sendString("5 ML detected\n");
		 syringe=5;
	 }
	 else if (syringe_dia < 17.0 )
	 {
		 USART1_sendString("10 ML detected\n");
		 syringe=10;
	 }
	 else if (syringe_dia < 23.0)
	 { 
		 USART1_sendString("20 Ml detected\n");
		 syringe= 20;
	 }
	 else if( syringe_dia > 29.0)
	 {
		 USART1_sendString("60 Ml detected\n");
		 syringe= 60;
	 }
 }
							
void Occlusion_check(void)
{
	if((syringe == 60) && ( wt > _60ml_occlusion))
	{
		USART1_sendString("occlusion_detected");
	}
	else if((syringe == 20) && ( wt > _20ml_occlusion))
	{
		USART1_sendString("occlusion_detected");
	}
	else if((syringe == 10) && ( wt > _10ml_occlusion))
	{
		USART1_sendString("occlusion_detected");
	}
	else if((syringe == 5) && ( wt > _5ml_occlusion))
	{
		USART1_sendString("occlusion_detected");
	}
}

int main(void)
{
	sei();
	//unsigned long val;
	USART1_init(9600);
	Occlusion_check();
	PORTA.DIR |= PIN4_bm | PIN5_bm | PIN6_bm;
	PORTA.DIR &= ~(1 << 2); //DT AS INPUT
	PORTA.DIR |= (1 << 3);  // SCK AS OUTPUT
	while(1)
	{
		USART1_sendInt(ReadCount());
		_delay_ms(1);
     	M_Weight = (float)((ReadCount() - 8409443 ) * 0.0093283582089552);
//         for(int i=1;i<=50;i++)
//      	{
//    			wt += M_Weight;
//      	}
//         wt = wt/50.0;
		
	   for(int i = 0; i<5 ; i++)
	   {
		   Loadcell_Value += ReadCount();
	   }
	   Loadcell_Value = Loadcell_Value/5;

	   USART1_sendInt(Loadcell_Value);
  	    wt -= 309.0;
   	   USART1_sendFloat((wt) , 2);
		/*USART1_sendFloat(abs(wt) , 2); 		*/
   }
}
