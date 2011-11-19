// ===============================================================================================
// =                                UAVX Quadrocopter Controller                                 =
// =                           Copyright (c) 2008 by Prof. Greg Egan                             =
// =                 Original V3.15 Copyright (c) 2007 Ing. Wolfgang Mahringer                   =
// =                     http://code.google.com/p/uavp-mods/ http://uavp.ch                      =
// ===============================================================================================

//    This is part of UAVX.

//    UAVX is free software: you can redistribute it and/or modify it under the terms of the GNU 
//    General Public License as published by the Free Software Foundation, either version 3 of the 
//    License, or (at your option) any later version.

//    UAVX is distributed in the hope that it will be useful,but WITHOUT ANY WARRANTY; without
//    even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  
//    See the GNU General Public License for more details.

//    You should have received a copy of the GNU General Public License along with this program.  
//    If not, see http://www.gnu.org/licenses/

#include "uavx.h"

void SendLEDs(void);
void LEDsOn(uint8);
void LEDsOff(uint8);
void LEDChaser(void);

#pragma udata access LEDvars
near uint8 LEDShadow, LEDShadowp;
#pragma udata

uint8 SavedLEDs, LEDPattern = 0;
boolean PrevHolding = false;
const uint8 LEDChase[7] = {
		AUX1M,	
		AUX2M,
		AUX3M,
		YellowM,
		RedM,
		GreenM,
		BlueM
	};

void SaveLEDs(void)
{ // one level only
	SavedLEDs = LEDShadow;
} // SaveLEDs

void RestoreLEDs(void)
{
	LEDShadow = SavedLEDs;
	SendLEDs();
} // RestoreLEDs

#ifdef UAVX_HW

void SendLEDs(void)
{	
	PORTBbits.RB6 = (LEDShadow &  YellowM ) != (uint8)0;
	PORTBbits.RB7 = (LEDShadow &  BlueM ) != (uint8)0;
	PORTCbits.RC0 = (LEDShadow &  RedM ) != (uint8)0;
	PORTCbits.RC1 = (LEDShadow &  GreenM ) != (uint8)0;
	PORTCbits.RC5 = (LEDShadow &  BeeperM ) == (uint8)0; // active low
} // SendLEDs


#else // UAVX_HW

void SendLEDs(void) // 39.3 uS @ 40MHz 
{
	static int8	i, s;

	if ( LEDShadow != LEDShadowp )
	{
		i = LEDShadow;
		SPI_CS = DSEL_LISL;	
		SPI_IO = WR_SPI;	// SDA is output
		SPI_SCL = 0;		// because shift is on positive edge
		
		for(s = 8; s ; s--)
		{
			if( i & 0x80 )
				SPI_SDA = 1;
			else
				SPI_SDA = 0;
			i<<=1;
			Delay10TCY();
			SPI_SCL = 1;
			Delay10TCY();
			SPI_SCL = 0;
		}
	
		PORTCbits.RC1 = 1;
		PORTCbits.RC1 = 0;	// latch into drivers
		SPI_SCL = 1;		// rest state for LISL
		SPI_IO = RD_SPI;
		
		LEDShadowp = LEDShadow;
	}

} // SendLEDs

#endif // UAVX_HW

void LEDsOn(uint8 l)
{
	LEDShadow |= l;
	SendLEDs();
} // LEDsOn

void LEDsOff(uint8 l)
{
	LEDShadow &= ~l;
	SendLEDs();
} // LEDsOff

void LEDChaser(void)
{
//	#define LED_NO 		(uint8)2	// just AUX LEDs
	#define LED_NO		(uint8)6	// all LEDs

	if ( mSClock() > mS[LEDChaserUpdate] )
	{
		mS[LEDChaserUpdate] = mSClock() + 60;
		if ( F.HoldingAlt ) 
		{
			LEDShadow ^= LEDChase[LEDPattern];
			if ( LEDPattern < LED_NO ) LEDPattern++; else LEDPattern = 0;
			LEDShadow |= LEDChase[LEDPattern];
			SendLEDs();
		}
		else
		{
			RestoreLEDs();
			if ( F.AccelerationsValid && F.AccelerometersEnabled )
				LEDYellow_ON;
			else
				LEDYellow_OFF;
		}			
	}
} // LEDChaser

