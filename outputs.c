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

uint8 PWMLimit(int16);
uint8 I2CESCLimit(int16);
void DoMulticopterMix(int16);
void CheckDemand(int16);
void MixAndLimitMotors(void);
void MixAndLimitCam(void);
void OutSignals(void);
void InitI2CESCs(void);
void StopMotors(void);
void InitMotors(void);

boolean OutToggle;
int16 PWM[6];
int16 PWMSense[6];
int16 ESCI2CFail[4];
int16 CurrThrottle;
int8 ServoInterval;

#pragma udata access outputvars
near uint8 SHADOWB, PWM0, PWM1, PWM2, PWM3, PWM4, PWM5;
near int8 ServoToggle;
#pragma udata

int16 ESCMin, ESCMax;

uint8 PWMLimit(int16 T)
{

	return((uint8)Limit(T,1,OUT_MAXIMUM));

} // PWMLimit

uint8 I2CESCLimit(int16 T)
{

	return((uint8)Limit(T,0,ESCMax));

} // I2CESCLimit

#ifdef MULTICOPTER

void DoMulticopterMix(int16 CurrThrottle)
{
	static int16 Temp, B;

	#ifdef Y6COPTER
		PWM[FrontTC] = PWM[LeftTC] = PWM[RightTC] = CurrThrottle;
	#else
		PWM[FrontC] = PWM[LeftC] = PWM[RightC] = PWM[BackC] = CurrThrottle;
	#endif

	#ifdef TRICOPTER // usually flown K1 motor to the rear - use orientation of 24
		Temp = SRS16(Pl, 1); 			
		PWM[FrontC] -= Pl;			// front motor
		PWM[LeftC] += (Temp - Rl);	// right rear
		PWM[RightC] += (Temp + Rl); // left rear
	
		PWM[BackC] = PWMSense[RudderC] * Yl + OUT_NEUTRAL;	// yaw servo
		if ( P[Balance] != 0 )
		{
			B = 128 + P[Balance];
			PWM[FrontC] =  SRS32((int32)PWM[FrontC] * B, 7);
		}
	#else
	    #ifdef VTCOPTER 	// usually flown VTail (K1+K4) to the rear - use orientation of 24
			Temp = SRS16(Pl, 1); 	

			PWM[LeftC] += (Temp - Rl);	// right rear
			PWM[RightC] += (Temp + Rl); // left rear
	
			PWM[FrontLeftC] -= Pl + PWMSense[RudderC] * Yl; 
			PWM[FrontRightC] -= Pl - PWMSense[RudderC] * Yl;
			if ( P[Balance] != 0 )
			{
				B = 128 + P[Balance];
				PWM[FrontLeftC] = SRS32((int32)PWM[FrontLeftC] * B, 7);
				PWM[FrontRightC] = SRS32((int32)PWM[FrontRightC] * B, 7);
			}
		#else 
			#ifdef Y6COPTER

				Temp = SRS16(Pl, 1); 			
				PWM[FrontTC] -= Pl;			 // front motor
				PWM[LeftTC] += (Temp - Rl);	 // right rear
				PWM[RightTC] += (Temp + Rl); // left rear
			
				PWM[FrontBC] = PWM[FrontTC];
				PWM[LeftBC]  = PWM[LeftTC];
				PWM[RightBC] = PWM[RightTC];

				if ( P[Balance] != 0 )
				{
					B = 128 + P[Balance];
					PWM[FrontTC] =  SRS32((int32)PWM[FrontTC] * B, 7);
					PWM[FrontBC] = PWM[FrontTC];
				}

				Temp = Yl / 2;
				PWM[FrontTC] += Temp;
				PWM[LeftTC]  += Temp;
				PWM[RightTC] += Temp;

				PWM[FrontBC] -= Temp;
				PWM[LeftBC]  -= Temp; 
				PWM[RightBC] -= Temp; 

			#else
				PWM[LeftC]  += -Rl - Yl;	
				PWM[RightC] +=  Rl - Yl;
				PWM[FrontC] += -Pl + Yl;
				PWM[BackC]  +=  Pl + Yl;
			#endif
		#endif
	#endif

} // DoMulticopterMix

boolean 	MotorDemandRescale;

void CheckDemand(int16 CurrThrottle)
{
	static int24 Scale, ScaleHigh, ScaleLow, MaxMotor, DemandSwing;
	static i24u Temp;

	#ifdef Y6COPTER
		MaxMotor = Max(PWM[FrontTC], PWM[LeftTC]);
		MaxMotor = Max(MaxMotor, PWM[RightTC]);
		MaxMotor = Max(MaxMotor, PWM[FrontBC]);
		MaxMotor = Max(MaxMotor, PWM[LeftBC]);
		MaxMotor = Max(MaxMotor, PWM[RightBC]);
	#else
		MaxMotor = Max(PWM[FrontC], PWM[LeftC]);
		MaxMotor = Max(MaxMotor, PWM[RightC]);
		#ifndef TRICOPTER
			MaxMotor = Max(MaxMotor, PWM[BackC]);
		#endif // TRICOPTER
	#endif // Y6COPTER

	DemandSwing = MaxMotor - CurrThrottle;

	if ( DemandSwing > 0 )
	{		
		ScaleHigh = (( OUT_MAXIMUM - (int24)CurrThrottle) * 256 )/ DemandSwing;	 
		ScaleLow = (( (int24)CurrThrottle - IdleThrottle) * 256 )/ DemandSwing;
		Scale = Min(ScaleHigh, ScaleLow); // just in case!
		if ( Scale < 0 ) Scale = 1;
		if ( Scale < 256 )
		{
			MotorDemandRescale = true;
			Temp.i24 = Rl * Scale; 
			Rl = Temp.i2_1;
			Temp.i24 = Pl * Scale;
			Pl = Temp.i2_1;
			#ifndef TRICOPTER 
				Temp.i24 = Yl * Scale;
				Yl = Temp.i2_1;
			#endif // TRICOPTER 
		}
		else
			MotorDemandRescale = false;	
	}
	else
		MotorDemandRescale = false;	

} // CheckDemand

#endif // MULTICOPTER

void MixAndLimitMotors(void)
{ 	// expensive ~160uSec @ 40MHz
    static int16 Temp, TempElevon, TempElevator;
	static uint8 m;

	if ( DesiredThrottle < IdleThrottle )
		CurrThrottle = 0;
	else
		CurrThrottle = DesiredThrottle;

	#ifdef MULTICOPTER
		if ( State == InFlight )
			 CurrThrottle += AltComp; 
			
		Temp = (int16)(OUT_MAXIMUM * 90 + 50) / 100; // 10% headroom for control
		CurrThrottle = Limit(CurrThrottle, 0, Temp ); 
			
		if ( CurrThrottle > IdleThrottle )
		{
			DoMulticopterMix(CurrThrottle);
			
			CheckDemand(CurrThrottle);
			
			if ( MotorDemandRescale )
				DoMulticopterMix(CurrThrottle);
		}
		else
		{
			#ifdef Y6COPTER
				for ( m = 0; m < (uint8)6; m++ )
					PWM[m] = CurrThrottle;
			#else
				PWM[FrontC] = PWM[LeftC] = PWM[RightC] = CurrThrottle;
				#ifdef TRICOPTER
					PWM[BackC] = PWMSense[RudderC] * Yl + OUT_NEUTRAL;	// yaw servo
				#else
					PWM[BackC] = CurrThrottle;
				#endif // !TRICOPTER
			#endif // Y6COPTER
		}
	#else
		CurrThrottle += AltComp; // simple - faster to climb with no elevator yet
		
		PWM[ThrottleC] = CurrThrottle;
		PWM[RudderC] = PWMSense[RudderC] * Yl + OUT_NEUTRAL;
		
		#if ( defined AILERON | defined HELICOPTER )
			PWM[AileronC] = PWMSense[AileronC] * Rl + OUT_NEUTRAL;
			PWM[ElevatorC] = PWMSense[ElevatorC] * Pl + OUT_NEUTRAL;
		#else // ELEVON
			TempElevator = PWMSense[2] * Pl + OUT_NEUTRAL;
			PWM[RightElevonC] = PWMSense[RightElevonC] * (TempElevator + Rl);
			PWM[LeftElevonC] = PWMSense[LeftElevonC] * (TempElevator -  Rl);		
		#endif
	#endif

} // MixAndLimitMotors

void MixAndLimitCam(void)
{
	#ifndef Y6COPTER
	static i24u Temp;
	static int32 NewCamRoll, NewCamPitch;

	// use only roll/pitch angle estimates
	Temp.i24 = (int24)CameraRollAngle * P[CamRollKp];
	NewCamRoll = Temp.i2_1 + (int16)P[CamRollTrim];
	NewCamRoll = PWMSense[CamRollC] * NewCamRoll + OUT_NEUTRAL;
	PWM[CamRollC] = SlewLimit( PWM[CamRollC], NewCamRoll, 2);

	Temp.i24 = (int24)CameraPitchAngle * P[CamPitchKp];
	NewCamPitch = Temp.i2_1 + SRS16(DesiredCamPitchTrim * 3, 1);
	NewCamPitch = PWMSense[CamPitchC] * NewCamPitch + OUT_NEUTRAL; 
	PWM[CamPitchC] = SlewLimit( PWM[CamPitchC], NewCamPitch, 2);

	#endif // !Y6COPTER

} // MixAndLimitCam

#if ( defined Y6COPTER )
	#include "outputs_y6.h"
#else
	#if ( defined TRICOPTER | defined MULTICOPTER | defined VTCOPTER )
		#include "outputs_copter.h"
	#else
		#include "outputs_conventional.h"
	#endif // Y6COPTER
#endif // TRICOPTER | MULTICOPTER

void InitI2CESCs(void)
{
	#ifndef SUPPRESS_I2C_ESC_INIT

	static int8 m;
	static uint8 r;
	
	#ifdef MULTICOPTER

	#ifndef Y6COPTER

	switch ( P[ESCType] ) {
 	case ESCHolger:
		for ( m = 0 ; m < NoOfI2CESCOutputs ; m++ )
		{
			ESCI2CStart();
			r = WriteESCI2CByte(0x52 + ( m*2 ));	// one cmd, one data byte per motor
			r += WriteESCI2CByte(0);
			ESCI2CFail[m] += r;  
			ESCI2CStop();
		}
		break;
	case ESCYGEI2C:
		for ( m = 0 ; m < NoOfPWMOutputs ; m++ )
		{
			ESCI2CStart();
			r = WriteESCI2CByte(0x62 + ( m*2 ));	// one cmd, one data byte per motor
			r += WriteESCI2CByte(0);
			ESCI2CFail[m] += r; 
			ESCI2CStop();
		}
		break;
	case ESCLRCI2C:
		// no action required
		break;
	case ESCX3D:
		ESCI2CStart();
		r = WriteESCI2CByte(0x10);			// one command, 4 data bytes
		r += WriteESCI2CByte(0); 
		r += WriteESCI2CByte(0);
		r += WriteESCI2CByte(0);
		r += WriteESCI2CByte(0);
		ESCI2CFail[0] += r;
		ESCI2CStop();
		break;
	default:
		break;
	} // switch		

	#endif // !Y6COPTER

	#endif // MULTICOPTER

	#endif // !SUPPRESS_I2C_ESC_INIT

} // InitI2CESCs

void StopMotors(void)
{
	#ifdef MULTICOPTER
		#ifdef Y6COPTER
			PWM[FrontTC] = PWM[LeftTC] = PWM[RightTC] = 
			PWM[FrontBC] = PWM[LeftBC] = PWM[RightBC] = ESCMin;
		#else
			PWM[FrontC] = PWM[LeftC] = PWM[RightC] = ESCMin;
			#ifndef TRICOPTER
				PWM[BackC] = ESCMin;
			#endif // !TRICOPTER
		#endif // Y6COPTER	
	#else
		PWM[ThrottleC] = ESCMin;
	#endif // MULTICOPTER

	DesiredThrottle = 0;
	F.MotorsArmed = false;
} // StopMotors

void InitMotors(void)
{
	StopMotors();

	#ifndef Y6COPTER
		#ifdef TRICOPTER
			PWM[BackC] = OUT_NEUTRAL;
		#endif // !TRICOPTER	
		PWM[CamRollC] = OUT_NEUTRAL;
		PWM[CamPitchC] = OUT_NEUTRAL;
	#endif // Y6COPTER

} // InitMotors






