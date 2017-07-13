/*
...::: CW-Keyer Program :::...
eHaJo.de Blinkenlights-Bausatz
www.eHaJo.de

Authors:		/joggl -> info@ehajo.de

Basierend auf dem LUFA-Stack:

LUFA Library
Copyright (C) Dean Camera, 2012.

dean [at] fourwalledcubicle [dot] com
www.lufa-lib.org

*/

#include "WinKeyDecoder/WinKeyDecoder.h"
#include "cwkeyer.h"
#include "RingBuffer/fifo.h"
#include "HW_Def/HW_Def.h"
#include "./MorseFSM/MorseFSM.h"

#include <util/delay.h>

static fifo_t USARTtoUSB_Buffer;
/** Underlying data buffer for \ref USARTtoUSB_Buffer, where the stored bytes are located. */
static uint8_t USARTtoUSB_Buffer_Data[128];

static fifo_t MorseEchotoUSB_Buffer;
/** Underlying data buffer for \ref USARTtoUSB_Buffer, where the stored bytes are located. */
static uint8_t MorseEchotoUSB_Buffer_Data[32];

static uint32_t Emulatedbaurate=0;
static uint32_t msCharSendtime=0;


//Okay grab a 16 Bit timer and buld a FSM running with 1kHz
//What to do if the USER push a paddel...
// debounce the input 
/*
But at each run the padle input into a 8Bit Variable 
A pressed Paddle will result in :
00000001 -> 00000011 -> 00000111 -> 00001111 -> ... ->01111111
														here we got a clean Keypress
so if input is 0b01111111 the paddle has pressed
for the release its revert order
so if input is 0b10000000 the paddle is released													

													

*/

void usb_puts(unsigned char x);


ISR(INT0_vect)
{	

} //Int is no longer requiered as the Statemachine will handle this.....

ISR(INT1_vect)
{	

} //Int is no longer requiered as the Statemachine will handle this.....

/** LUFA CDC Class driver interface configuration and state information. This structure is
 *  passed to all CDC Class driver functions, so that multiple instances of the same class
 *  within a device can be differentiated from one another.
 */
USB_ClassInfo_CDC_Device_t VirtualSerial_CDC_Interface =
	{
	.Config =
		{
			.ControlInterfaceNumber	= 0,
			.DataINEndpoint					=
				{
					.Address						= CDC_TX_EPADDR,
					.Size								= CDC_TXRX_EPSIZE,
					.Banks							= 1,
				},
			.DataOUTEndpoint				=
				{
					.Address						= CDC_RX_EPADDR,
					.Size								= CDC_TXRX_EPSIZE,
					.Banks							= 1,
				},
			.NotificationEndpoint		=
				{
					.Address						= CDC_NOTIFICATION_EPADDR,
					.Size								= CDC_NOTIFICATION_EPSIZE,
					.Banks							= 1,
				},
		},
	};


/** Main program entry point. This routine contains the overall program flow, including initial
 *  setup of all components and the main program loop.
 */

//TX Needs to be interrupt driven


int main(void)
{
	uint8_t freecounter=0;
	int16_t ReceivedByte;
	
	long_delay(200);
	
	SetupHardware();
	
	//SetupFSM();
	
	fifo_init(&USARTtoUSB_Buffer, USARTtoUSB_Buffer_Data, sizeof(USARTtoUSB_Buffer_Data));
	fifo_init(&MorseEchotoUSB_Buffer, MorseEchotoUSB_Buffer_Data, sizeof(MorseEchotoUSB_Buffer_Data));
	
	long_delay(200);
	//get_configuration();	// Werte EEPROM aus und schreibe config-bytes
	InitWinkeyDecoder();
	MorseFSM_Init(&MorseEchotoUSB_Buffer);
	sei(); // Setze globales Interrupt-Flag
	long_delay(10);
	Morse_FSM_Start();
	
	while(1==1)
	{

		/* Read bytes from the USB OUT endpoint into the USART transmit buffer */
			ReceivedByte = CDC_Device_ReceiveByte(&VirtualSerial_CDC_Interface);
			if(ReceivedByte>-1)	{
				switch(Decoder((uint8_t)(ReceivedByte),&USARTtoUSB_Buffer))
				{
					case DUMP_EEPROM_CONFIG:
					{
						//Okay we now need to send our EEPROM Back
						for(uint16_t i=0;i<256;i++)
						{
							usb_puts(WinkeyEEPROMread(i));	
						}
						
					}
					break;
					
					//If Nothing is to do or we know not what to do 
					case NOP:
					{
						//Nichts zu tun hier
					}
					break;
					default:
					{
						
					}
					break;
				}
				//Okay we may have to do some post_processing
				
			}
			//Every 16th run doing other tasks...
			if((freecounter&0x1F)==0x10){
				CDC_Device_USBTask(&VirtualSerial_CDC_Interface);
				USB_USBTask();
				
			}
			freecounter++; //Counts the current run....
			sende_uart_puffer(&USARTtoUSB_Buffer);	// UART-Puffer senden, Morsezeichen-Feedback
			sende_uart_puffer(&MorseEchotoUSB_Buffer);	// UART-Puffer senden, Morsezeichen-Feedback
	}
}

void sende_uart_puffer(fifo_t* buffer)
{
	volatile uint8_t BufferCount;
	volatile uint8_t BufferSize = (CDC_TXRX_EPSIZE - 1);
	//uint16_t BufferCount = RingBuffer_GetCount(&USARTtoUSB_Buffer);
	while ((BufferCount = fifo_get_item_count(buffer)))
	{
		Endpoint_SelectEndpoint(VirtualSerial_CDC_Interface.Config.DataINEndpoint.Address);
		if (Endpoint_IsINReady())
		{
			/* Never send more than one bank size less one byte to the host at a time, so that we don't block
				* while a Zero Length Packet (ZLP) to terminate the transfer is sent if the host isn't listening */
			//uint8_t BytesToSend = MIN(BufferCount, (CDC_TXRX_EPSIZE - 1)); //MIN seems broken dont't know why yet
			if(BufferCount>BufferSize)
			{
				BufferCount=BufferSize;
			}
			
			/* Read bytes from the USART receive buffer into the USB IN endpoint */
			while (BufferCount--)
			{
				uint8_t tx_byte=fifo_peek_nowait(buffer);
				/* Try to send the next byte of data to the host, abort if there is an error without dequeuing */
				if (CDC_Device_SendByte(&VirtualSerial_CDC_Interface,tx_byte ) != ENDPOINT_READYWAIT_NoError)
				{
					break;
				}
				/* Dequeue the already sent byte from the buffer now we have confirmed that no transmission error occurred */
				uint8_t txed_byte=fifo_get_nowait(buffer);
				long_delay(msCharSendtime); //We need to emulate the TX Rate of a real uart :-(
			}
		}
	}
}

void usb_puts(unsigned char x)
{
	volatile uint8_t BufferCount;
	volatile uint8_t BufferSize = (CDC_TXRX_EPSIZE - 1);
	//uint16_t BufferCount = RingBuffer_GetCount(&USARTtoUSB_Buffer);
	Endpoint_SelectEndpoint(VirtualSerial_CDC_Interface.Config.DataINEndpoint.Address);
	if (Endpoint_IsINReady())
	{
		/* Never send more than one bank size less one byte to the host at a time, so that we don't block
			* while a Zero Length Packet (ZLP) to terminate the transfer is sent if the host isn't listening */
		//uint8_t BytesToSend = MIN(BufferCount, (CDC_TXRX_EPSIZE - 1)); //MIN seems broken dont't know why yet
		if(BufferCount>BufferSize)
		{
			BufferCount=BufferSize;
		}
			
		/* Read bytes from the USART receive buffer into the USB IN endpoint */
			
			/* Try to send the next byte of data to the host, abort if there is an error without dequeuing */
			if (CDC_Device_SendByte(&VirtualSerial_CDC_Interface,x ) != ENDPOINT_READYWAIT_NoError)
			{
				
			}
			/* Dequeue the already sent byte from the buffer now we have confirmed that no transmission error occurred */
			long_delay(msCharSendtime); //We need to emulate the TX Rate of a rea uart :-(
		
	}
	
}

/** Configures the board hardware and chip peripherals for the demo's functionality. */
void SetupHardware(void)
{
	/* Disable watchdog if enabled by bootloader/fuses */
	MCUSR &= ~(1 << WDRF);
	wdt_disable();

	/* Disable clock division */
	clock_prescale_set(clock_div_1);
	
	//Setup der IO
	KEYOUTPUTHIGH;
	KEYDDR |= KEY1 | KEY2;
	
	PADDLEDDR &= ~(PADDLE1 | PADDLE2);
	PADDLEPORT |= PADDLE1 | PADDLE2;
	
	TIMECOUNTERDDR |= TIMECOUNTERPIN1;
	TIMECOUNTERPORT |= TIMECOUNTERPIN1;
	
	// Interrupts fuer Paddle/Key:
	// Lowlevel von INT0 und INT1 erzeugen INT
	//EICRA &= ~((1<<ISC00) | (1<<ISC01) | (1<<ISC10) | (1<<ISC11));
	//EIMSK |= (1<<INT0) | (1<<INT1);	// Enable interrupts
	//Change Clock to XTAL
	//
	MCUCR &= ~(1<<PUD);
	CLKSEL0 |= (1<<EXTE) | (1<<CLKS); // Diese Zeile schaltet die externe Taktquelle in und wählt sie als CPU-Takt
	CLKPR = (1<<CLKPCE);  // Prescaler-Change aktivieren
	CLKPR = 0;			  // Prescaler Teilefaktor 1

	//TCCR0A |= (1<<WGM01); // Timer im CTC-Mode
	//TCCR0B |= (1<<FOC0A) | (1<<CS01); // Compare output A aktivieren, 64er Teiler
	//OCR0A = 100; // Hier gibts nen Compare-Match Interrupt
	//TIMSK0 |= (1<<OCIE0A); // Capture Interrupt enable Timer 0 Capture A
	SREG |= (0b10000000); // Global Interrupt enable
	
	/* Hardware Initialization */
	USB_Init();
}

void long_delay(uint16_t ms) 
{
	for(; ms>0; ms--) 
		_delay_ms(1);
}

//DIT DAH Ratio
// DAH/DIT = 3*(nn/50)
//DAH=DIT*3*(nn/50)

//NN = WPM - Range from 5 - 99

//@ 100WPM ist ein DIT 12ms Lang



/** Event handler for the library USB Configuration Changed event. */
void EVENT_USB_Device_ConfigurationChanged(void)
{
	bool ConfigSuccess = true;
	ConfigSuccess &= CDC_Device_ConfigureEndpoints(&VirtualSerial_CDC_Interface);
}

/** Event handler for the library USB Control Request reception event. */
void EVENT_USB_Device_ControlRequest(void)
{
	CDC_Device_ProcessControlRequest(&VirtualSerial_CDC_Interface);
}

/** Event handler for the CDC Class driver Line Encoding Changed event.
 *
 *  \param[in] CDCInterfaceInfo  Pointer to the CDC class interface configuration structure being referenced
 */
void EVENT_CDC_Device_LineEncodingChanged(USB_ClassInfo_CDC_Device_t* const CDCInterfaceInfo)
{
	uint8_t ConfigMask = 0;

	switch (CDCInterfaceInfo->State.LineEncoding.ParityType)
	{
		case CDC_PARITY_Odd:
			ConfigMask = ((1 << UPM11) | (1 << UPM10));
			break;
		case CDC_PARITY_Even:
			ConfigMask = (1 << UPM11);
			break;
	}

	if (CDCInterfaceInfo->State.LineEncoding.CharFormat == CDC_LINEENCODING_TwoStopBits)
	  ConfigMask |= (1 << USBS1);

	switch (CDCInterfaceInfo->State.LineEncoding.DataBits)
	{
		case 6:
			ConfigMask |= (1 << UCSZ10);
			break;
		case 7:
			ConfigMask |= (1 << UCSZ11);
			break;
		case 8:
			ConfigMask |= ((1 << UCSZ11) | (1 << UCSZ10));
			break;
	}
	
	Emulatedbaurate=CDCInterfaceInfo->State.LineEncoding.BaudRateBPS;
	msCharSendtime=(uint32_t)(10000)/Emulatedbaurate;
	
}