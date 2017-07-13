#include "chartomorse.h"

//Offset zur ASCI tabelle ist 33
//Also '!' ist hier null daher aller werte einmal -'!' nehmen
//und '!' als untere grenze für den Lookup
//Asci 32 (SPACE) hat eine besondere Bedeutung
#define FIRSTLUTCHAR '!'

const morsechar_t morsetable[64] PROGMEM = {	
								
								{.morsecode=0b00110101,.lenght=6},	//ASCII '!'
								{.morsecode=0b00010010,.lenght=6}, 	//ASCII '"'
								{.morsecode=0b00000000,.lenght=0}, 	//ASCII '#'
								{.morsecode=0b01001000,.lenght=7}, 	//ASCII '$'
								{.morsecode=0b00000000,.lenght=0}, 	//ASCII '%'
								{.morsecode=0b00000010,.lenght=5}, 	//ASCII '&'
								{.morsecode=0b00011110,.lenght=6}, 	//ASCII '''
								
								//40
								{.morsecode=0b00001101,.lenght=5}, 	//ASCII '('
								{.morsecode=0b00101101,.lenght=6}, 	//ASCII ')'
								{.morsecode=0b00000000,.lenght=0}, 	//ASCII '*'
								{.morsecode=0b00001010,.lenght=5}, 	//ASCII '+'
								{.morsecode=0b00110011,.lenght=6}, 	//ASCII ','
								{.morsecode=0b00100001,.lenght=6}, 	//ASCII '-'
								{.morsecode=0b00101010,.lenght=6}, 	//ASCII '.'
								{.morsecode=0b00001001,.lenght=5}, 	//ASCII '/'
								//48
								{.morsecode=0b00011111,.lenght=5}, 	//ASCII '0'
								{.morsecode=0b00011110,.lenght=5}, 	//ASCII '1'
								{.morsecode=0b00011100,.lenght=5}, 	//ASCII '2'
								{.morsecode=0b00011000,.lenght=5}, 	//ASCII '3'
								{.morsecode=0b00010000,.lenght=5}, 	//ASCII '4'
								{.morsecode=0b00000000,.lenght=5}, 	//ASCII '5'
								{.morsecode=0b00000001,.lenght=5}, 	//ASCII '6'
								{.morsecode=0b00000011,.lenght=5}, 	//ASCII '7'
								//56
								{.morsecode=0b00000111,.lenght=5}, 	//ASCII '8'
								{.morsecode=0b00001111,.lenght=5}, 	//ASCII '9'
								{.morsecode=0b00000111,.lenght=6}, 	//ASCII ':'
								{.morsecode=0b00010101,.lenght=6}, 	//ASCII ';'
								{.morsecode=0b00000000,.lenght=0}, 	//ASCII '<'
								{.morsecode=0b00010001,.lenght=5}, 	//ASCII '='
								{.morsecode=0b00000000,.lenght=0}, 	//ASCII '>'
								{.morsecode=0b00001100,.lenght=6}, 	//ASCII '?'
								//64
								{.morsecode=0b00011010,.lenght=7}, 	//ASCII '@'
								{.morsecode=0b00000010,.lenght=2}, 	//ASCII 'A'
								{.morsecode=0b00000001,.lenght=4}, 	//ASCII 'B'
								{.morsecode=0b00000101,.lenght=4}, 	//ASCII 'C'
								{.morsecode=0b00000001,.lenght=3}, 	//ASCII 'D'
								{.morsecode=0b00000000,.lenght=1}, 	//ASCII 'E'
								{.morsecode=0b00000100,.lenght=4}, 	//ASCII 'F'
								{.morsecode=0b00000011,.lenght=3}, 	//ASCII 'G'
								//72
								{.morsecode=0b00000000,.lenght=4}, 	//ASCII 'H'
								{.morsecode=0b00000000,.lenght=2}, 	//ASCII 'I'
								{.morsecode=0b00001110,.lenght=4}, 	//ASCII 'J'
								{.morsecode=0b00000101,.lenght=3}, 	//ASCII 'K'
								{.morsecode=0b00000010,.lenght=4}, 	//ASCII 'L'
								{.morsecode=0b00000011,.lenght=2}, 	//ASCII 'M'
								{.morsecode=0b00000001,.lenght=2}, 	//ASCII 'N'
								{.morsecode=0b00000111,.lenght=3}, 	//ASCII 'O'
								//80
								{.morsecode=0b00000110,.lenght=4}, 	//ASCII 'P'
								{.morsecode=0b00001011,.lenght=4}, 	//ASCII 'Q'
								{.morsecode=0b00000010,.lenght=3}, 	//ASCII 'R'
								{.morsecode=0b00000000,.lenght=3}, 	//ASCII 'S'
								{.morsecode=0b00000001,.lenght=1}, 	//ASCII 'T'
								{.morsecode=0b00000100,.lenght=3}, 	//ASCII 'U'
								{.morsecode=0b00001000,.lenght=4}, 	//ASCII 'V'
								{.morsecode=0b00000110,.lenght=3}, 	//ASCII 'W'
								//88
								{.morsecode=0b00001001,.lenght=4}, 	//ASCII 'X'
								{.morsecode=0b00001101,.lenght=4}, 	//ASCII 'Y'
								{.morsecode=0b00000011,.lenght=4}, 	//ASCII 'Z'
								{.morsecode=0b00000000,.lenght=0}, 	//ASCII '['
								{.morsecode=0b00000000,.lenght=0}, 	//ASCII '\'
								{.morsecode=0b00000000,.lenght=0}, 	//ASCII ']'
								{.morsecode=0b00000000,.lenght=0}, 	//ASCII '^'
								
								//Last Line
								{.morsecode=0b00001101,.lenght=6} 	//ASCII '_'								
							};
							
morsechar_t CharToMorse(char Inputchar)
{
	morsechar_t returnvalue={.morsecode=0x00,.lenght=0}; //Default case for failed lookup
	
	//Calculate input offset
	if((Inputchar<FIRSTLUTCHAR)||(Inputchar>'z'))
	{
		//Okay Char can't be looked up, not in Table and therefore no morsechar
	}
	else
	{
		//Lets see if an ToUpper is requiered
		if((Inputchar>='a')&&(Inputchar<='z'))
		{
			//Okay to Upper reqiered
			Inputchar-=('a'-'A');
		}
		returnvalue.morsecode	= pgm_read_byte(&(morsetable[Inputchar-FIRSTLUTCHAR].morsecode));
		returnvalue.lenght		= pgm_read_byte(&(morsetable[Inputchar-FIRSTLUTCHAR].lenght));
	}
	return returnvalue;
}
