/*
 * Occlusion_test_002.c
 *
 * Created: 27-01-2022 15:19:55
 * Author : Nitin Patil
 */ 

 #define F_CPU 4000000UL
 #include "avr/interrupt.h"
 #include "UART_1_AVR128DA64.h"
 #include "ADC_AVR128DA64.h"
 #include <avr/io.h>
 #include <util/delay.h>
 #include <math.h>
 #include <stdio.h>
 #include <string.h>
 #include <stdlib.h>
 #include <stdbool.h>
 #define channel_3 0x03
 #define start_token 0x03
 #define end_token 0xFC
 
 #define _60ml_occlusion 1800
 #define _20ml_occlusion 1100
 #define _10ml_occlusion 800
 #define _5ml_occlusion 500

 float syringe_dia = 0.0;
 unsigned long  Loadcell_Value=0.0;
 float M_Weight = 0.0;
 float wt = 0.0;
 unsigned long adc0 = 0;
 float sum;
 int adc;
 int syringe= 0;
 float Avg;

 //int syringe_detected;

 bool syringe_detected = false, b_flag = false;
 long first_val = 0;
 bool first = true, run_ok_syringe = false;

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

	 for (i=0;i<24;i++) //24 bit
	 {
		 PORTA.OUT |= (1 << 3);
		 _delay_us(5);
		 Count = Count<<1;

		 PORTA.OUT &= ~(1 << 3);
		 _delay_us(5);

		 if((PORTA.IN & (1 << 2)))
		 {
			 Count++;
		 }
	 }
	 PORTA.OUT |= (1 << 3);
	 Count = Count^0x800000;
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

 void syringeforward(void) //motor on
 {
	 PORTA.OUTCLR |= PIN6_bm;
	 PORTA.OUTSET |=(1<<4);
	 _delay_ms(20);
	 PORTA.OUTCLR |=(1<<4);
	 _delay_ms(20);
 }

 void Syringeback(void)// motor back
 {
	 PORTA.OUTSET |= PIN6_bm ;
	 PORTA.OUTSET |=(1<<4);
	 _delay_ms(20);
	 PORTA.OUTCLR |=(1<<4);
	 _delay_ms(20);
 }

 /*****************************syringe****************************************************************/

 void ok_syringe(void) //which syringe is detected 60ml,20ml
 {
	 for(int i=1; i<=10; i++)
	 {
		 adc = ADC0_read(channel_3);
		 sum=sum+adc;
	 }
	 sum = sum/10.00;
	 syringe_dia = 0.00848527*(sum)+7.717863;
	 USART1_sendFloat(syringe_dia, 2);
	 
	 if (syringe_dia > 29.0)
	 {
		 USART1_sendString("60 ML detected\n");
	 }
	 else if (syringe_dia < 23.0 )
	 {
		 USART1_sendString("20 ML detected\n");
	 }
	 else if (syringe_dia < 17.0)
	 {
		 USART1_sendString("10 Ml detected\n");
	 }
	 else if( syringe_dia < 12.6)
	 {
		 USART1_sendString("5 Ml detected\n");
	 }
 }

 /*********************************************************************************************/

 ISR(PORTC_PORT_vect)
 {
	 if(PORTC.INTFLAGS & (1 << 7))
	 {
		 if (syringe_detected)
		 {
			 syringe_detected = false;
			 USART1_sendString("Waiting for Syringe\n");
		 }
		 else
		 {
			 syringe_detected = true;        //flag for Syringe button
			 ok_syringe();
		 }
		 PORTC.INTFLAGS |= (1<<7);
		 
	 }
 }

 /****************************syringe check init*******************************************************/

 void syringe_check_init(void)
 {
	 if(!(PORTC.IN & PIN7_bm))
	 {
		 syringe_detected = false;
	 }
	 else if ((PORTC.IN & PIN7_bm))
	 {
		 syringe_detected = true;
	 }
	 USART1_sendInt(syringe_detected);
 }


 /***********************occlusion check******************************************************/

 void Occlusion_check(float Avg) // occlusion check 60ml,20ml,10ml,5ml syringe
 {	 if((syringe == 60) && ( Avg > _60ml_occlusion))
	 {
		 USART1_sendString("occlusion_detected");
	 }
	 else if((syringe == 20) && (Avg > _20ml_occlusion))
	 {
		 USART1_sendString("occlusion_detected");
	 }
	 else if((syringe == 10) && (Avg > _10ml_occlusion))
	 {
		 USART1_sendString("occlusion_detected");
	 }
	 else if((syringe == 5) && (Avg > _5ml_occlusion))
	 {
		 USART1_sendString("occlusion_detected");
	 }
 }

 /******************************gpio init*****************************************************/
 void gpio_init(void) 
 {
	 PORTA.DIR |= PIN4_bm | PIN6_bm; // Set the direction PIN4,PIN5,PIN6
	 PORTA.DIR &= ~(1 << 2); //DT AS INPUT
	 PORTA.DIR |= (1 << 3);  // SCK AS OUTPUT
	// PORTC.DIR &= ~(1 << 7);
	 //PORTC.PIN7CTRL |= (0x1 << 0);//Interrupt enabled with sense on both edges
	 //PORTC.PIN7CTRL |=(0x1 << 0) || (1<<3);
	 PORTC.PIN7CTRL |= (0x02<<0); //Interrupt enabled with Rising edges
 }

 /******************************************loadcell******************************************/
 float Loadcell(void) //fun with no argument but return value
 {
	 float Avg=0.0;
	 //USART1_sendInt(ReadCount());
	 
	 for(int i = 0; i<5 ; i++)
	 {
		 M_Weight = (float)((ReadCount()- 8417865) * 0.009328283582089); //WEIGHT SCALES
		 Loadcell_Value +=  M_Weight;
	 }
	 Avg = Loadcell_Value/5; //average value
	 //Loadcell_Value -=350;
	 USART1_sendFloat(Avg,2);
	 return Avg;
 }

 void motoroff(void)
 {
	 PORTA.OUTCLR &= ~PIN5_bm;
 }

 int main(void)
 {
	 sei();
	 USART1_init(9600);
	 ADC0_init();
	 gpio_init();
	 syringe_check_init();
	 while(1)
	 {
		 syringeforward();
		 //syringe_dia();
// 		 if (run_ok_syringe)
// 		 {
// 			 ok_syringe();
// 			 run_ok_syringe = false;
// 		 }
		 Loadcell();
		 Occlusion_check(Avg);
		 motoroff();
	 }
 }

 