/*
...::: CW-Keyer Program :::...
eHaJo.de Blinkenlights-Bausatz
www.eHaJo.de

Authors:		/joggl -> info@ehajo.de
				/calm  -> info@ehajo.de

Basierend auf dem LUFA-Stack:

LUFA Library
Copyright (C) Dean Camera, 2012.

dean [at] fourwalledcubicle [dot] com
www.lufa-lib.org

*/

/** \file
 *
 *  Header file for cwkeyer.c
 */

#ifndef _USB_SERIAL_H_
#define _USB_SERIAL_H_

/* Includes: */
	#include <avr/io.h>
	#include <avr/wdt.h>
	#include <avr/interrupt.h>
	#include <avr/power.h>

	#include "Descriptors.h"

	#include <LUFA/Drivers/Board/LEDs.h>
	#include <LUFA/Drivers/Board/Joystick.h>
	#include <LUFA/Drivers/USB/USB.h>
	#include <LUFA/Platform/Platform.h>
	#include "./LUT/chartomorse.h"
	#include "./Enums/Enums.h"

	

// Globale Variablen
//Einmal aufräumen und volatine nach vorne holen
//Arrays und State variabelen Trennen










/* Macros: */



/* Funktionsprototypen */

//Umsortieren und kommentare einfügen
	void SetupHardware(void);
	void sende_uart_puffer(fifo_t* buffer);
	void get_configuration(void);
	uint8_t insert_keybyte(uint8_t);
	//void insert_paddlebyte(enum paddle_action pa);
	void cwsend(uint8_t);
	void dah(void);
	void dit(void);
	void keystop(void);
	void leerzeichen(void);
	void long_delay(uint16_t);


#endif

