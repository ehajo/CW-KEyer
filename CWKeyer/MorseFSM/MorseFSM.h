/*
 * MorseFSM.h
 *
 * Created: 09.06.2014 16:11:42
 *  Author: calm
 */ 


#ifndef MORSEFSM_H_
#define MORSEFSM_H_



#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>
#include <avr/power.h>
#include "../Enums/Enums.h"
#include "../RingBuffer/fifo.h"
#include "../Structs/Structs.h"

#define  KEYOUTPUTLOW (KEYPORT |= (KEY1 | KEY2))
#define  KEYOUTPUTHIGH (KEYPORT &= ~(KEY1 | KEY2))


uint8_t insert_keybyte(uint8_t keybyte);
void MorseFSM_Init(fifo_t* EchoBuffer);
void Morse_FSM_Start(void);
void Morse_FSM_SetWPM(uint8_t new_wpm);
void Morse_FSM_Update_Config(WinKeyConfig_t* Config);
uint8_t Morse_FSM_Get_Buffer_Free(void);
void Morse_FSM_Empty_Ascii_Buffer(void);
void Morse_FSM_Empty_All_Buffer(void);
void Morse_FSM_Force_Dit(void);
void Morse_FSM_Force_Dah(void);
uint8_t Morse_FSM_Transmitting(void);
void SetEchoMode(uint8_t Em);
#endif /* MORSEFSM_H_ */