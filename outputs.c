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
uint8 PWMLimit(int16);
uint8 I2CESCLimit(int16);
void DoMulticopterMix(int16);
void CheckDemand(int16);
void MixAndLimitMotors(void);
void MixAndLimitCam(void);
void OutSignals(void);
void StopMotors(void);
void InitMotors(void);

int16 PWM[6];
int16 PWMSense[6];
int16 ESCI2CFail[NO_OF_I2C_ESCS];
int16 CurrThrottle;
int8 ServoInterval;

#pragma udata access motorvars	
near int16 Rl, Pl, Yl;
near uint8 SHADOWB, PWM0, PWM1, PWM2, PWM3, PWM4, PWM5;
#pragma udata

int16 ESCMax;

#pragma idata escnames
const rom char * ESCName[ESCUnknown+1] = {
		"PPM","Holger","X3D I2C","YGE I2C","LRC I2C","Unknown"
		};
#pragma idata

void ShowESCType(void)
{
	TxString(ESCName[P[ESCType]]);
} // ShowESCType

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

	#if ( defined Y6COPTER ) | ( defined HEXACOPTER )
		PWM[K1] = PWM[K2] = PWM[K3] = PWM[K4] = PWM[K5] = PWM[K6] = CurrThrottle;
	#else
		PWM[FrontC] = PWM[LeftC] = PWM[RightC] = PWM[BackC] = CurrThrottle;
	#endif

	#if defined TRICOPTER // usually flown K1 motor to the rear - use orientation of 24
		Temp = SRS16(Pl, 1); 			
		PWM[FrontC] -= Pl;
		PWM[LeftC] += (Temp - Rl);
		PWM[RightC] += (Temp + Rl);
	
		PWM[BackC] = PWMSense[RudderC] * Yl + OUT_NEUTRAL;	// yaw servo
		if ( P[Balance] != 0 )
		{
			B = 128 + P[Balance];
			PWM[FrontC] =  SRS32((int32)PWM[FrontC] * B, 7);
		}
	#elif defined VTCOPTER 	// usually flown VTail (K1+K4) to the rear - use orientation of 24
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
	#elif defined Y6COPTER
		Temp = SRS16(Pl, 1); 			
		PWM[FrontTC] -= Pl;
		PWM[LeftTC] += (Temp - Rl);
		PWM[RightTC] += (Temp + Rl);
			
		PWM[FrontBC] = PWM[FrontTC];
		PWM[LeftBC]  = PWM[LeftTC];
		PWM[RightBC] = PWM[RightTC];

		PWM[FrontTC] += Yl;
		PWM[LeftTC]  += Yl;
		PWM[RightTC] += Yl;

		PWM[FrontBC] -= Yl;
		PWM[LeftBC]  -= Yl; 
		PWM[RightBC] -= Yl; 
	#elif defined HEXACOPTER
		Temp = SRS16(Pl, 1); 			
		PWM[HFrontC] += -Pl + Yl;
		PWM[HLeftBackC] += (Temp - Rl) + Yl;
		PWM[HRightBackC] += (Temp + Rl) + Yl; 
						
		PWM[HBackC] += Pl - Yl;
		PWM[HLeftFrontC] += (-Temp - Rl) - Yl;
		PWM[HRightFrontC] += (-Temp + Rl) - Yl; 
	
		PWM[HFrontC] += Yl;
		PWM[HRightBackC] += Yl;
		PWM[HLeftBackC] += Yl;
	
		PWM[HBackC] -= Yl;
		PWM[HRightFrontC] -= Yl; 
		PWM[HLeftFrontC] -= Yl; 
	#else
		PWM[LeftC]  += -Rl - Yl;	
		PWM[RightC] +=  Rl - Yl;
		PWM[FrontC] += -Pl + Yl;
		PWM[BackC]  +=  Pl + Yl;
	#endif

} // DoMulticopterMix

boolean 	MotorDemandRescale;

void CheckDemand(int16 CurrThrottle)
{
	static int24 Scale, ScaleHigh, ScaleLow, MaxMotor, DemandSwing;
	static i24u Temp;

	#if ( defined Y6COPTER ) | ( defined HEXACOPTER )
		MaxMotor = Max(PWM[K1], PWM[K2]);
		MaxMotor = Max(MaxMotor, PWM[K3]);
		MaxMotor = Max(MaxMotor, PWM[K4]);
		MaxMotor = Max(MaxMotor, PWM[K5]);
		MaxMotor = Max(MaxMotor, PWM[K6]);
	#else
		MaxMotor = Max(PWM[FrontC], PWM[LeftC]);
		MaxMotor = Max(MaxMotor, PWM[RightC]);
		#ifndef TRICOPTER
			MaxMotor = Max(MaxMotor, PWM[BackC]);
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
					PWM[m] = CurrThrottle;
			#else
				PWM[FrontC] = PWM[LeftC] = PWM[RightC] = CurrThrottle;
				#if defined TRICOPTER
					PWM[BackC] = PWMSense[RudderC] * Yl + OUT_NEUTRAL;	// yaw servo
				#else
					PWM[BackC] = CurrThrottle;
				#endif // !TRICOPTER
			#endif // Y6COPTER | HEXACOPTER
		}
	#elif ( defined VTOL )
		PWM[ThrottleC] = CurrThrottle + AltComp;
		PWM[RollC] = PWMSense[RollC] * Rl + OUT_NEUTRAL;
			
		TempElevator = PWMSense[2] * Pl + OUT_NEUTRAL;
		PWM[RightPitchYawC] = PWMSense[RightPitchYawC] * (TempElevator + Yl);
		PWM[LeftPitchYawC] = PWMSense[LeftPitchYawC] * (TempElevator -  Yl);
	#else	
		PWM[ThrottleC] = CurrThrottle; //+ AltComp;
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
	#if ( defined Y6COPTER ) | ( defined HEXACOPTER )
		// NO CAMERA
	#else

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
	NewCamRoll = PWMSense[CamRollC] * NewCamRoll + OUT_NEUTRAL;
	PWM[CamRollC] = SlewLimit( PWM[CamRollC], NewCamRoll, 2);

	Temp24.i24 = (int24)CameraAngle[Pitch] * P[CamPitchKp];
	NewCamPitch = Temp24.i2_1 + SRS16(DesiredCamPitchTrim * 3, 1);
	NewCamPitch = PWMSense[CamPitchC] * NewCamPitch + OUT_NEUTRAL; 
	PWM[CamPitchC] = SlewLimit( PWM[CamPitchC], NewCamPitch, 2);

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
			PWM[K1] = PWM[K2] = PWM[K3] = PWM[K4] = PWM[K5] = PWM[K6] = 0;
		#else
			PWM[FrontC] = PWM[LeftC] = PWM[RightC] = 0;
			#ifndef TRICOPTER
				PWM[BackC] = 0;
			#endif // !TRICOPTER
		#endif // Y6COPTER | HEXACOPTER	
	#else
		PWM[ThrottleC] = 0;
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




