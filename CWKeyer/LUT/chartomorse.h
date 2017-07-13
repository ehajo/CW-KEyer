//Tabelle Kürzen mit to Upper und ins Flash!
// Tabellen fuer CW:

#ifndef _CHARTOMORSE_H_
#define _CHARTOMORSE_H_

#include <stdint.h>
#include <avr/pgmspace.h>

typedef struct 
{
	uint8_t morsecode;
	uint8_t lenght;
} morsechar_t;

morsechar_t CharToMorse(char InputChar);
	
#endif
		
		
		