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

void ShowESCType(void);
uint8 PWLimit(int16);
uint8 I2CESCLimit(int16);
void DoMulticopterMix(int16);
void CheckDemand(int16);
void MixAndLimitMotors(void);
void MixAndLimitCam(void);
void OutSignals(void);
void StopMotors(void);
void InitMotors(void);

int16 PW[6];
int16 PWSense[6];
int16 ESCI2CFail[NO_OF_I2C_ESCS];
int16 CurrThrottle;
uint8 ServoInterval;

#pragma udata access motorvars	
near int16 Rl, Pl, Yl;
near uint8 SHADOWB, PW0, PW1, PW2, PW3, PW4, PW5;
#pragma udata

int16 ESCMax;

#pragma idata escnames
const rom char * ESCName[ESCUnknown+1] = {
		"PPM","Holger","X3D I2C","YGE I2C","LRC I2C","Unk"
		};
#pragma idata

void ShowESCType(void)
{
	TxString(ESCName[P[ESCType]]);
} // ShowESCType

uint8 PWLimit(int16 T)
{
	return((uint8)Limit(T,1,OUT_MAXIMUM));
} // PWLimit

uint8 I2CESCLimit(int16 T)
{
	return((uint8)Limit(T,0,ESCMax));
} // I2CESCLimit

#ifdef MULTICOPTER

void DoMulticopterMix(int16 CurrThrottle)
{
	static int16 Temp, B;

	#if ( defined Y6COPTER ) | ( defined HEXACOPTER )
		PW[K1] = PW[K2] = PW[K3] = PW[K4] = PW[K5] = PW[K6] = CurrThrottle;
	#else
		PW[FrontC] = PW[LeftC] = PW[RightC] = PW[BackC] = CurrThrottle;
	#endif

	#if defined TRICOPTER // usually flown K1 motor to the rear - use orientation of 24
		Temp = SRS16(Pl, 1); 			
		PW[FrontC] -= Pl;
		PW[LeftC] += (Temp - Rl);
		PW[RightC] += (Temp + Rl);
	
		PW[BackC] = PWSense[RudderC] * Yl + OUT_NEUTRAL;	// yaw servo
		if ( P[Balance] != 0 )
		{
			B = 128 + P[Balance];
			PW[FrontC] =  SRS32((int32)PW[FrontC] * B, 7);
		}
	#elif defined VTCOPTER 	// usually flown VTail (K1+K4) to the rear - use orientation of 24
		Temp = SRS16(Pl, 1); 	

		PW[LeftC] += (Temp - Rl);	// right rear
		PW[RightC] += (Temp + Rl); // left rear
	
		PW[FrontLeftC] -= Pl + PWSense[RudderC] * Yl; 
		PW[FrontRightC] -= Pl - PWSense[RudderC] * Yl;
		if ( P[Balance] != 0 )
		{
			B = 128 + P[Balance];
			PW[FrontLeftC] = SRS32((int32)PW[FrontLeftC] * B, 7);
			PW[FrontRightC] = SRS32((int32)PW[FrontRightC] * B, 7);
		}
	#elif defined Y6COPTER
		Temp = SRS16(Pl, 1); 			
		PW[FrontTC] -= Pl;
		PW[LeftTC] += (Temp - Rl);
		PW[RightTC] += (Temp + Rl);
			
		PW[FrontBC] = PW[FrontTC];
		PW[LeftBC]  = PW[LeftTC];
		PW[RightBC] = PW[RightTC];

		PW[FrontTC] += Yl;
		PW[LeftTC]  += Yl;
		PW[RightTC] += Yl;

		PW[FrontBC] -= Yl;
		PW[LeftBC]  -= Yl; 
		PW[RightBC] -= Yl; 
	#elif defined HEXACOPTER
		Temp = SRS16(Pl, 1); 			
		PW[HFrontC] += -Pl + Yl;
		PW[HLeftBackC] += (Temp - Rl) + Yl;
		PW[HRightBackC] += (Temp + Rl) + Yl; 
						
		PW[HBackC] += Pl - Yl;
		PW[HLeftFrontC] += (-Temp - Rl) - Yl;
		PW[HRightFrontC] += (-Temp + Rl) - Yl; 
	
		PW[HFrontC] += Yl;
		PW[HRightBackC] += Yl;
		PW[HLeftBackC] += Yl;
	
		PW[HBackC] -= Yl;
		PW[HRightFrontC] -= Yl; 
		PW[HLeftFrontC] -= Yl; 
	#else
		PW[LeftC]  += -Rl - Yl;	
		PW[RightC] +=  Rl - Yl;
		PW[FrontC] += -Pl + Yl;
		PW[BackC]  +=  Pl + Yl;
	#endif

} // DoMulticopterMix

boolean MotorDemandRescale;

void CheckDemand(int16 CurrThrottle)
{
	static int24 Scale, ScaleHigh, ScaleLow, MaxMotor, DemandSwing;
	static i24u Temp;

	#if ( defined Y6COPTER ) | ( defined HEXACOPTER )
		MaxMotor = Max(PW[K1], PW[K2]);
		MaxMotor = Max(MaxMotor, PW[K3]);
		MaxMotor = Max(MaxMotor, PW[K4]);
		MaxMotor = Max(MaxMotor, PW[K5]);
		MaxMotor = Max(MaxMotor, PW[K6]);
	#else
		MaxMotor = Max(PW[FrontC], PW[LeftC]);
		MaxMotor = Max(MaxMotor, PW[RightC]);
		#ifndef TRICOPTER
			MaxMotor = Max(MaxMotor, PW[BackC]);
		#endif // TRICOPTER
	#endif // Y6COPTER | HEXACOPTER

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
	{
		CurrThrottle = DesiredThrottle;
		if ( State == InFlight )
		{
			CurrThrottle += AltComp; 
			CurrThrottle = Limit(CurrThrottle, IdleThrottle, (OUT_MAXIMUM * 9)/10 );
		}
	}

	#if defined MULTICOPTER
			
		if ( CurrThrottle > IdleThrottle )
		{
			DoMulticopterMix(CurrThrottle);			
			CheckDemand(CurrThrottle);			
			if ( MotorDemandRescale )
				DoMulticopterMix(CurrThrottle);
		}
		else
		{
			#if ( defined Y6COPTER ) | ( defined HEXACOPTER )
				for ( m = 0; m < (uint8)6; m++ )
					PW[m] = CurrThrottle;
			#else
				PW[FrontC] = PW[LeftC] = PW[RightC] = CurrThrottle;
				#if defined TRICOPTER
					PW[BackC] = PWSense[RudderC] * Yl + OUT_NEUTRAL;	// yaw servo
				#else
					PW[BackC] = CurrThrottle;
				#endif // !TRICOPTER
			#endif // Y6COPTER | HEXACOPTER
		}
	#elif ( defined VTOL )
		PW[ThrottleC] = CurrThrottle + AltComp;
		PW[RollC] = PWSense[RollC] * Rl + OUT_NEUTRAL;
			
		TempElevator = PWSense[2] * Pl + OUT_NEUTRAL;
		PW[RightPitchYawC] = PWSense[RightPitchYawC] * (TempElevator + Yl);
		PW[LeftPitchYawC] = PWSense[LeftPitchYawC] * (TempElevator -  Yl);
	#else	
		PW[ThrottleC] = CurrThrottle; //+ AltComp;
		PW[RudderC] = PWSense[RudderC] * Yl + OUT_NEUTRAL;
			
		#if ( defined AILERON | defined HELICOPTER )
			PW[AileronC] = PWSense[AileronC] * Rl + OUT_NEUTRAL;
			PW[ElevatorC] = PWSense[ElevatorC] * Pl + OUT_NEUTRAL;
		#else // ELEVON
			TempElevator = PWSense[2] * Pl + OUT_NEUTRAL;
			PW[RightElevonC] = PWSense[RightElevonC] * (TempElevator + Rl);
			PW[LeftElevonC] = PWSense[LeftElevonC] * (TempElevator -  Rl);		
		#endif
	#endif

} // MixAndLimitMotors


void MixAndLimitCam(void)
{
	#if ( defined Y6COPTER ) | ( defined HEXACOPTER )
		// NO CAMERA
	#else

	#ifndef SIMULATE

	static i24u Temp24;
	static int32 NewCamRoll, NewCamPitch;

	// use only roll/pitch angle estimates

	Temp24.i24 = A[Pitch].Angle * OSO + A[Roll].Angle * OCO;
	CameraAngle[Roll] = Temp24.i2_1;
	Temp24.i24 = A[Pitch].Angle * OCO - A[Roll].Angle * OSO;
	CameraAngle[Pitch] = Temp24.i2_1;

	// zzz this where we lose the resolution say 20/256 so need camera gimbal gearing if ~12.

	Temp24.i24 = (int24)CameraAngle[Roll] * P[CamRollKp];
	NewCamRoll = Temp24.i2_1 + (int16)P[CamRollTrim];
	NewCamRoll = PWSense[CamRollC] * NewCamRoll + OUT_NEUTRAL;
	PW[CamRollC] = SlewLimit( PW[CamRollC], NewCamRoll, 2);

	Temp24.i24 = (int24)CameraAngle[Pitch] * P[CamPitchKp];
	NewCamPitch = Temp24.i2_1 + SRS16(DesiredCamPitchTrim * 3, 1);
	NewCamPitch = PWSense[CamPitchC] * NewCamPitch + OUT_NEUTRAL; 
	PW[CamPitchC] = SlewLimit( PW[CamPitchC], NewCamPitch, 2);

	#else

	PW[CamPitchC] = PW[CamRollC] = OUT_NEUTRAL;

	#endif // SIMULATE

	#endif // !(Y6COPTER | HEXACOPTER }

} // MixAndLimitCam

#include "outputs_i2c.h"

#if defined CLOCK_40MHZ
	#include "outputs_40.h"
#elif defined MULTICOPTER
	#include "outputs_copter.h"
#else
	#include "outputs_conventional.h"
#endif // CLOCK_40MHZ

void StopMotors(void)
{
	#if defined MULTICOPTER
		#if ( defined Y6COPTER ) | ( defined HEXACOPTER )
			PW[K1] = PW[K2] = PW[K3] = PW[K4] = PW[K5] = PW[K6] = 0;
		#else
			PW[FrontC] = PW[LeftC] = PW[RightC] = 0;
			#ifndef TRICOPTER
				PW[BackC] = 0;
			#endif // !TRICOPTER
		#endif // Y6COPTER | HEXACOPTER	
	#else
		PW[ThrottleC] = 0;
	#endif // MULTICOPTER

	DesiredThrottle = 0;
	F.MotorsArmed = false;
} // StopMotors


void InitMotors(void)
{
	StopMotors();

	#ifndef Y6COPTER
		#ifdef TRICOPTER
			PW[BackC] = OUT_NEUTRAL;
		#endif // !TRICOPTER	
		PW[CamRollC] = OUT_NEUTRAL;
		PW[CamPitchC] = OUT_NEUTRAL;
	#endif // Y6COPTER

} // InitMotors




