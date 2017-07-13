/*
 * HW_Def.h
 *
 * Created: 16.06.2014 21:10:27
 *  Author: calm
 */ 


#ifndef HW_DEF_H_
#define HW_DEF_H_

#define KEY1 			(1<<PD5)
#define KEY2 			(1<<PD6)
#define KEYPORT 		PORTD
#define KEYDDR 			DDRD
#define KEYPIN 			PIND
#define PADDLE1			(1<<PD1)
#define PADDLE2			(1<<PD0)
#define PADDLEPORT 		PORTD
#define PADDLEPIN 		PIND
#define PADDLEDDR 		DDRD

#define PADDELBIT1		(( (~PIND) &(1<<PD1))>>PD1)
#define PADDELBIT2		(( (~PIND) &(1<<PD0))>>PD0)

#define	TIMECOUNTERPORT PORTB
#define TIMECOUNTERDDR	DDRB
#define TIMECOUNTERPIN1 (1<<PB0)

#endif /* HW_DEF_H_ */