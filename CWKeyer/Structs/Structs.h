/*
 * Structs.h
 *
 * Created: 24.06.2014 19:48:19
 *  Author: calm
 */ 


#ifndef STRUCTS_H_
#define STRUCTS_H_

typedef struct
{
	uint8_t ConfHeader;
	uint8_t Moderegister;
	uint8_t WPM;
	uint8_t Sidetone;
	uint8_t Weight;
	uint8_t LeadInTime;
	uint8_t TailTime;
	uint8_t MinWPM;
	uint8_t WPMRange;
	uint8_t X2Mode;
	uint8_t KeyCompensation;
	uint8_t FarnsworthWPM;
	uint8_t PaddleSetPoint;
	uint8_t DitDahRatio;
	uint8_t PinConfiguration;
	uint8_t X1Mode;
} WinKeyConfig_t;

#endif /* STRUCTS_H_ */