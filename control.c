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

void DoAltitudeHold(void);
int24 AltitudeCF(int24);
void AltitudeHold(void);
void DoAttitudeAngle(AxisStruct *);
void DoYawRate(void);
void DoOrientationTransform(void);
void GainSchedule(void);
void DoControl(void);

void CheckThrottleMoved(void);
void InitControl(void);

#pragma udata axisvars			
AxisStruct A[3];
#pragma udata

uint8 PIDCyclemS, PIDCycleShift;

#pragma udata hist
uint32 CycleHist[16];
#pragma udata
		
int16 CameraAngle[3];

int16 Ylp;				
int24 OSO, OCO;

int32 AngleE, RateE;
int16 YawRateIntE;
int16 HoldYaw;

int16 CurrMaxRollPitch;

int16 AttitudeHoldResetCount;
int24 DesiredAltitude, Altitude, Altitudep; 
int16 AccAltComp, AltComp, BaroROC, BaroROCp, RangefinderROC, ROC, ROCIntE, MinROCCmpS;

int32 GS;

int8 BeepTick = 0;

void DoAltitudeHold(void)
{ 	// Syncronised to baro intervals independant of active altitude source
	
	static int24 AltE;
	static int16 ROCE, pROC, iROC, DesiredROC;
	static int16 NewAltComp;

	#ifdef ALT_SCRATCHY_BEEPER
	if ( (--BeepTick <= 0) && !F.BeeperInUse ) 
		Beeper_TOG;
	#endif
					
	AltE = DesiredAltitude - Altitude;
		
	DesiredROC = Limit(AltE, MinROCCmpS, ALT_MAX_ROC_CMPS);
		
	ROCE = DesiredROC - ROC;		
	pROC = ROCE * (int16)P[AltKp]; 
		
	ROCIntE += ROCE  * (int16)P[AltKi];
	ROCIntE = Limit1(ROCIntE, (int16)P[AltIntLimit]);
	iROC = ROCIntE;
				
	NewAltComp = SRS16(pROC + iROC, 6);
	AltComp = Limit1(NewAltComp, ALT_MAX_THR_COMP);
					
	#ifdef ALT_SCRATCHY_BEEPER
	if ( (BeepTick <= 0) && !F.BeeperInUse) 
	{
		Beeper_TOG;
		BeepTick = 5;
	}
	#endif

} // DoAltitudeHold	

void AltitudeHold()
{  // relies upon good cross calibration of baro and rangefinder!!!!!!

	static int16 ActualThrottle;

	if ( SpareSlotTime && ( F.NormalFlightMode || F.AltHoldEnabled ) )
	{
		GetBaroAltitude();
		if ( F.NewBaroValue )
		{
			SpareSlotTime = false;

			GetRangefinderAltitude();
			CheckThrottleMoved();
			
			if ( F.UsingRangefinderAlt )
			{
				Altitude = RangefinderAltitude;	 
				ROC = RangefinderROC;
			}
			else
			{
				Altitude = BaroRelAltitude;
				ROC = BaroROC;
			}
		}
	}
	else
		Altitude = ROC = 0;	// for GS display
			
	if ( F.AltHoldEnabled )
	{
		if ( F.NewBaroValue )
		{	
			SpareSlotTime = false;	
			if ( F.ForceFailsafe || (( NavState != HoldingStation ) && F.AllowNavAltitudeHold )) 
			{  // Navigating - using CruiseThrottle
				F.HoldingAlt = true;
				DoAltitudeHold();
			}	
			else
				if ( F.ThrottleMoving )
				{
					F.HoldingAlt = false;
					SetDesiredAltitude(Altitude);
					AltComp = Decay1(AltComp);
				}
				else
				{
					F.HoldingAlt = true;
					DoAltitudeHold(); // not using cruise throttle

					#ifndef SIMULATE
						ActualThrottle = DesiredThrottle + AltComp;
						if (( Abs(ROC) < ALT_HOLD_MAX_ROC_CMPS ) && ( ActualThrottle > THROTTLE_MIN_CRUISE )) 
						{
							NewCruiseThrottle = HardFilter(NewCruiseThrottle, ActualThrottle);
							NewCruiseThrottle = Limit(NewCruiseThrottle, THROTTLE_MIN_CRUISE, THROTTLE_MAX_CRUISE );
							CruiseThrottle = NewCruiseThrottle;
						}
					#endif // !SIMULATE
				}
			F.NewBaroValue = false;
		}	
	}
	else
	{
		F.NewBaroValue = false;
		ROCIntE = 0;
		AltComp = Decay1(AltComp);
		F.HoldingAlt = false;
	}
} // AltitudeHold

void DoAttitudeAngle(AxisStruct *C)
{	
	// Angles and rates are normal aircraft coordinate conventions
	// X/Forward axis reversed for Acc to simplify compensation
	
	static int16 a, Temp;

	Temp = SRS16(C->Rate, 2 - PIDCycleShift );

	#ifdef INC_RAW_ANGLES
	C->RawAngle += Temp; // for Acc comp studies
	#endif

	a = C->Angle + Temp;

	a = Limit1(a, ANGLE_LIMIT_DEG); // turn off comp above this angle?
	a = Decay1(a);
	a -= C->AngleCorr;			// last for accelerometer compensation
	C->Angle = a;

} // DoAttitudeAngle

void DoYawRate(void)
{ 	// Yaw gyro compensation using compass
	static int16 HE;

	A[Yaw].Rate = SRS16(A[Yaw].Rate, 2 - PIDCycleShift);

	if ( F.CompassValid && F.NormalFlightMode )
	{
		// + CCW
		if ( A[Yaw].Hold > COMPASS_MIDDLE ) // acquire new heading
		{
			DesiredHeading = Heading;
			A[Yaw].Ratep = A[Yaw].Rate;
		}
		else
		{
			A[Yaw].Rate = YawFilter(A[Yaw].Ratep, A[Yaw].Rate);
			A[Yaw].Ratep = A[Yaw].Rate;
			HE = MinimumTurn(DesiredHeading - Heading);
			HE = Limit1(HE, SIXTHMILLIPI); // 30 deg limit
			HE = SRS32((int24)HE * (int24)P[CompassKp], 12); 
			A[Yaw].Rate -= Limit1(HE, COMPASS_MAXDEV); // yaw gyro drift compensation
		}
	}

} // DoYawRate

void YawControl(void)
{
	static int16 RateE;
	static i24u Temp;
	
	DoYawRate();
		
	RateE = A[Yaw].Rate + ( A[Yaw].Desired + A[Yaw].NavCorr );
	Temp.i24  = (int24)RateE * (int16)A[Yaw].Kp;	
		
	#if defined TRICOPTER
		Temp.i2_1 = SlewLimit(A[Yaw].Outp, Temp.i2_1, 2);				
		A[Yaw].Outp = Temp.i2_1;
		A[Yaw].Out = Limit1(Temp.i2_1,(int16)P[YawLimit]);
	#else
		A[Yaw].Out = Limit1(Temp.i2_1, (int16)P[YawLimit]);
	#endif // TRICOPTER

	A[Yaw].RateEp = RateE;

} // YawControl

void DoOrientationTransform(void)
{
	static i24u Temp;

	OSO = OSin[Orientation];
	OCO = OCos[Orientation];

	if ( !F.NavigationActive )
		A[Roll].NavCorr = A[Pitch].NavCorr = A[Yaw].NavCorr = 0;

	// -PS+RC
	Temp.i24 = -A[Pitch].Desired * OSO + A[Roll].Desired * OCO;
	A[Roll].Control = Temp.i2_1;
		
	// PC+RS
	Temp.i24 = A[Pitch].Desired * OCO + A[Roll].Desired * OSO;
	A[Pitch].Control = Temp.i2_1; 

} // DoOrientationTransform

#ifdef TESTING

void GainSchedule(void)
{
	GS = 256;
} // GainSchedule

#else

void GainSchedule(void)
{  // rudimentary gain scheduling (linear)
	static int16 AttDiff, ThrDiff;

	GS = 256;

	if ( (!F.NavigationActive) || ( F.NavigationActive && (NavState == HoldingStation ) ) )
	{
		// also density altitude?
	
		if (( !F.NormalFlightMode ) && ( P[Acro] > 0)) // due to Foliage 2009 and Alexinparis 2010
		{
		 	AttDiff = CurrMaxRollPitch  - ATTITUDE_HOLD_LIMIT;
			GS = (int32)GS * ( 2048L - (AttDiff * (int16)P[Acro]) );
			GS = SRS32(GS, 11);
			GS = Limit(GS, 0, 256);
		}
	
		if ( P[GSThrottle] > 0 ) 
		{
		 	ThrDiff = DesiredThrottle - CruiseThrottle;
			GS = (int32)GS * ( 2048L + (ThrDiff * (int16)P[GSThrottle]) );
			GS = SRS32(GS, 11);
		}	
	}
	
} // GainSchedule

#endif

#include "exp_control.h"

void DoControl(void)
{
	static int16 Temp;
	static i24u Temp24;
	static i32u Temp32;
	static uint8 a;
	static AxisStruct *C;

	GetGyroValues();
	CalculateGyroRates();
	CompensateRollPitchGyros();

	DoOrientationTransform();

	#if defined SIMULATE

		for ( a = Roll; a<=(uint8)Yaw; a++ )
		{
			C = &A[a];
			C->Control += C->NavCorr;
			C->FakeDesired = C->Control;
			C->Angle = SlewLimit(C->Angle, -C->FakeDesired * 32, 16);
			C->Out = -C->FakeDesired;
		}

		A[Yaw].Angle = SlewLimit(A[Yaw].Angle, A[Yaw].FakeDesired, 8);
		A[Yaw].Out = -A[Yaw].Desired;
	
	//	CameraAngle[Roll] = A[Roll].Angle;
	//	CameraAngle[Pitch] = A[Pitch].Angle;
	 
    #else

		GainSchedule();

		for ( a = Roll; a<=(uint8)Pitch; a++)
		{
			C = &A[a];
			DoAttitudeAngle(C);
			C->Control += C->NavCorr;
			
			if ( F.UsingAltControl )	
				CONTROLLER(C);
			else
				Do_Wolf_Rate(C);
		}			
			
		YawControl();
	
	#endif // SIMULATE

	F.NearLevel = Max(Abs(A[Roll].Angle), Abs(A[Pitch].Angle)) < NAV_RTH_LOCKOUT;
	Rl = A[Roll].Out; Pl = A[Pitch].Out; Yl = A[Yaw].Out;

} // DoControl

void InitControl(void)
{
	static uint8 a;
	static AxisStruct *C;

	Ylp = AltComp = ROCIntE = 0;
	YawRateIntE = 0;

	for ( a = Roll; a<=(uint8)Yaw; a++)
	{
		C = &A[a];
		C->Outp = C->RateEp = C->Ratep = C->RateIntE = 0;
	}

} // InitControl



