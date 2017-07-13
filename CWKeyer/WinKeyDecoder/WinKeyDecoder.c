/*
 * WinKeyDecoder.c
 *
 * Created: 09.06.2014 15:48:38
 *  Author: calm
 */ 

#include "./WinKeyDecoder.h"
#include "../RingBuffer/fifo.h"
#include "../cwkeyer.h"
#include "../Enums/Enums.h"
#include "../MorseFSM/MorseFSM.h"
#include "../Structs/Structs.h"
#include <util/crc16.h>

#define CRC16_EEPROM_ADDR ((uint16_t)(E2END-sizeof(int16_t)+1)) //CRC16 for our Config

extern void usb_puts(unsigned char x);
extern void SetWPM(uint8_t Value);

volatile WinKey_FSM_State current_state = {.state=ANFANG, .parameter=0};

volatile WinKeyConfig_t DefaultConfiguration; //Used by CMD 0x0F

//Okay we need to Emulate the EEPROM
#define Winkey_EEPROM (sizeof(WinKeyConfig_t)) //Emulates the Winkey EEPROM Startadress

uint8_t WinkeyEEPROMread(uint16_t Index)
{
	return eeprom_read_byte((const uint8_t *)(Index+Winkey_EEPROM));
}

void WinkeyEEPROMwrite(uint16_t Index, uint8_t data)
{
	eeprom_update_byte((uint8_t *)(Index+Winkey_EEPROM), data);
}

void InitWinkeyDecoder()
{
	load_configuration();	
}

void load_configuration()
{
	//Read Status of Config....
	WinKeyConfig_t wparam;
	uint16_t stored_crc16=eeprom_read_word((const uint16_t *)(CRC16_EEPROM_ADDR));
	uint8_t* ptr=(uint8_t *)(&wparam);
	uint16_t lenght=sizeof(WinKeyConfig_t);
	for(uint16_t i=0;i<lenght;i++)
	{
		
		*ptr = eeprom_read_byte((const uint8_t *)i);
		ptr++;
	}
	
	//okay now we need to build a crc16-ccitt style
	//as our last 2 bytes are crc we now run to lenght-2
	uint16_t crc_calc=0xFFFF;
	ptr=(uint8_t *)(&wparam);
	for(uint16_t i=0;i<(lenght);i++)
	{
		
		crc_calc = _crc_ccitt_update(crc_calc,*ptr);
		ptr++;
	}
	//Check if CRC matches
	if(crc_calc==stored_crc16)
	{
		//Alles okay
		asm("NOP");
	}
	else
	{
		//Need to Build Config with default values
		
		wparam.WPM = 10;
		wparam.Moderegister = 0;
		wparam.DitDahRatio = 50;		
		
		//Okay now we need to Build our CRC
		ptr=(uint8_t *)(&wparam);
		crc_calc=0xFFFF;
		for(uint16_t i=0;i<(lenght);i++)
		{
			
			crc_calc = _crc_ccitt_update(crc_calc,*ptr);
			ptr++;
		}
		
		ptr=(uint8_t *)(&wparam);
		//And now everything must back to the EEPROM
		for(uint16_t i=0;i<lenght;i++)
		{
			eeprom_update_byte((uint8_t *)i , *ptr);
			ptr++;
		}
		eeprom_update_word((uint16_t *)CRC16_EEPROM_ADDR,crc_calc);
	}

	//Send Config to FSM
	Morse_FSM_Update_Config(&wparam);
}

void save_configuration(WinKeyConfig_t* wparam)
{
	//Okay now we need to Build our CRC
	uint8_t* ptr=(uint8_t *)(wparam);
	uint16_t crc_calc=0xFFFF;
	uint16_t lenght=sizeof(WinKeyConfig_t);
	for(uint16_t i=0;i<(lenght);i++)
	{
		
		crc_calc = _crc_ccitt_update(crc_calc,*ptr);
		ptr++;
	}
	
	ptr=(uint8_t *)(wparam);
	//And now everything must back to the EEPROM
	for(uint16_t i=0;i<lenght;i++)
	{
		eeprom_update_byte((uint8_t *)i , *ptr);
		ptr++;
	}
	eeprom_update_word((uint16_t *)CRC16_EEPROM_ADDR,crc_calc);
};

volatile uint8_t unhandeld_data;
volatile uint16_t eeprom_ptr=0;
enum post_processing Decoder(uint8_t ReceivedByte, fifo_t* USARTtoUSB_Buffer)
{
	enum post_processing post_action=NOP;
	switch(current_state.state)
	{
		case ANFANG:
		// Lets Check if this if for Output
		if(ReceivedByte>0x17)
		{
			if(insert_keybyte(ReceivedByte) == 0)	// Wenn kein Platz mehr im Ringpuffer war
			{
				fifo_put(USARTtoUSB_Buffer, 0b11000000); //Buffer full
			}
		}
		else //Okay seems we go a command here....
		{
			switch (ReceivedByte)
			{
				case 0x00: // Adminbefehl
				{
					current_state.state = ADMIN_CMD;
					
				}
				break;
				
				case 0x01:	// Seitenton
				{
					current_state.state = SIDETONE;
					
				}
				break;
				
				case 0x02: // Setze WPM-Geschwindigkeit
				{
					current_state.state = WPM_SPEED;
				
				}
				break;
				case 0x03: // Setze Gewichtung
				{
					current_state.state = WEIGHT;
					
				}
				break;
				case 0x04:	// Setze PTT Zeiten
				{
					current_state.parameter = 2;
					current_state.state = PTT_TIME;
					
				}
				break;
				case 0x05:	// Potieinstellung
				{
					current_state.parameter = 3;
					current_state.state = SPEEDPOTI;
					
				}
				break;
				case 0x06:	// Pause
				{
					current_state.state = PAUSE;
				}
				break;
				case 0x07:	// Potistatus
				{
					//  Direkte Verarbeitung hier. Poti nicht supportet
					fifo_put(USARTtoUSB_Buffer,0x10);
					current_state.state = ANFANG;
				
				}
				break;
				case 0x08:	// Backup input buffer
				{
					// Ignored
				}
				break;
				case 0x09:	// Setze PINCFG Register
				{
					current_state.state = PINCFG;
				
				}
				break;
				case 0x0A:	// Loesche Puffer
				{
					Morse_FSM_Empty_Ascii_Buffer();
				}
				break;
				case 0x0B:	// Dauerton ausgeben
				{
					current_state.state = KEY_IMMEDIATE;
				}
				break;
				case 0x0C:	// Setze HSCW
				{
					current_state.state = HSCW;
				}
				break;
				case 0x0D:	// Setze WPM
				{
					current_state.state = SET_WPM;
				}
				break;
				case 0x0E:	// Setze Keymode
				{
					//Also setzs Modebits at once...
					current_state.state = SET_KEYMODE;
				}
				break;
				case 0x0F:	// Setze alle Werte auf einmal
				{
					current_state.parameter = 15;
					current_state.state = LOAD_ALL;
				}
				break;
				case 0x10:	// Verlaengerung des ersten Zeichens
				{
					current_state.state = EXTENSION;
				}
				break;
				case 0x11:	// Key-Kompensation, Verlaengerung aller Zeichen
				{
					current_state.state = KEY_COMP;
				}
				break;
				case 0x12:	// Setze Paddle Umschaltzeit
				{
					current_state.state = PADDLE_SP;
				}
				break;
				case 0x13:	// NOP
				{
				
				}
				break;
				case 0x14:	// Software paddle
				{
					current_state.state = SW_PADDLE;
				}
				break;
				case 0x15: // Keyer-Status <- Buffer full or not :-)
				{
					uint8_t status = 0b11000000;
					//Calculate the Keyerstatus what we need:
					/*
					Bit 7 = 1
					Bit 6 = 1
					Bit 5 = 0
					Bit 4 = WAIT -> if 1 WK waits for an internal event to finish
					Bit 3 = KEYDOWN / Tune
					Bit 2 = Keyer is Busy sending Morse
					Bit 1 = BREAKIN / Paddle Breakin active = 1
					Bit 0 = XOFF <- Buffer more then 2/3 full when 1
					*/
					if(Morse_FSM_Get_Buffer_Free()<42)
					{
						status |=0x00000001;
					}
					//Is System transmitting?
					//status |= Morse_FSM_Transmitting()<<2;
					
					
					fifo_put(USARTtoUSB_Buffer, status); //Buffer full
					current_state.state=ANFANG;
				}
				break;
				case 0x16:	// Eingabepuffer manipulieren
				{
					current_state.state = POINTER_CMD1;
				}
				break;
				case 0x17:	// Dit-Dah Verhaeltnis
				{
					current_state.state = DD_RATIO;
				}
				break;
				default:
				{
					//Not recognized what ever this is....
					asm("NOP");
					unhandeld_data=ReceivedByte;
					current_state.state=ANFANG;
				}
				break;
			}
			
		}
		break;
		case ADMIN_CMD:
		{
			switch(ReceivedByte)
			{
				case 0x00:
				{
					//We will get <0x00> <0x00> [100ms Pause] <0xFF>
					current_state.state =CALIBRATE;
				}
				break;
				
				case 0x01:
				{
				wdt_enable(WDTO_250MS);
				while(1)
				{}
				}
				break;
				case 0x02:
				{
					fifo_put(USARTtoUSB_Buffer, 0x16);
					current_state.state =ANFANG;
				}
				break;
				case 0x04:
				{
					current_state.state = ECHO_LOOPBACK;
				}
				break;
				
				case 0x05:
				{
				current_state.state =ANFANG;
				fifo_put(USARTtoUSB_Buffer,0x00);
				}
				break;
				
				case 0x06:
				{
				current_state.state =ANFANG;
				fifo_put(USARTtoUSB_Buffer,0x00);
				}
				break;
				
				case 0x07:
				{
				current_state.state =ANFANG;
				fifo_put(USARTtoUSB_Buffer,0x00);
				}
				break;
				
				case 0x08:
				{
				current_state.state =ANFANG;
				fifo_put(USARTtoUSB_Buffer,0x00);
				}
				break;
				
				case 0x09:
				{
				current_state.state =ANFANG;
				fifo_put(USARTtoUSB_Buffer,0x00);
				}
				break;
				
				case 0x0A: //Set WK1 Mode (Disabels pushbotton reporting)
				{
				current_state.state =ANFANG;
				}
				break;
				
				
				case 0x0B:
				{
				current_state.state =ANFANG;	
				}
				break;
				
				case 0x0C:
				{
					//Needs to Dump EEPROM (256 Byte)
					//The decoder is modified and can inform a post-pocessing 
					post_action=DUMP_EEPROM_CONFIG;
					current_state.state = ANFANG;
					
				}		
				break;
				
				case 0x0D:
				{
					//Here we should load 256 Byte into our eeprom	
					current_state.state=LOAD_EEPROM; //There should follow 256 Byte Data
					eeprom_ptr=0;
				}	
				break;
				
				case 0x0E: //Send Standalone Message
				{
					current_state.state = ANFANG;
				}
				break;
				
				case 0x0F: //Load X1MODE
				{
					current_state.state = ANFANG;
				}
				break;
				
				case 0x10: //Firnwareupdate and will init the Firmwareupload
				{
						current_state.state = ANFANG;
				}
				break;
				
				case 0x11: //Set High Baudrate
				{
					current_state.state = ANFANG;
				}
				break;
				
				case 0x12: //Set Low Baudrate
				{
					current_state.state = ANFANG;
				}
				break;
				
				case 0x13: //Set K1EL Antenna Switch
				{
					current_state.state = ANFANG;
				}
				break;
				
				case 0x14: //Set WK3 Mode
				{
					current_state.state = ANFANG;
				}
				break;
				
				case 0x15: //Return Powersupplyvolateg
				{
					current_state.state = ANFANG;
					//Return the Powersupplyvoltage, here hardcoded to 5V
					fifo_put(USARTtoUSB_Buffer,0x52); //almost 5Volt
				}
				break;
				
				case 0x16: //Load X2MODE
				{
					current_state.state = ANFANG;	
				}
				break;
				
				//End of Admincommands
				default:
				current_state.state = ANFANG;
				break;
				
			}
			
		}
		break;
		case SIDETONE:
		{
			
			//Winkey 1 and 2 Mode only
			//Bit 7 = Enable Paddle Only Sidetone when =1 
			//Bit 6-4  Unused set to zero
			//Bit 3-0 Sidetone frequency N with
			/*
				0x1 4000Hz
				0x2 2000Hz
				0x3 1333Hz
				0x4 1000Hz
				0x5 800Hz
				0x6 666Hz
				0x7 571Hz
				0x8 500Hz
				0x9 444Hz
				0xa 400Hz
				
			*/
			
			//Winkey 3 Mode
			// Frequency = 62500/byte
			//Paddleonly sidetone is at X2MODE Register
			
			
			
		}
		break;
		case WPM_SPEED:
		{
			//eeprom_write_byte(&wpm_speed_EEPROM, ReceivedByte+6);
			//speedconfig = 1200l / ((uint16_t)ReceivedByte+6l);
			DefaultConfiguration.WPM=ReceivedByte;
			SetWPM(ReceivedByte);
			save_configuration((WinKeyConfig_t*)(&DefaultConfiguration));
			current_state.state = ANFANG;
			
		}
		break;
		case PTT_TIME:
		{
			switch(current_state.parameter)
			{
				case 2:
				current_state.parameter = 1;
				break;
				case 1:
				default:
				current_state.parameter = 0;
				current_state.state = ANFANG;
				break;
			}
			
		}
		break;
		case SPEEDPOTI:
		{
			switch(current_state.parameter)
			{
				case 3:
				current_state.parameter = 2;
				break;
				case 2:
				current_state.parameter = 1;
				break;
				case 1:
				default:
				current_state.parameter = 0;
				current_state.state = ANFANG;
				break;
			}
			
		}
		break;
		case SET_KEYMODE:
		{
			//eeprom_write_byte(&winkey_mode_EEPROM, ReceivedByte);
			
			DefaultConfiguration.Moderegister=ReceivedByte;
			save_configuration((WinKeyConfig_t*)(&DefaultConfiguration));
			
			//Okay our KexMode tells us the following
			/*
			Bit 7 - Disable Paddle Watchdog
			Bit 6 - Paddle Echoback (1 = Enabled, 0 = Disabled)
			Bit 5 - \_ Keymode: 00 = lambc B		, 01 = llambic A
			Bit 4- /                  10 = Ultimatic		, 11 = Bug Mode
			Bit 3 - Paddleswap
			Bit 2 - Serial Echo Back
			Bit 1 - Autospace
			Bit 0 - CT Spacing = 1, Normal Wordspace when = 0
			*/
			Morse_FSM_Update_Config((WinKeyConfig_t*)(&DefaultConfiguration));
			current_state.state = ANFANG;
			
		}
		break;
		case SW_PADDLE:
		{
			switch(ReceivedByte)
			{
				case 0x01:	// dit
				//SW command for a DIT what shall we do with the stuff in our Buffer?
				Morse_FSM_Force_Dit();
				break;
				case 0x02:	// dah
				//SW command for a DIT what shall we do with the stuff in our Buffer?
				Morse_FSM_Force_Dah();
				break;
				default:	// Alles andere kann er nicht
				break;
			}
			
		}
		break;
		case LOAD_ALL:
		{
			switch(current_state.parameter)
			{
				case 15:
				DefaultConfiguration.Moderegister=ReceivedByte;
				current_state.parameter = 14;
				break;
				case 14:	// Geschwindigkeit
				DefaultConfiguration.WPM=ReceivedByte;
				current_state.parameter = 13;
				break;
				case 13:
				DefaultConfiguration.Sidetone=ReceivedByte;
				current_state.parameter = 12;
				break;
				case 12:
				DefaultConfiguration.Weight=ReceivedByte;
				current_state.parameter = 11;
				break;
				case 11:
				DefaultConfiguration.LeadInTime=ReceivedByte;
				current_state.parameter = 10;
				break;
				case 10:
				DefaultConfiguration.TailTime=ReceivedByte;
				current_state.parameter = 9;
				break;
				case 9:
				DefaultConfiguration.MinWPM=ReceivedByte;
				current_state.parameter = 8;
				break;
				case 8:
				DefaultConfiguration.WPMRange=ReceivedByte;
				current_state.parameter = 7;
				break;
				case 7:
				DefaultConfiguration.X2Mode=ReceivedByte; //In WK2 Mode this is 1st Extension
				current_state.parameter = 6;
				break;
				case 6:
				DefaultConfiguration.KeyCompensation=ReceivedByte;
				current_state.parameter = 5;
				break;
				case 5:
				DefaultConfiguration.FarnsworthWPM=ReceivedByte;
				current_state.parameter = 4;
				break;
				case 4:
				DefaultConfiguration.PaddleSetPoint=ReceivedByte;
				current_state.parameter = 3;
				break;
				case 3:
				DefaultConfiguration.DitDahRatio=ReceivedByte;
				current_state.parameter = 2;
				break;
				case 2:
				DefaultConfiguration.PinConfiguration=ReceivedByte;
				current_state.parameter = 1;
				break;
				case 1:
				DefaultConfiguration.X1Mode=ReceivedByte;	//IN WK2 Mode this is Don't Care and set to '0'
				current_state.parameter = 0;
				current_state.state = ANFANG;
				save_configuration((WinKeyConfig_t*)(&DefaultConfiguration));
				break;
				default:
				current_state.parameter = 0;
				current_state.state = ANFANG;
				break;
			}
			
		}
		break;
		case LOAD_EEPROM:
		{
			WinkeyEEPROMwrite(eeprom_ptr++,ReceivedByte);
			if(eeprom_ptr>255) //Last Byte recived
			{
				current_state.state=ANFANG;
			}
			
		}
		break;
		case ECHO_LOOPBACK:
		{
			fifo_put(USARTtoUSB_Buffer, ReceivedByte); //Echo Back the Byte
			current_state.state=ANFANG;
		}
		break;
		
		case PINCFG:
		{
			//Okay we ignore what we recive
			current_state.state=ANFANG;
				
		}
		break;
		
		case CALIBRATE:
		{
			//Okay we need to get 0x00 , 0xFF
			if(ReceivedByte==0xFF)
			{
				current_state.state=ANFANG;
			}
			else
			{
				current_state.state=CALIBRATE;
			}
		}
		break;
		
		default:
		current_state.state=ANFANG;
		break;
	}
	
	return post_action;
}

