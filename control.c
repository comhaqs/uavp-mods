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

void DoAltitudeHold(int24, int16);
void UpdateAltitudeSource(void);
int16 AltitudeCF(int16);
void AltitudeHold(void);
void LimitRollAngle(void);
void LimitPitchAngle(void);
void LimitYawAngle(void);
void DoOrientationTransform(void);
void GainSchedule(void);
void DoControl(void);

void CheckThrottleMoved(void);
void LightsAndSirens(void);
void InitControl(void);

int16 RC[CONTROLS], RCp[CONTROLS];

int16 Ratep[4];					

int16 Angle[3], RawAngle[3];		
int16 CameraRollAngle, CameraPitchAngle, CameraRollAnglep, CameraPitchAnglep;
int16 Rl, Pl, Yl, Ylp;				
int24 OSO, OCO;
int16 YawFilterA;
int32 GS;

int16 Trim[3];
int16 HoldYaw;
int16 RollIntLimit2048, PitchIntLimit2048, YawIntLimit256;

int16 CruiseThrottle, MaxCruiseThrottle, DesiredThrottle, IdleThrottle, InitialThrottle, StickThrottle;
int16 DesiredRoll, DesiredPitch, DesiredYaw, DesiredHeading, DesiredCamPitchTrim, Heading;
int16 ControlRoll, ControlPitch, ControlRollP, ControlPitchP;
int16 CurrMaxRollPitch;

int16 RollOuterInp, RollOuterInpP, RollOuter, RollInnerInp;
int16 PitchOuterInp, PitchOuterInpP, PitchOuter, PitchInnerInp;

int16 YawRateIntE;

int16 ThrLow, ThrHigh, ThrNeutral;

int16 AttitudeHoldResetCount;
int16 VertVel, VertDisp, AccAltComp, AltComp, AltDiffSum, AltD, AltDSum;
int24 DesiredAltitude, Altitude;
int16 ROC;

boolean	FirstPass;
int8 BeepTick = 0;

int16 AltCF = 0;
int16 AltF[3] = { 0, 0, 0};

int16 AltitudeCF( int16 AltE ) 
{	// Complementary Filter originally authored by RoyLB
	// http://www.rcgroups.com/forums/showpost.php?p=12082524&postcount=1286

	const int16 dT = 1;
	const int16 AltTauCF = 5;

    if ( F.AccelerationsValid && F.NearLevel ) {

        VertVel += Acc[DU] * dT;
        VertDisp += VertVel * dT;

        AltF[0] = (AltE - AltCF) * Sqr(AltTauCF);
    	AltF[2] += AltF[0] * dT;
  		AltF[1] =  AltF[2] + (AltE - AltCF) * 2 * AltTauCF + VertDisp;
 		AltCF = ( AltF[1] * dT) + AltCF;

		AccAltComp = VertDisp; //AltE - AltCF; // debugging tracking

        return( AltE ); //AltCF );

    } else
        return( AltE ); // no correction

} // AltitudeCF

void DoAltitudeHold(int24 Altitude, int16 ROC)
{ // Syncronised to baro intervals independant of active altitude source
	
	static int16 NewAltComp, LimBE, AltP, AltI, AltD;
	static int24 Temp, BE;

	#ifdef ALT_SCRATCHY_BEEPER
	if ( (--BeepTick <= 0) && !F.BeeperInUse ) 
		Beeper_TOG;
	#endif
			
	BE = AltitudeCF( DesiredAltitude - Altitude );
	LimBE = Limit1(BE, ALT_BAND_DM);
		
	AltP = SRS16(LimBE * (int16)P[AltKp], 4);
	AltP = Limit1(AltP, ALT_MAX_THR_COMP);
		
	AltDiffSum += LimBE;
	AltDiffSum = Limit1(AltDiffSum, ALT_INT_WINDUP_LIMIT);
	AltI = SRS16(AltDiffSum * (int16)P[AltKi], 3);
	AltI = Limit1(AltDiffSum, (int16)P[AltIntLimit]);
				 
	AltD = SRS16(ROC * (int16)P[AltKd], 2);
	AltD = Limit1(AltD, ALT_MAX_THR_COMP); 
			 
	if ( ROC < (int16)P[MaxDescentRateDmpS] )
	{
		AltDSum += 1;
		AltDSum = Limit(AltDSum, 0, ALT_MAX_THR_COMP * 2); 
	}
	else
		AltDSum = DecayX(AltDSum, 1);
			
	NewAltComp = AltP + AltI + AltD + AltDSum;
	NewAltComp = Limit1(NewAltComp, ALT_MAX_THR_COMP);
	
	AltComp = SlewLimit(AltComp, NewAltComp, 1);
		
	#ifdef ALT_SCRATCHY_BEEPER
	if ( (BeepTick <= 0) && !F.BeeperInUse) 
	{
		Beeper_TOG;
		BeepTick = 5;
	}
	#endif

} // DoAltitudeHold	

void UpdateAltitudeSource(void)
{
	if ( F.UsingRangefinderAlt )
	{
		Altitude = RangefinderAltitude / 10; 	// Decimetres for now
		ROC = 0;
	}
	else
	{
		Altitude = BaroRelAltitude;
		ROC = BaroROC;
	}
} // UpdateAltitudeSource

void AltitudeHold()
{  // relies upon good cross calibration of baro and rangefinder!!!!!!
	static int16 NewCruiseThrottle;

	GetBaroAltitude();
	GetRangefinderAltitude();
	CheckThrottleMoved();

	if ( F.NewBaroValue  ) // sync on Baro which MUST be working
	{		
		F.NewBaroValue = false;

		UpdateAltitudeSource();

		if ( F.AltHoldEnabled )
		{
			if ( F.ForceFailsafe || (( NavState != HoldingStation ) && F.AllowNavAltitudeHold )) 
			{  // Navigating - using CruiseThrottle
				F.HoldingAlt = true;
				DoAltitudeHold(Altitude, ROC);
			}	
			else
				if ( F.ThrottleMoving )
				{
					F.HoldingAlt = false;
					DesiredAltitude = Altitude;
					AltComp = Decay1(AltComp);
// zero acc vel/disp
				}
				else
				{
					F.HoldingAlt = true;
					if ( Abs(ROC) < ALT_HOLD_MAX_ROC_DMPS  ) 
					{
						NewCruiseThrottle = DesiredThrottle + AltComp;
						CruiseThrottle = HardFilter(CruiseThrottle, NewCruiseThrottle);
						CruiseThrottle = Limit(CruiseThrottle , IdleThrottle, THROTTLE_MAX_CRUISE );
					}
					DoAltitudeHold(Altitude, ROC); // not using cruise throttle
				}	
		}
		else
		{
			AltComp = Decay1(AltComp);
			F.HoldingAlt = false;
		}	
	}

} // AltitudeHold

void LimitRollAngle(void)
{
	// Caution: Angle[Roll] is positive left which is the opposite sense to the roll angle
	RawAngle[Roll] += Rate[Roll];
	RawAngle[Roll] = Limit1(RawAngle[Roll], RollIntLimit2048);

	Angle[Roll] += Rate[Roll];
    Angle[Roll] = Limit1(Angle[Roll], RollIntLimit2048);
    Angle[Roll] = Decay1(Angle[Roll]);		// damps to zero even if still rolled
	Angle[Roll] += IntCorr[LR];				// last for accelerometer compensation
} // LimitRollAngle

void LimitPitchAngle(void)
{
	// Caution: Angle[Pitch] is positive down which is the opposite sense to the pitch angle
	RawAngle[Pitch] += Rate[Pitch];
	RawAngle[Pitch] = Limit1(RawAngle[Pitch], RollIntLimit2048);

	Angle[Pitch] += Rate[Pitch];
	Angle[Pitch] = Limit1(Angle[Pitch], RollIntLimit2048);
	Angle[Pitch] = Decay1(Angle[Pitch]); 
	Angle[Pitch] += IntCorr[FB];
} // LimitPitchAngle

void LimitYawAngle(void)
{ 	// Yaw gyro compensation using compass
	static int16 Temp, HE;

	if ( F.CompassValid )
	{
		// + CCW
		Temp = DesiredYaw - Trim[Yaw];
		if ( Abs(Temp) > COMPASS_MIDDLE ) // acquire new heading
			DesiredHeading = Heading;
		else
		{
			HE = MinimumTurn(DesiredHeading - Heading);
			HE = Limit1(HE, SIXTHMILLIPI); // 30 deg limit
			HE = SRS32((int32)HE * (int32)P[CompassKp], 12); 
			Rate[Yaw] -= Limit1(HE, COMPASS_MAXDEV); // yaw gyro drift compensation
		}
	}

	#ifdef NEW_YAW

	Angle[Yaw] = Heading;

	#else

	Angle[Yaw] += Rate[Yaw];
	Angle[Yaw] = Limit1(Angle[Yaw], YawIntLimit256);

	Angle[Yaw] = DecayX(Angle[Yaw], 2); 				// GKE added to kill gyro drift

	#endif // NEW_YAW

} // LimitYawAngle

void DoOrientationTransform(void)
{
	static i24u Temp;

	if ( F.UsingPolar )
	{
		OSO = OSin[PolarOrientation];
		OCO = OCos[PolarOrientation];
	}
	else
	{
		OSO = OSin[Orientation];
		OCO = OCos[Orientation];
	}

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
	
		if ( P[Acro] > 0) // due to Foliage 2009 and Alexinparis 2010
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
	static i24u Temp;
	static int16 RateE;

	CalculateGyroRates();
	CompensateRollPitchGyros();	

	DoOrientationTransform();

	#ifdef SIMULATE

	FakeDesiredRoll = ControlRoll;
	FakeDesiredPitch = ControlPitch;
	FakeDesiredYaw =  DesiredYaw + NavCorr[Yaw];
	Angle[Roll] = SlewLimit(Angle[Roll], -FakeDesiredRoll * 16, 4);
	Angle[Pitch] = SlewLimit(Angle[Pitch], -FakeDesiredPitch * 16, 4);
	Angle[Yaw] = SlewLimit(Angle[Yaw], FakeDesiredYaw, 4);
	Rl = -FakeDesiredRoll;
	Pl = -FakeDesiredPitch;
	Yl = DesiredYaw;

	CameraRollAngle = Angle[Roll];
	CameraPitchAngle = Angle[Pitch];
	 
    #else

	GainSchedule();

	// Roll
				
	LimitRollAngle();

	#ifdef ML_PID

// set P = -32, I = -16, D = 0 as it is rescaled by /32

	RollOuterInp = - ( Angle[Roll] + ControlRoll ); //+ NavCorr[Roll] );
	RollOuter = SRS16(RollOuterInp * P[RollKi] + ( RollOuterInp - RollOuterInpP ) * P[RollKd], 5);

	RollInnerInp = RollOuter - Rate[Roll];
	Rl = SRS16(RollInnerInp * P[RollKp], 5);

	//Rl = SRS32((int32)Rl * GS, 8);

	RollOuterInpP = RollOuterInp;

	#else

	Rl  = SRS16(Rate[Roll] * P[RollKp] - (Rate[Roll] - Ratep[Roll]) * P[RollKd], 5);
	Rl += SRS16(Angle[Roll] * (int16)P[RollKi], 9); 

	Rl = SRS32((int32)Rl * GS, 8);

	Rl -= ControlRoll + NavCorr[Roll];

	Ratep[Roll] = Rate[Roll];

	#endif // ML_PID

	// Pitch

	LimitPitchAngle();

	#ifdef ML_PID

	PitchOuterInp = -( Angle[Pitch] + ControlPitch ); //+ NavCorr[Pitch] );
	PitchOuter = SRS16(PitchOuterInp * P[PitchKi] + ( PitchOuterInp - PitchOuterInpP ) * P[PitchKd], 5);

	PitchInnerInp = PitchOuter - Rate[Pitch];
	Pl = -SRS16(PitchInnerInp * P[PitchKp], 5);

	//Pl = SRS32((int32)Pl * GS, 8);

	RollOuterInpP = RollOuterInp;

	#else

	Pl  = SRS16(Rate[Pitch] * P[PitchKp] - (Rate[Pitch] - Ratep[Pitch]) * P[PitchKd], 5);
	Pl += SRS16(Angle[Pitch] * (int16)P[PitchKi], 9);

	Pl = -SRS32((int32)Pl * GS, 8);

	Pl -= ControlPitch + NavCorr[Pitch];

	Ratep[Pitch] = Rate[Pitch];

	#endif // ML_PID

	// Yaw  - should be tuned to rate control
	
	LimitYawAngle();

	#ifdef NEW_YAW

	RateE = Rate[Yaw] + DesiredYaw + NavCorr[Yaw];

	YawRateIntE += RateE;
	YawRateIntE = Limit1(YawRateIntE, YawIntLimit256);

	Yl  = SRS16( RateE * (int16)P[YawKp] + SRS16( YawRateIntE * (int16)P[YawKi], 4), 4);

	#else

	Yl  = SRS16( ( Rate[Yaw] + DesiredYaw + NavCorr[Yaw] ) * (int16)P[YawKp] + (Ratep[Yaw]-Rate[Yaw]) * (int16)P[YawKd], 4);
	Yl += SRS16(Angle[Yaw] * (int16)P[YawKi], 8);

	#endif // NEW_YAW

	Ratep[Yaw] = Rate[Yaw];

	#ifdef TRICOPTER
		Yl = SlewLimit(Ylp, Yl, 2);
		Ylp = Yl;
		Yl = Limit1(Yl,(int16)P[YawLimit]*4);
	#else
		Yl = Limit1(Yl, (int16)P[YawLimit]);
	#endif // TRICOPTER

	CameraRollAngle = SRS32(Angle[Pitch] * OSO + Angle[Roll] * OCO, 8);
	CameraPitchAngle = SRS32(Angle[Pitch] * OCO - Angle[Roll] * OSO, 8);

	#endif // SIMULATE	

	F.NearLevel = Max(Abs(Angle[Roll]), Abs(Angle[Pitch])) < NAV_RTH_LOCKOUT;

} // DoControl

static int8 RCStart = RC_INIT_FRAMES;

void LightsAndSirens(void)
{
	static int24 Ch5Timeout;

	LEDYellow_TOG;
	if ( F.Signal ) LEDGreen_ON; else LEDGreen_OFF;

	Beeper_OFF;
	Ch5Timeout = mSClock() + 500; // mS.
	do
	{
		ProcessCommand();
		if( F.Signal )
		{
			LEDGreen_ON;
			if( F.RCNewValues )
			{
				UpdateControls();
				if ( --RCStart == 0 ) // wait until RC filters etc. have settled
				{
					UpdateParamSetChoice();
					MixAndLimitCam();
					RCStart = 1;
				}
				GetBaroAltitude();
				InitialThrottle = StickThrottle;
				StickThrottle = DesiredThrottle = 0; 
				OutSignals(); // synced to New RC signals
				if( mSClock() > Ch5Timeout )
				{
					if ( F.Ch5Active || !F.ParametersValid )
					{
						Beeper_TOG;					
						LEDRed_TOG;
					}
					else
						if ( Armed )
							LEDRed_TOG;
							
					Ch5Timeout += 500;
				}	
			}
		}
		else
		{
			LEDRed_ON;
			LEDGreen_OFF;
		}	
		ReadParametersEE();	
	}
	while( ( P[TxRxType] == UnknownTxRx ) ||  (!F.Signal) || (Armed && FirstPass) || F.Ch5Active || F.GyroFailure || 
		( InitialThrottle >= RC_THRES_START ) || (!F.ParametersValid)  );
				
	FirstPass = false;

	Beeper_OFF;
	LEDRed_OFF;
	LEDGreen_ON;
	LEDYellow_ON;

	mS[LastBattery] = mSClock();
	mS[FailsafeTimeout] = mSClock() + FAILSAFE_TIMEOUT_MS;
	PIDUpdate = mSClock() + PID_CYCLE_MS;
	F.LostModel = false;
	FailState = MonitoringRx;

} // LightsAndSirens

void InitControl(void)
{
	static uint8 g;

	for ( g = 0; g <(uint8)3; g++ )
		Rate[g] = Ratep[g] = Trim[g] = 0;

	CameraRollAngle = CameraPitchAngle = RollOuterInpP = PitchOuterInpP = 0;
	ControlRollP = ControlPitchP = 0;

	Ylp = 0;
	AltComp = YawRateF.i32 = 0;	
	AltSum = 0;
} // InitControl



