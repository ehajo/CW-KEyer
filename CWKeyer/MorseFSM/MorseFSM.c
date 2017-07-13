/*
 * MorseFSM.c
 *
 * Created: 09.06.2014 16:11:28
 *  Author: calm
 */ 

#include "MorseFSM.h"
#include "../RingBuffer/fifo.h"
#include "../LUT/chartomorse.h"
#include "../HW_Def/HW_Def.h"



typedef enum {
	IDLE,
	DIT_MACHINE,
	DIT_LOW_MACHINE,
	DIT_HIGH_MACHINE,
	DAH_MACHINE,
	DAH_LOW_MACHINE,
	DAH_HIGH_MACHINE,
	SPACE_MACHINE,
	DO_SPACE_MACHINE,
	DIT_HUMAN,
	DIT_LOW_HUMAN,
	DIT_HIGH_HUMAN,
	DAH_HUMAN,
	DAH_LOW_HUMAN,
	DAH_HIGH_HUMAN,
	BUGMODE_DAH_PRESSED,
	BUGMODE_DAH_RELEASED,
	LETTERPAUSE_MACHINE,
	PAUSE_LETTERS

} FSMstate_t;

typedef enum
{
	PRESSED,
	STILLPRESSED,
	RELEASED
} KeyState_t;

typedef enum
{
	FSM_Automacit,
	FSM_Manuall
	
} FSMmode_t;

static fifo_t FSM_Queue;			// DITs und DAHs für den Automaten
FSMstate_t FSM_Queue_Buffer[16];	//16 Elements Buffer for the FSM
static fifo_t AsciiQueue;			//Die Eingabezeichen die Konvertiert werden müssen
uint8_t AsciiData[128];				//128 Byte Buffer for Ascii Data
FSMstate_t ManualData[64];			//Buffer for the FSM_STATE for Human Input
static fifo_t ManualDataQueue;
volatile uint16_t SwitchBagLevel=0;


volatile FSMmode_t FSM_Mode= FSM_Automacit;
volatile FSMstate_t FSM_State=IDLE;
volatile FSMstate_t FSM_ManualState=IDLE;
volatile uint16_t FSM_ParameterA=0;
volatile uint16_t FSM_ParameterB=0;
volatile enum KeyMode CurrentKeymode=LAMBICB;
volatile uint8_t current_WPM=10; //Per default 1 WPM
fifo_t* EchoBuffer;
volatile uint8_t EchoModeOn=1;

volatile uint8_t PaddleKeyA=0;
volatile uint8_t PaddleKeyB=0;
volatile int16_t TXChar=-1;
volatile uint8_t current_paddle_swap;
volatile KeyState_t KeyA=RELEASED;
volatile KeyState_t KeyB=RELEASED;

volatile uint8_t LAMBICB_EXTENSION=0;
volatile uint8_t ULTIMATIC_EXTENSION=0;

void MorseFSM_Init(fifo_t* EchoBuf)
{
	fifo_init(&FSM_Queue,(uint8_t *)(FSM_Queue_Buffer),sizeof(FSM_Queue_Buffer));
	fifo_init(&AsciiQueue,AsciiData,sizeof(AsciiData));
	fifo_init(&ManualDataQueue,(uint8_t *)(ManualData),sizeof(ManualData));
	//Next what we need to do is to initialize the timer
	//For that we try to get 1kHz FSM frequency assuming we need less then 8000 clocks per run
	TCCR1B |= (1<<WGM12);
	TCCR1B |= (1<<CS10);	// Volle Taktgeschw
	OCR1A = 8000;	// Alle 1ms ein compareint
	EchoBuffer=EchoBuf;
}

void SetEchoMode(uint8_t Em)
{
	EchoModeOn=Em;
}

void Morse_FSM_Start()
{
	// Timer1 macht timeout:
	TIMSK1 |= (1<<OCIE1A);	// Outputcompare A INT ein
}

void Morse_FSM_Stop()
{
	// Timer1 macht timeout:
	TIMSK1 &= ~(1<<OCIE1A);	// Outputcompare A INT aus
}

void Morse_FSM_Update_Config(WinKeyConfig_t* Config)
{
	//Okay we get a new complete Setting
	//okay we got a whole bunch of Winkey Konfig we need to update :-(
	//Config->DitDahRatio			//We sort of Don't support that yet
	//Config->FarnsworthWPM		//We sort of Don't support that yet
	//Config->KeyCompensation		//We sort of Don't support that yet
	//Config->LeadInTime			//We sort of Don't support that yet
	//Config->MinWPM				//We sort of Don't support that yet
	
	uint8_t mode=((Config->Moderegister&0x30)>>4);
	uint8_t pswap=((Config->Moderegister&0x08)>>3);
	uint8_t echo=((Config->Moderegister&0x04)>>2);
	switch(mode)
	{
		case 0:
		{
			CurrentKeymode=LAMBICA;
		}
		break;
		case 1:
		{
			CurrentKeymode=LAMBICB;
		}
		break;
		case 2:
		{
			CurrentKeymode=ULTIMATIC;
		}
		break;
		case 3:
		{
			CurrentKeymode=BUGMODE;
		}
		break;
		default:
		{
			CurrentKeymode=LAMBICA;
		}
		break;
	}
	if(pswap==0)
	{
		current_paddle_swap=NORMAL;
	}
	else
	{
		current_paddle_swap=SWAP;
	}
	if(echo==1)
	{
		EchoModeOn=1;
	}
	else
	{
		EchoModeOn=0;
	}
	//We need to grab from the Moderegister the Keyermode we need to work with
	/*
	BIT7 = Disable PaddleWatchDog
	BIT6 = Paddle echoBack
	Bit5 \______00 = lambic B , 01 = lambic A , 10 = Ultimatic, 11 = BugMode 
	Bit4 /
	Bit3 = Paddleswap (1=Swap, 0=Disabled)
	Bit2 = SerialEchoBack (1=Enabled, 0=Disabled)
	Bit1 = Autospace (1 = Enabled, 0=Disabled)				-- We sort of Don't support that yet
	Bit0 = CT Spacing when=1, normal Wordspace when=0	-- We sort of Don't support that yet
	*/
	
	//Config->PaddleSetPoint		//We sort of Don't support that yet
	//Config->PinConfiguration		//We sort of Don't support that yet
	//Config->Sidetone				//We sort of Don't support that yet
	//Config->TailTime				//We sort of Don't support that yet
	
	current_WPM=Config->WPM;
	
	//current_WPM = 10;
	//EchoModeOn=1;
	//current_paddle_swap=NORMAL;
	//CurrentKeymode=ULTIMATIC;
	
	//Config->WPMRange			//We sort of Don't support that yet
	//Config->X1Mode				//We sort of Don't support that yet
	//Config->X2Mode				//We sort of Don't support that yet
	
}

uint8_t Morse_FSM_Get_Buffer_Free()
{
	return fifo_get_free_count(&AsciiQueue);
}

void Morse_FSM_Empty_All_Buffer()
{
	fifo_reset(&AsciiQueue);
	fifo_reset(&FSM_Queue);
}

void Morse_FSM_Force_Dit()
{
	Morse_FSM_Empty_All_Buffer();
	fifo_put(&FSM_Queue,DIT_MACHINE);
}

void Morse_FSM_Force_Dah()
{
	Morse_FSM_Empty_All_Buffer();
	fifo_put(&FSM_Queue,DAH_MACHINE);
}

uint8_t Morse_FSM_Transmitting()
{
	if(FSM_State!=IDLE)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

void Morse_FSM_Empty_Ascii_Buffer()
{
	fifo_reset(&AsciiQueue);
}

void Morse_FSM_SetWPM(uint8_t new_wpm)
{
	current_WPM=new_wpm;
}

uint8_t insert_keybyte(uint8_t keybyte)
{
	return fifo_put(&AsciiQueue,keybyte);
}

static inline void CalculateTimingDit(volatile uint16_t* ParamA, volatile uint16_t* ParamB)
{
	//Okay we got a WPM setting wich will be translatet in a Dit and Dah Timing
	*ParamA=1200/current_WPM; //Roundingproblem and therfore timingisues will arise .:-(
	*ParamB=1200/current_WPM;
}

static inline void CalculateTimingDah(volatile uint16_t* ParamA, volatile uint16_t* ParamB)
{
	//Okay we got a WPM setting wich will be translatet in a Dit and Dah Timing
	//Okay we got a WPM setting wich will be translatet in a Dit and Dah Timing
	*ParamA=3600/current_WPM;
	*ParamB=1200/current_WPM; //Roundingproblem and therfore timingisues will arise .:-(
	
}

static inline uint16_t CalculateDeltaDitDah(void)
{
	return (2400/current_WPM);
}

static inline void CalculateTimingSpace(volatile uint16_t* ParamA, volatile uint16_t* ParamB)
{
	//Okay we got a WPM setting wich will be translatet in a Dit and Dah Timing
	*ParamA=0;
	*ParamB=8400/current_WPM;
}


static inline uint16_t GetTimingHalfDitHigh(void)  
{
	return (2400/current_WPM);
}

static inline uint16_t GetTimingHalfDahHigh(void)
{
	return (7200/current_WPM);
}


//Set WPM
void SetWPM(uint8_t Value)
{
	if(Value<1)
	{
		current_WPM=1;	
	}
	else if(Value>100)
	{
		current_WPM=100;
	}
	else
	{
		current_WPM=Value;
	}
	
}



void SetPaddleSwap(enum paddle_swap value)
{
	
}

uint8_t insert_FSMData(uint8_t Dat)
{
	morsechar_t Data=CharToMorse(Dat);
	uint8_t FreeSpace=fifo_get_free_count(&FSM_Queue);
	if(Data.lenght>FreeSpace)
	{
		return 0;
	}
	else
	{
		for(uint8_t i=0;i<Data.lenght;i++)
		{
			if((Data.morsecode&0x01)>0)
			{
				//Its a Long so a DAH
				fifo_put(&FSM_Queue, (uint8_t)DAH_MACHINE);
			}
			else
			{
				fifo_put(&FSM_Queue, (uint8_t)DIT_MACHINE);
			}
			Data.morsecode=Data.morsecode>>1;
		}
	}
	return 1;
}



ISR(TIMER1_COMPA_vect) //Currently uses 99,38us runtime
{
TIMECOUNTERPORT &= ~(TIMECOUNTERPIN1); //Set Pin Low
//Small Shine FSM lets do what has to be done....

volatile uint8_t P1=PADDELBIT1;
volatile uint8_t P2=PADDELBIT2;

if(current_paddle_swap==NORMAL)
{
	PaddleKeyA=(PaddleKeyA<<1)|P1;
	PaddleKeyB=(PaddleKeyB<<1)|P2;
}
else
{
	PaddleKeyA=(PaddleKeyA<<1)|P2;
	PaddleKeyB=(PaddleKeyB<<1)|P1;
}

//We need to Check if one or both of the Paddle are used......
/*
What can happen :
1) A paddle is pressed
2) A paddle is still pressed
3) A paddle is released

-> we need to generate a event handeld in our fsm
Possible events are:
	Paddle A pressed
	Paddle A released
	Paddle B pressed
	Paddle B released
	Du to the nature of a keyer we also need a timestamp to see when a paddle was pressed

*/
/*
check if we have some paddleinputs
*/

if(PaddleKeyA==0b01111111) {//PaddleKeyA Pressed
	KeyA=PRESSED;
} else if(PaddleKeyA==0b11111111) {//PaddleKeyA Still Pressed
	KeyA=STILLPRESSED;
} else { //Key not or no more pressed
	KeyA=RELEASED;
}
 
if(PaddleKeyB==0b01111111) {//PaddleKeyB Pressed
	 KeyB=PRESSED;
} else if(PaddleKeyB==0b11111111) {//PaddleKeyB Still Pressed
	KeyB=STILLPRESSED;
} else { //Key not or no more pressed
	KeyB=RELEASED;
}

if((KeyB!=RELEASED)||(KeyA!=RELEASED))
{
	//Okay we change to postwait for at least a DIT....
	FSM_Mode=FSM_Manuall;
	
}

//Okay our FSM has two modes Automaticmode and Manuallmode
if(FSM_Mode==FSM_Manuall)
{
	//Okay we are in automaticmode lets see what has happend...
	if(KeyA==PRESSED) { //We need a DIT here
		//Okay User pressed a DIT that shall be done....
		fifo_put(&ManualDataQueue,(uint8_t)(DIT_HUMAN));
	}
	
	
	if(KeyB==PRESSED) { //We need a mode dependent action
		//Now we need to see what mode we are and what to do
		if(CurrentKeymode!=BUGMODE)
		//User Presses a DAH that shall be done but, if we are currently transmitting a DIT_HIGH we need to extend that and change the Way of the FSM
		{
			if(FSM_ManualState==DIT_HIGH_HUMAN) {
				FSM_ParameterA+=CalculateDeltaDitDah(); //Extend Dit to Dah
			} else {
				fifo_put(&ManualDataQueue,(uint8_t)(DAH_HUMAN));	
			}
		}else {
		//We are in Bugmode .... So DAH key is altered imidatly and overridesy any FSM Action....
		FSM_ManualState=BUGMODE_DAH_PRESSED;
		}
	}
	if((KeyA==STILLPRESSED) && (KeyB==STILLPRESSED))
	{
		//Both Peddels are pressed mode dependent acton requiered
		//IF we are in Bugmode Paddle B wins!
		if(CurrentKeymode==BUGMODE)
		{
			FSM_ManualState=BUGMODE_DAH_PRESSED;
		} else {
			//Okay in labmibic A and B we do ... DIT HAH DIT DAH 
			
			
		}
		
		
	} else {
		if(KeyA==STILLPRESSED)
		{
			
		}
	
		if(KeyB==STILLPRESSED)
		{
					
		}
	}
	if(KeyB==RELEASED)
	{
		if((CurrentKeymode==BUGMODE) && ( FSM_ManualState==BUGMODE_DAH_PRESSED))
		{
			FSM_ManualState=BUGMODE_DAH_RELEASED;
			fifo_reset(&ManualDataQueue);
		}
	}
	


	switch(FSM_ManualState)
	{
		case IDLE:
		{
			//Here we do sort of nothing....
				if(fifo_get_item_count(&ManualDataQueue)!=0)
				{
					FSM_ManualState=(FSMstate_t)fifo_get_nowait(&ManualDataQueue);
					SwitchBagLevel=GetTimingHalfDahHigh()*2;
				}
				else
				{
					SwitchBagLevel--;
					if(SwitchBagLevel==0)
					{
						FSM_Mode=FSM_Automacit;
						Morse_FSM_Empty_All_Buffer();
					}
				}
					
		}
		break;
		
		case BUGMODE_DAH_PRESSED:
		{
			//We don't set a next State...
			KEYOUTPUTHIGH;
		}
		break;
		
		case BUGMODE_DAH_RELEASED:
		{
			KEYOUTPUTLOW;
			FSM_ManualState=IDLE;
		}
		break;
		
		case DIT_HUMAN:
		{
			FSM_ParameterA=0;
			FSM_ParameterB=0; //Keep in Mind for Low time this state adds one cycle to it....
		
			//Okay we got a DIT let's set the OUTPUT for that
			KEYOUTPUTHIGH;
			//Lets Check if the user has used the paddle
			CalculateTimingDit(&FSM_ParameterA,&FSM_ParameterB);
			//TODO insert Code for handling the Paddle
		
			//
			//Sainity Check
			if(FSM_ParameterA==0)
			{
				FSM_ManualState=DIT_LOW_HUMAN;
			}
			else
			{
			
				FSM_ManualState=DIT_HIGH_HUMAN;
				FSM_ParameterA--; //We just got one cycle high time
			}
		}
		break;
	
		case DIT_HIGH_HUMAN:
		{
			KEYOUTPUTHIGH;
			if(FSM_ParameterA>0)
			{
				FSM_ParameterA--;
			}
			else
			{
				
				FSM_ManualState=DIT_LOW_HUMAN;
			}
		}
		break;
		
		case DIT_LOW_HUMAN:
		{
			KEYOUTPUTLOW;
			//If the Machine reaches this state there will be no return
			if(FSM_ParameterB>0)
			{
				FSM_ParameterB--;
			}
			else
			{
				if((KeyA==STILLPRESSED)&&(KeyB!=STILLPRESSED))
				{
					if(CurrentKeymode!=BUGMODE)
					{
						FSM_ManualState=DIT_HUMAN;	
					}
					else
					{
						if(CurrentKeymode==ULTIMATIC)
						{
							ULTIMATIC_EXTENSION=0;
						}
						FSM_ManualState=DIT_HUMAN;
					}
						
				} 
				else if((KeyB==STILLPRESSED)&&(KeyA==STILLPRESSED))
				{
					//Okay we need to switch the Mode Specific behaviour.....
					if(CurrentKeymode!=BUGMODE)
					{
						if(CurrentKeymode==ULTIMATIC)
						{
							if(ULTIMATIC_EXTENSION==0)
							{
								ULTIMATIC_EXTENSION=1;
								FSM_ManualState=DAH_HUMAN;
							}
							else
							{
								FSM_ManualState=DIT_HUMAN;
							}
						}
						else
						{
							if(CurrentKeymode==LAMBICB)
							{
								LAMBICB_EXTENSION=1;
							}
							FSM_ManualState=DAH_HUMAN;
						}
					}
					else
					{
						//In Bugmode not supportet lets go IDLE
						if(LAMBICB_EXTENSION>0)
						{
							FSM_ManualState=DAH_HUMAN;
							LAMBICB_EXTENSION=0;
						}
						else
						{
							FSM_ManualState=IDLE;	
						}
						
					}
				}
				else
				{
					FSM_ManualState=IDLE;
				}
					
			}
			
		}
		break;
		
		case DAH_HUMAN:
		{
			FSM_ParameterA=0;
			FSM_ParameterB=0; //Keep in Mind for Low time this state adds one cycle to it....
			
			//Okay we got a DIT let's set the OUTPUT for that
			KEYOUTPUTHIGH;
			//Lets Check if the user has used the paddle
			CalculateTimingDah(&FSM_ParameterA,&FSM_ParameterB);
			//TODO insert Code for handling the Paddle
			
			//
			//Sainity Check
			if(FSM_ParameterA==0)
			{
				FSM_ManualState=DAH_LOW_HUMAN;
			}
			else
			{
				
				FSM_ManualState=DAH_HIGH_HUMAN;
				FSM_ParameterA--; //We just got one cycle high time
			}
		}
		break;
		
		case DAH_HIGH_HUMAN:
		{
			KEYOUTPUTHIGH;
			if(FSM_ParameterA>0)
			{
				FSM_ParameterA--;
			}
			else
			{
				FSM_ManualState=DAH_LOW_HUMAN;
			}
			
		}
		break;
		
		case DAH_LOW_HUMAN:
		{
			KEYOUTPUTLOW;
			//If the Machine reaches this state there will be no return
			//If the Machine reaches this state there will be no return
			if(FSM_ParameterB>0)
			{
				FSM_ParameterB--;
			}
			else
			{
				if((KeyB==STILLPRESSED)&&(KeyA!=STILLPRESSED))
				{
					if(CurrentKeymode==ULTIMATIC)
					{
						ULTIMATIC_EXTENSION=0;
					}
					FSM_ManualState=DAH_HUMAN;
				}
				else if((KeyB==STILLPRESSED)&&(KeyA==STILLPRESSED))
				{
						
					//Okay we need to switch the Mode Specific behaviour.....
					if(CurrentKeymode!=BUGMODE)
					{
						if(CurrentKeymode==ULTIMATIC)
						{
							if(ULTIMATIC_EXTENSION==0)
							{
								ULTIMATIC_EXTENSION=1;
								FSM_ManualState=DIT_HUMAN;
							}
							else
							{
								FSM_ManualState=DAH_HUMAN;
							}
						}
						else
						{
							if(CurrentKeymode==LAMBICB)
							{
								LAMBICB_EXTENSION=1;
							}
							FSM_ManualState=DIT_HUMAN;
						}
					}
					else
					{
						//In Bugmode not supportet lets go IDLE
						FSM_ManualState=IDLE;
					}
					
				}
				else
				{
					//In Bugmode not supportet lets go IDLE
					if(LAMBICB_EXTENSION>0)
					{
						FSM_ManualState=DIT_HUMAN;
						LAMBICB_EXTENSION=0;
					}
					else
					{
						FSM_ManualState=IDLE;
					}
				}
			}
		}
		break;
			
		default:
		{
			//Not known state something is wrong with that!
		}
		break;
	} //End of switch
	
}
else
{

		switch(FSM_State) //This FSM is for automaticmode only and dosn't handle manual input
		{
			case IDLE:
			{
				KEYOUTPUTLOW;
				//Lets see what is in our TODO List
				if((fifo_get_item_count(&FSM_Queue)==0))
				{
					if(EchoModeOn==1)
					{
						if(TXChar>=0)
						{
							fifo_put(EchoBuffer,(uint8_t)TXChar);
							TXChar=-1;
						}
					}
					//Our Buffer is empty lets refill it.....
					//Is there something that we can send?
					if(fifo_get_item_count(&AsciiQueue)!=0)
					{
						TXChar=(uint8_t)fifo_get_nowait(&AsciiQueue);
						if (TXChar == ' ')
						{
							CalculateTimingSpace(&FSM_ParameterA,&FSM_ParameterB);
							fifo_put(&FSM_Queue,SPACE_MACHINE);
						}
						else
						{
							morsechar_t mc=CharToMorse(TXChar);
							//Now we need to get mc into the queue
							for(uint8_t i=0;i<mc.lenght;i++)
							{
								if((mc.morsecode&(1<<i))>0)
								{
									fifo_put(&FSM_Queue,DAH_MACHINE);
								}
								else
								{
									fifo_put(&FSM_Queue,DIT_MACHINE);
								}
							}
							fifo_put(&FSM_Queue,LETTERPAUSE_MACHINE);
						}
					}
				
					
					
					//Get a Char from our Char Buffer if needed....	
				}
				if(fifo_get_item_count(&FSM_Queue)>0)
				{
					FSM_State=fifo_get_nowait(&FSM_Queue);
					/*
					//Okay Now we need to calculate the Timing.....
					*/
					switch(FSM_State)
					{
						case DIT_MACHINE:
						{
							FSM_ParameterA=0;
							FSM_ParameterB=0; //Keep in Mind for Low time this state adds one cycle to it....
						
							//Okay we got a DIT let's set the OUTPUT for that
							KEYOUTPUTHIGH;
							//Lets Check if the user has used the paddle
							CalculateTimingDit(&FSM_ParameterA,&FSM_ParameterB);
							//TODO insert Code for handling the Paddle
			
							//
							//Sainity Check
							if(FSM_ParameterA==0)
							{
								FSM_State=DIT_LOW_MACHINE;
							}
							else
							{
							
								FSM_State=DIT_HIGH_MACHINE;
								FSM_ParameterA--; //We just got one cycle high time
							}	
						
						}
						break;
						
					
						case DAH_MACHINE:
						{
							FSM_ParameterA=0;
							FSM_ParameterB=0; //Keep in Mind for Low time this state adds one cycle to it....
						
							//Okay we got a DIT let's set the OUTPUT for that
							KEYOUTPUTHIGH;
							//Lets Check if the user has used the paddle
							CalculateTimingDah(&FSM_ParameterA,&FSM_ParameterB);
							//TODO insert Code for handling the Paddle
						
							//
							//Sainity Check
							if(FSM_ParameterB==0)
							{
								FSM_State=DAH_LOW_MACHINE;
							}
							else
							{
							
								FSM_State=DAH_HIGH_MACHINE;
								FSM_ParameterB--; //We just got one cycle high time
							}
							//TODO insert Code for handling the Paddle
						
							//
						}
						break;
						//Unused State, just to keep warnings away
					
						case SPACE_MACHINE:
						{
							FSM_ParameterA=0;
							FSM_ParameterB=0; //Keep in Mind for Low time this state adds one cycle to it....
							CalculateTimingSpace(&FSM_ParameterA,&FSM_ParameterB);
							FSM_State=DO_SPACE_MACHINE;
							//TODO insert Code for handling the Paddle
									
							//
						}
						break;

						case LETTERPAUSE_MACHINE:
						{
							FSM_ParameterA=0;
							FSM_ParameterB=0; //Keep in Mind for Low time this state adds one cycle to it....
							CalculateTimingDit(&FSM_ParameterA,&FSM_ParameterB);
							FSM_State=PAUSE_LETTERS;
						}
						break;

						default:
						{
							FSM_ParameterA=0; //Parameter A not used for this State
							FSM_ParameterB=0; //Parameter B not used for this State
							//NOT Supported State
							FSM_State=IDLE;
						}
						break;
				
					}
				
				}
			
			}
			
			break;
		
			case DIT_HIGH_MACHINE:
			{
				//KEYOUTPUTHIGH;
				KEYOUTPUTHIGH;
				//Okay we need to decrese
				if(FSM_ParameterA>0)
				{
					FSM_ParameterA--;
				}			
				else //We are done here
				{
					FSM_State=DIT_LOW_MACHINE;
				}
				//TODO insert Code for handling the Paddle
			
				//
			}
			break;
		
			case DIT_LOW_MACHINE:
			{
				KEYOUTPUTLOW;
				if(FSM_ParameterB>0)
				{
					FSM_ParameterB--;
				}
				else //We are done here
				{
					FSM_State=IDLE;
				}
				//TODO insert Code for handling the Paddle
			
				//
			}
			break;
		
			case DAH_HIGH_MACHINE:
			{
				KEYOUTPUTHIGH;
				//Okay we need to decrese
				if(FSM_ParameterA>0)
				{
					FSM_ParameterA--;
				}			
				else //We are done here
				{
					FSM_State=DAH_LOW_MACHINE;
				}
				//TODO insert Code for handling the Paddle
			
				//
			}
			break;
		
			case DAH_LOW_MACHINE:
			{
				KEYOUTPUTLOW;
				if(FSM_ParameterB>0)
				{
					FSM_ParameterB--;
				}
				else //We are done here
				{
					FSM_State=IDLE;
				}
				//TODO insert Code for handling the Paddle
			
				//
		
			}
			break;	

			case PAUSE_LETTERS:
			{
				if(FSM_ParameterB>0)
				{
					FSM_ParameterB--;
				}
				else //We are done here
				{
					FSM_State=IDLE;
				}
				//TODO insert Code for handling the Paddle
				
				//
			}
			break;
			
			case DO_SPACE_MACHINE:
			{
				if(FSM_ParameterB>0)
				{
					FSM_ParameterB--;
				}
				else //We are done here
				{
					FSM_State=IDLE;
				}
				//TODO insert Code for handling the Paddle
							
				//
			}
			break;	
			
			default:
			{
				//Not used here or not valide
				KEYOUTPUTLOW;
			}
		


		}

		}
	TIMECOUNTERPORT |= (TIMECOUNTERPIN1);
}