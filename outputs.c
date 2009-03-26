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

#if (defined ESC_X3D || defined ESC_HOLGER || defined ESC_YGEI2C) && !defined DEBUG_SENSORS

#define ESC_I2C_DELAY Delay10TCY()
#define INP_ESC 1
#define OUT_ESC 0

void EscWaitClkHi(void)
{
	ESC_I2C_DELAY;
	ESC_CIO=1;	
	while( ESC_SCL == OUT_ESC ) ;	// Ensure PORTB Pullups enabled
	ESC_I2C_DELAY;
} // EscWaitClkHi

void EscI2CStart(void)
{
	ESC_DIO = INP_ESC;	
	EscWaitClkHi();
	ESC_SDA = 0;
	ESC_DIO = OUT_ESC;
	ESC_I2C_DELAY;
	ESC_SCL = OUT_ESC;
	ESC_CIO = 0;
} // EscI2CStart

void EscI2CStop(void)
{
	ESC_DIO = OUT_ESC;
	ESC_SDA = 0;
	EscWaitClkHi();

	ESC_DIO = INP_ESC;	// STOP condition
	ESC_I2C_DELAY;	
} // EscI2CStop


void SendEscI2CByte(uint8 d)
{
	int8 s;

	for( s = 8; s ; s-- )
	{
		if( (d & 0x80) !=0 )
			ESC_DIO = INP_ESC;	
		else
		{
			ESC_SDA = 0;			
			ESC_DIO = OUT_ESC;	
		}
	
		ESC_CIO = 1;
		while( ESC_SCL == OUT_ESC ) ;
		ESC_I2C_DELAY;
		ESC_SCL = OUT_ESC;
		ESC_CIO = 0;
		d <<= 1;
	}
	ESC_DIO = INP_ESC;	
	ESC_I2C_DELAY;
	ESC_CIO = 1;	
	while( ESC_SCL == OUT_ESC ) ;	

	ESC_I2C_DELAY;		
	//	nii = I2C_SDA;	

	ESC_SCL = OUT_ESC;
	ESC_CIO = 0;	
	ESC_I2C_DELAY;
	//	I2C_IO = 0;	
	//	I2C_SDA = 0;
	return;
} // SendEscI2CByte

#endif	// ESC_X3D || ESC_HOLGER || ESC_YGEI2C

uint8 SaturInt(int16 l)
{
	int16 r;

	#if defined ESC_PPM || defined ESC_HOLGER || defined ESC_YGEI2C
	r = Limit(l,  Max(_Minimum, MotorLowRun), _Maximum );
	#endif

	#ifdef ESC_X3D
	r = Limit(l - _Minimum, 200, 200);
	#endif
	return((uint8) r);
} // SaturInt


#ifdef TRADITIONAL_MOTOR_MIX

#ifndef GARYMIX

void MixAndLimit(void)
{
	int16 CurrGas;
    int16 Temp;

	CurrGas = IGas;	// to protect against IGas being changed in interrupt

	// Altitude stabilization factor
	CurrGas = CurrGas + (Vud + VBaroComp);
	Mf = Ml = Mr = CurrGas;
	#ifndef TRICOPTER
	Mb = CurrGas;
	#endif // TRICOPTER

	#ifndef TRICOPTER
	if( FlyCrossMode )
	{	// "Cross" Mode
		Ml +=  Pl - Rl;
		Mr += -Pl + Rl;
		Mf += -Pl - Rl;
		Mb +=  Pl + Rl;
	}
	else
	{	// "Plus" Mode
		#ifdef MOUNT_45
		Ml += -Rl - Pl;	
		Mr +=  Rl + Pl;	
		Mf +=  Rl - Pl;	
		Mb += -Rl + Pl;
		#else
		Ml += -Rl;
		Mr +=  Rl;
		Mf += -Pl;
		Mb +=  Pl;
		#endif
	}

	Mf += Yl;
	Mb += Yl;
	Ml -= Yl;
	Mr -= Yl;

	// if low-throttle limiting occurs, must limit other motor too
	// to prevent flips!

	if( CurrGas > MotorLowRun )
	{
		if( (Mf > Mb) && (Mb < MotorLowRun) )
		{
			Temp = Mb - MotorLowRun;
			Mf += Temp;
			Ml += Temp;
			Mr += Temp;
		}
		if( (Mb > Mf) && (Mf < MotorLowRun) )
		{
			Temp = Mf - MotorLowRun;
			Mb += Temp;
			Ml += Temp;
			Mr += Temp;
		}
		if( (Ml > Mr) && (Mr < MotorLowRun) )
		{
			Temp = Mr - MotorLowRun;
			Ml += Temp;
			Mf += Temp;
			Mb += Temp;
		}
		if( (Mr > Ml) && (Ml < MotorLowRun) )
		{	
			Temp = Ml - MotorLowRun;
			Mr += Temp;
			Mf += Temp;
			Mb += Temp;
		}
	}
	#else	// TRICOPTER
	Temp = SRS16(Rl - Pl, 1); 
	Motor[Front] += Pl ;			// front motor
	Motor[Left]  += Temp;			// rear left
	Motor[Right] -= Temp; 			// rear right
	Motor[Back]   = Yl + _Neutral;	// yaw servo

	if( CurrGas > MotorLowRun )
	{
		if( (Ml > Mr) && (Mr < MotorLowRun) )
		{
			Ml += Mr;
			Ml -= MotorLowRun;
		}
		if( (Mr > Ml) && (Ml < MotorLowRun) )
		{
			Mr += Ml;
			Mr -= MotorLowRun;
		}
	}
	#endif

	MFront = SaturInt(Mf);
	MLeft = SaturInt(Ml);
	MRight = SaturInt(Mr);
	MBack = SaturInt(Mb);
} // MixAndLimit

#else

void MixAndLimit(void)
{
    int16 MinMotor, CurrGas;

	// Altitude stabilization factor
	CurrGas = IGas + (Vud + VBaroComp); // vertical compensation not optional
	CurrGas = Limit(CurrGas, 0, (int16)(_Maximum * 90 + 50) / 100); // 10% headroom for control

	MinMotor = CurrGas;
	if ( MinMotor < Rl )
		Rl = MinMotor;
	if ( MinMotor < Pl )
		Pl = MinMotor;

	MinMotor = -MinMotor;
	
	if ( Rl < MinMotor )
		Rl = MinMotor;
	if ( Pl < MinMotor )
		Pl = MinMotor;

	Ml = CurrGas - Rl;
	Mr = CurrGas + Rl;
	Mf = CurrGas - Pl;
	Mb = CurrGas + Pl;

	MinMotor = Min(Mf, Mb);
	MinMotor = Min(MinMotor, Ml);
	MinMotor = Min(MinMotor, Mr);
	
	if ( MinMotor < Yl )
		Yl = MinMotor;

	MinMotor = - MinMotor;

	if ( Yl < MinMotor )
		Yl = MinMotor;

	Mf += Yl;
	Mb += Yl;
	Ml -= Yl;
	Mr -= Yl;

	Mf += MotorLowRun;
	Mb += MotorLowRun;
	Ml += MotorLowRun;
	Mr += MotorLowRun;

	MFront = SaturInt(Mf);
	MLeft = SaturInt(Ml);
	MRight = SaturInt(Mr);
	MBack = SaturInt(Mb);
}  // GaryMixAndLimit

#endif // GARYMIX

#else

void DoMix(int16 CurrGas)
{
	int16 Temp;

	Motor[Front] = Motor[Left] = Motor[Right] = CurrGas;
	#ifndef TRICOPTER
	Motor[Back] = CurrGas;
	#endif // !TRICOPTER

	#ifndef TRICOPTER
	if( FlyCrossMode )
	{	// "Cross" Mode
		Motor[Left] +=   Pl - Rl - Yl;
		Motor[Right] += -Pl + Rl - Yl;
		Motor[Front] += -Pl - Rl + Yl;
		Motor[Back] +=   Pl + Rl + Yl; //*
	}
	else
	{	// "Plus" Mode
		#ifdef MOUNT_45
		Motor[Left]  += -Rl - Pl - Yl; //*	
		Motor[Right] +=  Rl + Pl - Yl;
		Motor[Front] +=  Rl - Pl + Yl;
		Motor[Back]  += -Rl + Pl + Yl;	
		#else
		Motor[Left]  += -Rl - Yl;	
		Motor[Right] +=  Rl - Yl;
		Motor[Front] += -Pl + Yl;
		Motor[Back]  +=  Pl + Yl;
		#endif
	}

	#else	// TRICOPTER
	Temp = SRS16(Rl - Pl, 1); 
	Motor[Front] += Pl ;			// front motor
	Motor[Left]  += Temp;			// rear left
	Motor[Right] -= Temp; 			// rear right
	Motor[Back]   = Yl + _Neutral;	// yaw servo
	#endif

} // DoMix

uint8 	MotorDemandRescale;

void CheckDemand(int16 CurrGas)
{
	int8 s;
	int24 Scale, ScaleHigh, ScaleLow, MaxMotor, DemandSwing;

	MaxMotor = Max(Motor[Front], Motor[Left]);
	MaxMotor = Max(MaxMotor, Motor[Right]);
	#ifndef TRICOPTER
	MaxMotor = Max(MaxMotor, Motor[Back]);
	#endif // TRICOPTER

	DemandSwing = MaxMotor - CurrGas;

	if ( DemandSwing > 0 )
	{		
		ScaleHigh = (( _Maximum - (int24)CurrGas) * 256 )/ DemandSwing;	 
		ScaleLow = (( (int24)CurrGas - MotorLowRun) * 256 )/ DemandSwing;
		Scale = Min(ScaleHigh, ScaleLow);
		if ( Scale < 256 )
		{
			MotorDemandRescale = true;
			Rl = (Rl * Scale + 128)/256;  
			Pl = (Pl * Scale + 128)/256; 
			Yl = (Yl * Scale + 128)/256; 
		}
		else
			 MotorDemandRescale = false;	
	}
	else
		MotorDemandRescale = false;	

} // CheckDemand

void MixAndLimit(void)
{ 	// expensive ~400uSec @16MHz
    int16 Temp, CurrGas;

	// Altitude stabilization factor
	CurrGas = IGas + (Vud + VBaroComp); // vertical compensation not optional
	CurrGas = Limit(CurrGas, 0, (int16)(_Maximum * 90 + 50) / 100); // 10% headroom for control

	DoMix(CurrGas);

	CheckDemand(CurrGas);

	if ( MotorDemandRescale )
		DoMix(CurrGas);

} // MixAndLimit

#endif // TRADITIONAL_MOTOR_MIX

void MixAndLimitCam(void)
{
	int16 Cr, Cp;

	// use only roll/pitch angle estimates
	if( (IntegralCount == 0) && ((CamRollFactor != 0) || (CamPitchFactor != 0)) )
	{
		Cr = RollSum / (int16)CamRollFactor;
		Cp = PitchSum / (int16)CamPitchFactor;
	}
	else
		Cr = Cp = _Minimum;

	#ifdef DISABLE_ACC_ON_HIGH_YAWRATE
	Cr += _Neutral; // use IK7 for Yaw Rate threshold control
	#else
	if( _UseCh7Trigger )
		Cr += _Neutral;
	else
		Cr += IK7;
	#endif // DISABLE_ACC_ON_HIGH_YAWRATE
		
	Cp += IK6;		// only Pitch servo is controlled by channel 6

	MCamRoll = Limit(Cr, _Minimum, _Maximum);
	MCamPitch = Limit(Cp, _Minimum, _Maximum);

} // MixAndLimitCam

#pragma udata assembly_language=0x080 
uint8 SHADOWB, MF, MB, ML, MR, MT, ME; // motor/servo outputs
#pragma udata

void OutSignals(void)
{
	#ifdef DEBUG_SENSORS
	Trace[TIGas] = IGas;

	Trace[TIRoll] = IRoll;
	Trace[TIPitch] = IPitch;
	Trace[TIYaw] = IYaw;

	Trace[TMFront] = MFront;
	Trace[TMBack] = MBack;
	Trace[TMLeft] = MLeft;
	Trace[TMRight] = MRight;

	Trace[TMCamRoll] = MCamRoll;
	Trace[TMCamPitch] = MCamPitch;
	#else // !DEBUG_SENSORS

	WriteTimer0(0);
	INTCONbits.TMR0IF = false;

	#ifdef ESC_PPM
	_asm
	MOVLB	0						// select Bank0
	MOVLW	0x0f					// turn on motors
	MOVWF	SHADOWB,1
	_endasm	
	PORTB |= 0x0f;
	#endif

	MF = MFront;
	MB = MBack;
	ML = MLeft;
	MR = MRight;

	#ifdef DEBUG_MOTORS
	// if DEBUG_MOTORS is active, CamIntFactor is a bitmap:
	// bit 0 = no front motor
	// bit 1 = no rear motor
	// bit 2 = no left motor
	// bit 3 = no right motor
	// bit 4 = turns on the serial output

	if( IsSet(CamPitchFactor,0) )
		MF = _Minimum;
	if( IsSet(CamPitchFactor,1) )
		MB = _Minimum;
	if( IsSet(CamPitchFactor,2) )
		ML = _Minimum;
	if( IsSet(CamPitchFactor,3) )
		MR = _Minimum;
	#endif

	MT = MCamRoll;
	ME = MCamPitch;

	#ifdef ESC_PPM

	// simply wait for nearly 1 ms
	// irq service time is max 256 cycles = 64us = 16 TMR0 ticks
	while( ReadTimer0() < (uint16)(0x100-3-20) ) ; // 16

	// now stop CCP1 interrupt
	// capture can survive 1ms without service!

	// Strictly only if the masked interrupt region below is
	// less than the minimum valid Rx pulse/gap width which
	// is 1027uS less capture time overheads

	DisableInterrupts;	// BLOCK ALL INTERRUPTS for NO MORE than 1mS
	while( !INTCONbits.TMR0IF ) ;	// wait for first overflow
	INTCONbits.TMR0IF=0;		// quit TMR0 interrupt

	if( _OutToggle )	// driver cam servos only every 2nd pulse
	{
		_asm
		MOVLB	0					// select Bank0
		MOVLW	0x3f				// turn on motors
		MOVWF	SHADOWB,1
		_endasm	
		PORTB |= 0x3f;
	}
	_OutToggle ^= 1;

// This loop is exactly 16 cycles int16
// under no circumstances should the loop cycle time be changed
_asm
	MOVLB	0						// select Bank0
OS005:
	MOVF	SHADOWB,0,1				// Cannot read PORTB ???
	MOVWF	PORTB,0
	ANDLW	0x0f
	BZ		OS006
			
	DECFSZ	MF,1,1					// front motor
	GOTO	OS007
			
	BCF		SHADOWB,PulseFront,1	// stop Front pulse
OS007:
	DECFSZ	ML,1,1					// left motor
	GOTO	OS008
			
	BCF		SHADOWB,PulseLeft,1		// stop Left pulse
OS008:
	DECFSZ	MR,1,1					// right motor
	GOTO	OS009
			
	BCF		SHADOWB,PulseRight,1	// stop Right pulse
OS009:
	DECFSZ	MB,1,1					// rear motor
	GOTO	OS005
				
	BCF		SHADOWB,PulseBack,1		// stop Back pulse			

	GOTO	OS005
OS006:
_endasm
	// This will be the corresponding C code:
	//	while( ALL_OUTPUTS != 0 )
	//	{	// remain in loop as int16 as any output is still high
	//		if( TMR2 = MFront  ) PulseFront  = 0;
	//		if( TMR2 = MBack ) PulseBack = 0;
	//		if( TMR2 = MLeft  ) PulseLeft  = 0;
	//		if( TMR2 = MRight ) PulseRight = 0;
	//	}

	EnableInterrupts;	// Re-enable interrupt

	#endif	// ESC_PPM

	#if defined ESC_X3D || defined ESC_HOLGER || defined ESC_YGEI2C

	if( _OutToggle )	// driver cam servos only every 2nd pulse
	{
		_asm
		MOVLB	0					// select Bank0
		MOVLW	0x3f				// turn on motors
		MOVWF	SHADOWB,1
		_endasm	
		PORTB |= 0x3f;
	}
	_OutToggle ^= 1;

	// in X3D- and Holger-Mode, K2 (left motor) is SDA, K3 (right) is SCL
	#ifdef ESC_X3D
	EscI2CStart();
	SendEscI2CByte(0x10);	// one command, 4 data bytes
	SendEscI2CByte(MF); // for all motors
	SendEscI2CByte(MB);
	SendEscI2CByte(ML);
	SendEscI2CByte(MR);
	EscI2CStop();
	#endif	// ESC_X3D

	#ifdef ESC_HOLGER
	EscI2CStart();
	SendEscI2CByte(0x52);	// one cmd, one data byte per motor
	SendEscI2CByte(MF); // for all motors
	EscI2CStop();

	EscI2CStart();
	SendEscI2CByte(0x54);
	SendEscI2CByte(MB);
	EscI2CStop();

	EscI2CStart();
	SendEscI2CByte(0x58);
	SendEscI2CByte(ML);
	EscI2CStop();

	EscI2CStart();
	SendEscI2CByte(0x56);
	SendEscI2CByte(MR);
	EscI2CStop();
	#endif	// ESC_HOLGER

	#ifdef ESC_YGEI2C
	EscI2CStart();
	SendEscI2CByte(0x62);	// one cmd, one data byte per motor
	SendEscI2CByte(MF>>1); // for all motors
	EscI2CStop();

	EscI2CStart();
	SendEscI2CByte(0x64);
	SendEscI2CByte(MB>>1);
	EscI2CStop();

	EscI2CStart();
	SendEscI2CByte(0x68);
	SendEscI2CByte(ML>>1);
	EscI2CStop();

	EscI2CStart();
	SendEscI2CByte(0x66);
	SendEscI2CByte(MR>>1);
	EscI2CStop();
	#endif	// ESC_YGEI2C

	#endif	// ESC_X3D or ESC_HOLGER or ESC_YGEI2C

	while( ReadTimer0() < (uint16)(0x100-3-20) ) ; 	// wait for 2nd TMR0 near overflow

	INTCONbits.GIE = false;					// Int wieder sperren, wegen Jitter
	while( !INTCONbits.TMR0IF ) ;		// wait for 2nd overflow (2 ms)

	// This loop is exactly 16 cycles int16
	// under no circumstances should the loop cycle time be changed
_asm
	MOVLB	0
OS001:
	MOVF	SHADOWB,0,1				// Cannot read PORTB ???
	MOVWF	PORTB,0
	ANDLW	0x30		// output ports 4 and 5
	BZ		OS002		// stop if all 2 outputs are 0

	DECFSZ	MT,1,1
	GOTO	OS003

	BCF		SHADOWB,PulseCamRoll,1
OS003:
	DECFSZ	ME,1,1
	GOTO	OS004

	BCF		SHADOWB,PulseCamPitch,1
OS004:
_endasm
	Delay1TCY();
	Delay1TCY();
	Delay1TCY();
	Delay1TCY();
_asm
	GOTO	OS001
OS002:
_endasm

	EnableInterrupts;	// re-enable interrupt

#endif  // DEBUG_SENSORS
} // OutSignals


