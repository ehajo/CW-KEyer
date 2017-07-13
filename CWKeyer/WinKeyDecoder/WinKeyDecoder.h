/*
 * WinKeyDecoder.h
 *
 * Created: 09.06.2014 15:48:52
 *  Author: calm
 */ 


#ifndef WINKEYDECODER_H_
#define WINKEYDECODER_H_

#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>
#include <avr/power.h>
#include <avr/eeprom.h>
#include "../RingBuffer/fifo.h"



uint16_t speedconfig;
uint8_t winkey_mode;

enum states {
	ANFANG,
	ADMIN_CMD,
	SIDETONE,
	WPM_SPEED,
	WEIGHT,
	PTT_TIME,
	SPEEDPOTI,
	PAUSE,
	PINCFG,
	KEY_IMMEDIATE,
	HSCW,
	SET_WPM,
	SET_KEYMODE,
	LOAD_ALL,
	EXTENSION,
	KEY_COMP,
	PADDLE_SP,
	SW_PADDLE,
	POINTER_CMD1,
	POINTER_CMD2,
	DD_RATIO,
	ECHO_LOOPBACK,
	CALIBRATE,
	LOAD_EEPROM
};

enum post_processing {
	NOP,
	DUMP_EEPROM_CONFIG
};

typedef struct
{
	enum states state;
	uint8_t parameter;

} WinKey_FSM_State;

void InitWinkeyDecoder(void);
enum post_processing Decoder(uint8_t ReceivedByte, fifo_t* USARTtoUSB_Buffer);
uint8_t WinkeyEEPROMread(uint16_t Index);
void WinkeyEEPROMwrite(uint16_t Index, uint8_t data);
void load_configuration(void);

#endif /* WINKEYDECODER_H_ */