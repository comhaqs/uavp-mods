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
void DoAttitudeAngle(uint8, uint8);
void DoYawRate(void);
void DoOrientationTransform(void);
void GainSchedule(void);
void DoControl(void);

void CheckThrottleMoved(void);
void InitControl(void);

#pragma udata access motorvars	
near int16 Rl, Pl, Yl;
#pragma udata
			
int16 Angle[3], RawAngle[3];		
int16 CameraRollAngle, CameraPitchAngle, CameraRollAnglep, CameraPitchAnglep;

int16 Ylp;				
int24 OSO, OCO;

int16 YawRateIntE;
int16 HoldYaw;
int16 YawIntLimit256;
int16 RateE[3], RateEp[3];

int16 ControlRoll, ControlPitch, CurrMaxRollPitch;

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
	static i24u Temp;

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

void DoAttitudeAngle(uint8 a, uint8 c)
{	// Caution: Angles are the opposite to the normal aircraft coordinate conventions
	static int16 Temp;

	#ifdef CLOCK_16MHZ
		Temp = Angle[a] + Rate[a];
	#else
		Temp = Angle[a] + SRS16(Rate[a], 1);
	#endif // CLOCK_16MHZ
	Temp = Limit1(Temp, ANGLE_LIMIT);
	Temp = Decay1(Temp);
	Temp += IntCorr[c];			// last for accelerometer compensation
	Angle[a] = Temp;

} // DoAttitudeAngle

void DoYawRate(void)
{ 	// Yaw gyro compensation using compass
	static int16 HE;

	if ( F.CompassValid && F.NormalFlightMode )
	{
		// + CCW
		if ( Hold[Yaw] > COMPASS_MIDDLE ) // acquire new heading
		{
			RawYawRateP = Rate[Yaw];
			DesiredHeading = Heading;
		}
		else
		{
			Rate[Yaw] = YawFilter(RawYawRateP, Rate[Yaw]);
			RawYawRateP = Rate[Yaw];
			HE = MinimumTurn(DesiredHeading - Heading);
			HE = Limit1(HE, SIXTHMILLIPI); // 30 deg limit
			HE = SRS32((int24)HE * (int24)P[CompassKp], 12); 
			Rate[Yaw] -= Limit1(HE, COMPASS_MAXDEV); // yaw gyro drift compensation
		}
	}

	#ifdef CLOCK_16MHZ
		Angle[Yaw] += Rate[Yaw];
	#else
		Angle[Yaw] += SRS16(Rate[Yaw], 1);
	#endif // CLOCK_16MHZ
	
	Angle[Yaw] = Limit1(Angle[Yaw], YawIntLimit256);
	Angle[Yaw] = DecayX(Angle[Yaw], 2);

} // DoYawRate

void DoOrientationTransform(void)
{
	static i24u Temp;

	OSO = OSin[Orientation];
	OCO = OCos[Orientation];

	if ( !F.NavigationActive )
		NavCorr[Roll] = NavCorr[Pitch] = NavCorr[Yaw] = 0;

	// -PS+RC
	Temp.i24 = -DesiredPitch * OSO + DesiredRoll * OCO;
	ControlRoll = Temp.i2_1;
		
	// PC+RS
	Temp.i24 = DesiredPitch * OCO + DesiredRoll * OSO;
	ControlPitch = Temp.i2_1; 

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

void DoControl(void)
{
	static int32 Temp;
	static i32u Temp32;
	static i24u Temp24;

	CalculateGyroRates();
	CompensateRollPitchGyros();	

	DoOrientationTransform();

	#ifdef SIMULATE

		FakeDesiredRoll = ControlRoll + NavCorr[Roll];
		FakeDesiredPitch = ControlPitch + NavCorr[Pitch];
		FakeDesiredYaw =  DesiredYaw + NavCorr[Yaw];
		Angle[Roll] = SlewLimit(Angle[Roll], -FakeDesiredRoll * 32, 16);
		Angle[Pitch] = SlewLimit(Angle[Pitch], -FakeDesiredPitch * 32, 16);
		Angle[Yaw] = SlewLimit(Angle[Yaw], FakeDesiredYaw, 8);
		Rl = -FakeDesiredRoll;
		Pl = -FakeDesiredPitch;
		Yl = -DesiredYaw;
	
		CameraRollAngle = Angle[Roll];
		CameraPitchAngle = Angle[Pitch];
	 
    #else

		DoAttitudeAngle(Roll, LR);
		DoAttitudeAngle(Pitch, FB);

		#define D_LIMIT 32
	
		#ifdef EXPERIMENTAL		// define is set in uavx.h
	
			#include "exp_control.h"
	
		#else // DEFAULT
		
		#ifndef TESTING		
			if ( F.UsingAltControl )
			{ 	// Angle - No gain scheduling for now 

				// Roll
				RateE[Roll] = SRS32(((ControlRoll + NavCorr[Roll]) * RC_STICK_ANGLE_SCALE - Angle[Roll]) * P[RollKp], 5) - Rate[Roll];

				Rl =  SRS32(RateE[Roll] * P[RollKi], 7);

				#ifdef CLOCK_16MHZ
			 		Temp = SRS32((int32)(RateE[Roll] - RateEp[Roll]) * P[RollKd], 1);
				#else
					Temp = (int32)(Rate[Roll] - RateEp[Roll]) * P[RollKd];
				#endif // CLOCK_16MHZ
				Temp = Limit1(Temp, D_LIMIT);
				Rl -= Temp;
				Rl = SRS16(Rl, 2);

				// Pitch

				// Roll
				RateE[Pitch] = SRS32(((ControlPitch + NavCorr[Pitch]) * RC_STICK_ANGLE_SCALE - Angle[Pitch]) * P[PitchKp], 5) - Rate[Pitch];

				Pl =  SRS32(RateE[Pitch] * P[PitchKi], 7);

				#ifdef CLOCK_16MHZ
			 		Temp = SRS32((int32)(RateE[Pitch] - RateEp[Pitch]) * P[Pitch], 1);
				#else
					Temp = (int32)(Rate[Pitch] - RateEp[Pitch]) * P[PitchKd];
				#endif // CLOCK_16MHZ
				Temp = Limit1(Temp, D_LIMIT);
				Pl -= Temp;
				Pl = SRS16(Pl, 2);
			}
			else
			#endif // !TESTING	
			{	// Rate - Wolf's UAVP Original

				GainSchedule();

				// Roll

				Rl  = SRS32((int32)Rate[Roll] * P[RollKp], 5);			
				#ifdef CLOCK_16MHZ
			 		Temp = SRS32((int32)(Rate[Roll] - Ratep[Roll]) * P[RollKd], 5);
				#else
					Temp = SRS32((int32)(Rate[Roll] - Ratep[Roll]) * P[RollKd], 4);
				#endif // CLOCK_16MHZ
				Temp = Limit1(Temp, D_LIMIT);
				Rl -= Temp;	
		
				Temp = SRS32((int32)Angle[Roll] * P[RollKi], 9);
				Rl += Limit1(Temp, (int16)P[RollIntLimit]); 
			
				Temp24.i24 = (int24)Rl * GS;
				Rl = Temp24.i2_1;

				// Pitch
			
				Rl -= (ControlRoll + NavCorr[Roll]);

				Pl  = SRS32((int32)Rate[Pitch] * P[PitchKp], 5);
				#ifdef CLOCK_16MHZ
			 		Temp = (int32)((Rate[Pitch] - Ratep[Pitch]) * P[PitchKd], 5);
				#else
					Temp = (int32)((Rate[Pitch] - Ratep[Pitch]) * P[PitchKd], 4);
				#endif // CLOCK_16MHZ
				Temp = Limit1(Temp, D_LIMIT);
				Pl -= Temp;	
			
				Temp = SRS32((int32)Angle[Pitch] * P[PitchKi], 9);
				Pl += Limit1(Temp, (int16)P[PitchIntLimit]);

				Temp24.i24 = (int24)Pl * GS;
				Pl = Temp24.i2_1;
			
				Pl -= ( ControlPitch + NavCorr[Pitch]);
			}

			Ratep[Roll] = Rate[Roll];
			Ratep[Pitch] = Rate[Pitch];
	
		#endif // EXPERIMENTAL
	
		// Yaw - rate control
	
		#ifdef NAV_WING
	
			Yl = DesiredYaw + NavCorr[Yaw];
	
		#else
		
			DoYawRate();
		
			RateE[Yaw] = Rate[Yaw] + ( DesiredYaw + NavCorr[Yaw] );
			Yl  = SRS16( RateE[Yaw] * (int16)P[YawKp] + SRS16( Angle[Yaw] * P[YawKi], 4), 4);
		
			Ratep[Yaw] = Rate[Yaw];
		
			#ifdef TRICOPTER
				Yl = SlewLimit(Ylp, Yl, 2);
				Ylp = Yl;
				Yl = Limit1(Yl,(int16)P[YawLimit]);
			#else
				Yl = Limit1(Yl, (int16)P[YawLimit]);
			#endif // TRICOPTER
	
		#endif // NAV_WING
	
		Temp24.i24 = Angle[Pitch] * OSO + Angle[Roll] * OCO;
		CameraRollAngle = Temp24.i2_1;
		Temp24.i24 = Angle[Pitch] * OCO - Angle[Roll] * OSO;
		CameraPitchAngle = Temp24.i2_1;

	#endif // SIMULATE	

	F.NearLevel = Max(Abs(Angle[Roll]), Abs(Angle[Pitch])) < NAV_RTH_LOCKOUT;

} // DoControl

void InitControl(void)
{
	uint8 a;

	CameraRollAngle = CameraPitchAngle = 0;
	Ylp = AltComp = ROCIntE = 0;
	YawRateIntE = 0;

	for ( a = 0; a <(uint8)3; a++)
		RateEp[a] = Ratep[a] = 0;

} // InitControl



