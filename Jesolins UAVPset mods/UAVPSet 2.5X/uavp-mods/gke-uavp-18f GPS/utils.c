// =======================================================================
// =                   U.A.V.P Brushless UFO Controller                  =
// =                         Professional Version                        =
// =               Copyright (c) 2008-9 by Prof. Greg Egan               =
// =     Original V3.15 Copyright (c) 2007 Ing. Wolfgang Mahringer       =
// =                          http://www.uavp.org                        =
// =======================================================================

//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.

//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.

//  You should have received a copy of the GNU General Public License along
//  with this program; if not, write to the Free Software Foundation, Inc.,
//  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

// Utilities and subroutines

#include "c-ufo.h"
#include "bits.h"

void Delay1mS(int16 d)
{ 	// Timer0 interrupt at 1mS must be running
	int16 i;
	uint8 T0IntEn;

	T0IntEn = INTCONbits.TMR0IE;	// not protected?
	INTCONbits.TMR0IE = false;

	// if d is 1 then delay can be less than 1mS due to 	
	for (i=d; i; i--)
	{						// compromises ClockMilliSec;
		while ( !INTCONbits.TMR0IF ) {};
		INTCONbits.TMR0IF = 0;
	}

	INTCONbits.TMR0IE = T0IntEn;

} // Delay1mS

// wait blocking for "dur" * 0.1 seconds
// Motor and servo pulses are still output every 10ms
void Delay100mSWithOutput(int16 dur)
{ // Timer0 Interrupts must be off 
	int16 i, j;
	uint8 T0IntEn;

	T0IntEn = INTCONbits.TMR0IE;	// not protected?
	INTCONbits.TMR0IE = false;

	for(i = 0; i < dur*10; i++)
		{
			for (j = 8; j ; j--)
			{
				while ( !INTCONbits.TMR0IF ) {};
				INTCONbits.TMR0IF = 0;
			}
			OutSignals(); // 1-2 ms Duration
			#ifdef RX_INTERRUPTS
			if ( RxTail != RxHead )
			{
				INTCONbits.TMR0IE = T0IntEn;
				return;
			}
			#else
			if( PIR1bits.RCIF )
			{
				INTCONbits.TMR0IE = T0IntEn;
				return;
			}
			#endif // RX_INTERRUPTS
		}
	INTCONbits.TMR0IE = T0IntEn;
} // Delay100mSWithOutput

int16 SRS16(int16 x, uint8 s)
{
	return((x<0) ? -((-x)>>s) : (x>>s));
} // SRS16

int24 xxxSRS24(int24 x, uint8 s)
{
	return((x<0) ? -((-x)>>s) : (x>>s));
} // SRS24

void InitPorts(void)
{
	// general ports setup
	TRISA = 0b00111111;								// all inputs
	ADCON1 = 0b00000010;							// uses 5V as Vref

	PORTB = 0b11000000;								// all outputs to low, except RB6 & 7 (I2C)!
	TRISB = 0b01000000;								// all servo and LED outputs
	PORTC = 0b01100000;								// all outputs to low, except TxD and CS
	TRISC = 0b10000100;								// RC7, RC2 are inputs

	SSPSTATbits.CKE = true;							// low logic threshold for LISL
	INTCON2bits.NOT_RBPU = false;	// WEAK PULLUPS MUST BE ENABLED OTHERWISE I2C VERSIONS 
									// WITHOUT ESCS INSTALLED WILL PREVENT ANY FURTHER BOOTLOADS
} // InitPorts

// resets all important variables - Do NOT call that while in flight!
void InitArrays(void)
{
	int8 i;

	for (i = 0; i < NoOfMotors; i++)
		Motor[i] = _Minimum;
	MCamPitch = MCamRoll = _Neutral;

	_Flying = false;
	REp = PEp = YEp = 0;
	
	Rp = Pp = Vud = VBaroComp = 0;
	
	UDSum = 0;
	LRIntKorr = FBIntKorr = 0;
	YawSum = RollSum = PitchSum = 0;

	AverageYawRate = 0;

	BaroRestarts = 0;
	RCGlitchCount = 0;
} // InitArrays

#pragma idata sintable
const uint8 SineTable[17]={ 
	0, 50, 98, 142, 180, 212, 236, 250, 255,
	250, 236, 212, 180, 142, 98, 50, 0
   };
#pragma idata

int16 Table16(int16 Val, uint8 *T)
{
	uint8 Index,Offset;
	int16 Temp, Low, High, Result;

	Index = (uint8) (Val >> 4);
	Offset = (uint8) (Val & 0x0f);
	Low = T[Index];
	High = T[++Index];
	Temp = (High-Low) * Offset;
	Result = Low + SRS16(Temp, 4);

	return(Result);
} // Table16

int16 int16sin(int16 A)
{	// A is in milliradian 0 to 2000Pi, result is -255 to 255
	int16 	v;
	uint8	Negate;

	while ( A < 0 ) A += TWOMILLIPI;
	while ( A >= TWOMILLIPI ) A -= TWOMILLIPI;

	Negate = A >= MILLIPI;
	if ( Negate )
		A -= MILLIPI;

	v = Table16((((int24)A * 256 + HALFMILLIPI)/MILLIPI)-1, &SineTable);

	if ( Negate )
		v= -v;

	return(v);
} // int16sin

int16 int16cos(int16 A)
{	// A is in milliradian 0 to 2000Pi, result is -255 to 255
	return(int16sin(A + HALFMILLIPI));
} // int16cos

#pragma idata arctan
const int16 ArctanTable[17]={
    0,  62, 124, 185, 245, 303, 359, 412,
  464, 512, 559, 602, 644, 682, 719, 753,785
   };
#pragma idata

int16 int16atan2(int16 y, int16 x)
{	// Result is in milliradian
	// Caution - this routine is intended to be acceptably accurate for 
	// angles less Pi/4 within a quadrant. Larger angles are directly interpolated
	// to Pi/2. 
 
	int32 Absx, Absy, TL;
	int16 A;

	Absy = Abs(y);
	Absx = Abs(x);

	if ( x == 0 )
		if ( y < 0 )
			A = -HALFMILLIPI;
		else
			A = HALFMILLIPI;
	else
		if (y == 0)
			if ( x < 0 )
				A=MILLIPI;
			else
				A = 0;
		else
		{
			TL = (Absy * 256)/Absx;
			if ( TL < 256 )
				A = Table16(TL, &ArctanTable);
			else
			{  // interpolate above ~Pi/4
				TL >>= 8;
				A = ArctanTable[16] + ((HALFMILLIPI - ArctanTable[16])*TL)/(32767-256);
			}

			if ( x < 0 )
				if ( y > 0 ) // 2nd Quadrant 
					A = MILLIPI - A;
				else // 3rd Quadrant 
					A = MILLIPI + A;
			else
				if ( y < 0 ) // 4th Quadrant 
					A = TWOMILLIPI-A;
	}
	return(A);
} // int16atan2


void CheckAlarms(void)
{
	int16 NewBatteryVolts;

	NewBatteryVolts = ADC(ADCBattVoltsChan, ADCVREF5V) >> 3; 
	BatteryVolts = SoftFilter(BatteryVolts, NewBatteryVolts);
	_LowBatt =  (BatteryVolts < (int16) LowVoltThres) & 1;

	if( _LowBatt ) // repeating beep
	{
		if( BlinkCount < BLINK_LIMIT/2 )
		{
			Beeper_ON;
			LedRed_ON;
		}
		else
		{
			Beeper_OFF;
			LedRed_OFF;
		}	
	}
	else
	if ( _LostModel ) // 2 beeps with interval
		if( (BlinkCount < (BLINK_LIMIT/2)) && ( BlinkCycle < (BLINK_CYCLES/4 )) )
		{
			Beeper_ON;
			LedRed_ON;
		}
		else
		{
			Beeper_OFF;
			LedRed_OFF;
		}	
	else
	if ( _BaroRestart ) // 1 beep with interval
		if( (BlinkCount < (BLINK_LIMIT/2)) && ( BlinkCycle == 0 ) )
		{
			Beeper_ON;
			LedRed_ON;
		}
		else
		{
			Beeper_OFF;
			LedRed_OFF;
		}	
	else
	{
		Beeper_OFF;				
		LedRed_OFF;
	}

} // CheckAlarms

void LedGame(void)
{
	if( --LedCount == 0 )
	{
		LedCount = ((255-IGas)>>3) +5;	// new setup
		if( _Hovering )
		{
			AUX_LEDS_ON;	// baro locked, all aux-leds on
		}
		else
		if( LedShadow & LedAUX1 )
		{
			AUX_LEDS_OFF;
			LedAUX2_ON;
		}
		else
		if( LedShadow & LedAUX2 )
		{
			AUX_LEDS_OFF;
			LedAUX3_ON;
		}
		else
		{
			AUX_LEDS_OFF;
			LedAUX1_ON;
		}
	}
} // LedGame

void DoPIDDisplays()
{
	if ( IntegralTest )
	{
		ALL_LEDS_OFF;
		if( (int8)(RollSum>>8) > 0 )
			LedRed_ON;
		else
			if( (int8)(RollSum>>8) < -1 )
				LedGreen_ON;

		if( (int8)(PitchSum>>8) >  0 )
			LedYellow_ON;
		else
			if( (int8)(PitchSum>>8) < -1 )
				LedBlue_ON;
	}
	else
		if( CompassTest )
		{
			ALL_LEDS_OFF;
			if( CurDeviation > 0 )
				LedGreen_ON;
			else
				if( CurDeviation < 0 )
					LedRed_ON;
			if( AbsDirection > COMPASS_MAX )
				LedYellow_ON;
		}
} // DoPIDDisplays

void UpdateBlinkCount(void)
{
	if( BlinkCount == 0 )
	{
		BlinkCount = BLINK_LIMIT;
		if ( BlinkCycle == 0)
			BlinkCycle = BLINK_CYCLES;
		BlinkCycle--;
	}
	BlinkCount--;
} // UpdateBlinkCount

void SendLeds(void)
{
	int8	i, s;
	i = LedShadow;
	SPI_CS = DSEL_LISL;	
	SPI_IO = WR_SPI;	// SDA is output
	SPI_SCL = 0;		// because shift is on positive edge
	
	for(s=8; s!=0; s--)
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
}

void SwitchLedsOn(uint8 l)
{
	LedShadow |= l;
	SendLeds();
} // SwitchLedsOn

void SwitchLedsOff(uint8 l)
{
	LedShadow &= ~l;
	SendLeds();
} // SwitchLedsOff

#ifdef DEBUG_SENSORS
void DumpTrace(void)
{
	int8 t;

	for (t=0; t <= TopTrace; t++)
	{
		TxValH16(Trace[t]);
		TxChar(';');
	}
	TxNextLine();
} // DumpTrace
#endif // DEBUG_SENSORS
