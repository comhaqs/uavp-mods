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
void AltitudeHold(void);
void DoOrientationTransform(void);
void GainSchedule(void);
void DoControl(void);

void CheckThrottleMoved(void);
void InitControl(void);

#pragma udata axisvars			
AxisStruct A[3];
#pragma udata

#pragma udata hist
uint32 CycleHist[16];
#pragma udata

uint8 PIDCyclemS, PIDCycleShift;
		
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

void DoAltitudeHold(void) { 	// Syncronised to baro intervals independant of active altitude source
	
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

void DoOrientationTransform(void) {
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

void GainSchedule(void) {
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

#endif // TESTING

void Do_Wolf_Rate(AxisStruct *C) { // Original by Wolfgang Mahringer
	static i24u Temp24;
	static int32 Temp, r;

	r =  SRS32((int32)C->Rate * C->Kp - (int32)(C->Rate - C->Ratep) * C->Kd, 5);
    Temp = SRS32((int32)C->Angle * C->Ki , 9);
	r += Limit1(Temp, C->IntLimit);

	Temp24.i24 = r * GS;
	r = Temp24.i2_1;

	C->Out = - ( r + C->Control );

	C->Ratep = C->Rate;
	
} // Do_Wolf_Rate

#define D_LIMIT 32*32

void Do_PD_P_Angle(AxisStruct *C)
{
	static int32 p, d, DesRate, AngleE, AngleEp, AngleIntE, RateE;

	AngleEp = C->AngleE;
	AngleIntE = C->AngleIntE;

	AngleE = C->Control * RC_STICK_ANGLE_SCALE - C->Angle;
	AngleE = Limit1(AngleE, MAX_BANK_ANGLE_DEG * DEG_TO_ANGLE_UNITS); // limit maximum demanded angle

	p = -SRS32(AngleE * C->Kp, 10);

	d = SRS32((AngleE - AngleEp) * C->Kd, 8);
//	d = Limit1(d, D_LIMIT);

	DesRate = p + d;

	RateE = DesRate - C->Rate; 	

	C->Out = SRS32(RateE * C->Kp2, 5);

	C->AngleE = AngleE;
	C->AngleIntE = AngleIntE;

} // Do_PD_P_Angle


void YawControl(void) {
	static int16 YawControl, RateE;
	static int24 Temp;

	RateE = Smooth16x16(&YawF, A[Yaw].Rate);
	
	Temp  = SRS32((int24)RateE * (int16)A[Yaw].Kp, 4);
			
	Temp = SlewLimit(A[Yaw].Outp, Temp, 1);	
	A[Yaw].Outp = Temp;
	A[Yaw].Out = Limit1(Temp, (int16)P[YawLimit]);

	YawControl = (A[Yaw].Desired + A[Yaw].NavCorr);
	if ( Abs(YawControl) > 5 )
		A[Yaw].Out += YawControl;

} // YawControl


void DoControl(void)
{
	static uint8 a;
	static AxisStruct *C;

	GetAttitude();

	DoOrientationTransform();

	#if defined SIMULATE

		for ( a = Roll; a<=(uint8)Yaw; a++ )
		{
			C = &A[a];
			C->Control += C->NavCorr;
			C->FakeDesired = C->Control;
			C->Angle = SlewLimit(C->Angle, -C->FakeDesired * 32, 16); // zzz
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
			C->Control += C->NavCorr;
			
			if ( F.UsingAltControl )	
				Do_PD_P_Angle(C);
			else
				Do_Wolf_Rate(C);
		}			
			
		YawControl();
	
	#endif // SIMULATE

	F.NearLevel = Max(Abs(A[Roll].Angle), Abs(A[Pitch].Angle)) < NAV_RTH_LOCKOUT;
	Rl = A[Roll].Out; Pl = A[Pitch].Out; Yl = A[Yaw].Out;

	OutSignals(); 

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



