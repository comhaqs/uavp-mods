// =================================================================================================
// =                                  UAVX Quadrocopter Controller                                 =
// =                             Copyright (c) 2008 by Prof. Greg Egan                             =
// =                   Original V3.15 Copyright (c) 2007 Ing. Wolfgang Mahringer                   =
// =                       http://code.google.com/p/uavp-mods/ http://uavp.ch                      =
// =================================================================================================

//    This is part of UAVX.

//    UAVX is free software: you can redistribute it and/or modify it under the terms of the GNU 
//    General Public License as published by the Free Software Foundation, either version 3 of the 
//    License, or (at your option) any later version.

//    UAVX is distributed in the hope that it will be useful,but WITHOUT ANY WARRANTY; without even 
//    the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
//    General Public License for more details.

//    You should have received a copy of the GNU General Public License along with this program.  
//    If not, see http://www.gnu.org/licenses/

#include "uavx.h"

void DoAltitudeHold(int24, int16);
void AltitudeHold(void);
void InertialDamping(void);
void LimitRollSum(void);
void LimitPitchSum(void);
void LimitYawSum(void);
void DoControl(void);

void UpdateControls(void);
void CaptureTrims(void);
void StopMotors(void);
void CheckThrottleMoved(void);
void LightsAndSirens(void);
void InitControl(void);

int16 RC[CONTROLS];

int16 RE, PE, YE, HE;					// gyro rate error	
int16 REp, PEp, YEp, HEp;				// previous error for derivative
int16 RollSum, PitchSum, YawSum;		// integral 	

int16 RollTrim, PitchTrim, YawTrim;
int16 HoldYaw;
int16 RollIntLimit256, PitchIntLimit256, YawIntLimit256;

int16 CruiseThrottle, DesiredThrottle, IdleThrottle, InitialThrottle;
int16 DesiredRoll, DesiredPitch, DesiredYaw, DesiredHeading, DesiredCamPitchTrim, Heading;
int16 DesiredRollP, DesiredPitchP;
int16 CurrMaxRollPitch;

int16 ThrLow, ThrHigh, ThrNeutral;

int16 AttitudeHoldResetCount;
int16 AltComp, AltDiffSum, AltDSum, DescentCmpS;
int24 DesiredAltitude, Altitude;
int16 ROC;

boolean	FirstPass;

void DoAltitudeHold(int24 Altitude, int16 ROC)
{ // Syncronised to baro intervals independant of active altitude source

	static int16 NewAltComp, LimBE, AltP, AltI, AltD;
	static int24 Temp, BE;

	if ( F.NewBaroValue && F.BaroAltitudeValid ) 
	{
		#ifdef ALT_SCRATCHY_BEEPER
		if ( !F.BeeperInUse ) Beeper_TOG;
		#endif

		F.NewBaroValue = false;
	
		BE = DesiredAltitude - Altitude;
		LimBE = Limit(BE, -ALT_BAND_CM, ALT_BAND_CM);

		AltP = SRS16(LimBE * (int16)P[AltKp], 8);
		AltP = Limit(AltP, ALT_LOW_THR_COMP, ALT_HIGH_THR_COMP);

		AltDiffSum += Sign(LimBE);
		AltDiffSum = Limit(AltDiffSum, -ALT_INT_WINDUP_LIMIT, ALT_INT_WINDUP_LIMIT);
		AltI = SRS16(AltDiffSum * (int16)P[AltKi], 4);
		AltI = Limit(AltDiffSum, -(int16)P[AltIntLimit], (int16)P[AltIntLimit]);
		 
		if ( ROC < DescentCmpS )
		{
			AltDSum+=2;
			AltDSum = Limit(AltDSum, 0, ALT_HIGH_THR_COMP); 
		}
		AltDSum = Decay1(AltDSum);
	
		NewAltComp = AltP + AltI + AltDSum;
		NewAltComp = Limit(NewAltComp, ALT_LOW_THR_COMP, ALT_HIGH_THR_COMP);	
		AltComp = SlewLimit(AltComp, NewAltComp, 2);
/*
if ( State == InFlight )
{
TxVal32(mS[Clock],3,' ');
TxVal32(BE,2,' ');
TxVal32(AltP,0,' ');
TxVal32(AltI,0,' ');
TxVal32(AltDiffSum,0,' ');
TxVal32(AltDSum,0,' ');
TxVal32(NewAltComp,0,' ');
TxVal32(AltComp,0,' ');
TxNextLine();
}
*/
		#ifdef ALT_SCRATCHY_BEEPER
		if ( !F.BeeperInUse ) Beeper_TOG;
		#endif
	}

} // DoAltitudeHold	

void AltitudeHold()
{
	// relies upon good cross calibration of baro and rangefinder!!!!!!

	if ( F.RangefinderAltitudeValid ) // some hysteresis
		if (( RangefinderAltitude < 500 ) && !F.UsingRangefinderAlt)
			F.UsingRangefinderAlt = true;
		else
			if (( RangefinderAltitude > 600 ) && F.UsingRangefinderAlt)
				F.UsingRangefinderAlt = false;	

	if ( F.UsingRangefinderAlt )
	{
		Altitude = RangefinderAltitude;
		ROC = RangefinderROC;
	}
	else
		if ( F.UsingGPSAlt && F.GPSValid )
		{
			Altitude = GPSRelAltitude;
			ROC = GPSROC;
		}
		else
		{
			Altitude = RelBaroAltitude;
			ROC = BaroROC;
		}

	if ( F.NavAltitudeHold && ( NavState != HoldingStation ) )
	{
		F.HoldingAlt = false;
		DoAltitudeHold(Altitude, ROC);
	}
	else // holding station
	{
		F.HoldingAlt = !F.ThrottleMoving; 
		
		if( F.HoldingAlt )
		{
			if ( Abs(ROC) < ALT_HOLD_MAX_ROC_CMPS )
				CruiseThrottle = HardFilter(CruiseThrottle, DesiredThrottle);
			DoAltitudeHold(Altitude, ROC);
		}
		else	
		{
			DesiredAltitude = Altitude;
			AltDSum = Decay1(AltDSum);
			AltComp = Decay1(AltComp);	
		}
	}
} // AltitudeHold

void InertialDamping(void)
{ // Uses accelerometer to damp disturbances while holding altitude

	static int16 Temp;

	// Down - Up
	// Empirical - acceleration changes at ~approx Sum/8 for small angles
	DUVel += DUAcc + SRS16( Abs(RollSum) + Abs(PitchSum), 3);		
	DUVel = Limit(DUVel , -16384, 16383); 			
	Temp = SRS32(SRS16(DUVel, 4) * (int32)P[VertDampKp], 13);
	if( Temp > DUComp ) 
		DUComp++;
	else
		if( Temp < DUComp )
			DUComp--;			
	DUComp = Limit(DUComp, DAMP_VERT_LIMIT_LOW, DAMP_VERT_LIMIT_HIGH); 
	DUVel = DecayX(DUVel, (int16)P[VertDampDecay]);

	// Lateral compensation only when holding altitude?	
	if ( F.HoldingAlt && F.AttitudeHold ) 
	{
 		if ( F.WayPointCentred )
		{
			// Left - Right
			LRVel += LRAcc;
			LRVel = Limit(LRVel , -16384, 16383);  	
			Temp = SRS32(SRS16(LRVel, 4) * (int32)P[HorizDampKp], 13);
			if( Temp > LRComp ) 
				LRComp++;
			else
				if( Temp < LRComp )
					LRComp--;
			LRComp = Limit(LRComp, -DAMP_HORIZ_LIMIT, DAMP_HORIZ_LIMIT);
			LRVel = DecayX(LRVel, (int16)P[HorizDampDecay]);
	
			// Front - Back
			FBVel += FBAcc;
			FBVel = Limit(FBVel , -16384, 16383);  
			Temp = SRS32(SRS16(FBVel, 4) * (int32)P[HorizDampKp], 13);
			if( Temp > FBComp ) 
				FBComp++;
			else
				if( Temp < FBComp )
					FBComp--;
			FBComp = Limit(FBComp, -DAMP_HORIZ_LIMIT, DAMP_HORIZ_LIMIT);
			FBVel = DecayX(FBVel, (int16)P[HorizDampDecay]);
		}
		else
		{
			LRVel = FBVel = 0;
			LRComp = Decay1(LRComp);
			FBComp = Decay1(FBComp);
		}
	}
	else
	{
		LRVel = FBVel = 0;

		LRComp = Decay1(LRComp);
		FBComp = Decay1(FBComp);
	}
} // InertialDamping	

void LimitRollSum(void)
{
	RollSum += SRS16(RollRate, 1);		// use 9 bit res. for I controller
	RollSum = Limit(RollSum, -RollIntLimit256, RollIntLimit256);
	#ifdef ATTITUDE_ENABLE_DECAY
	RollSum = Decay1(RollSum);			// damps to zero even if still rolled
	#endif // ATTITUDE_ENABLE_DECAY
	RollSum += LRIntCorr;				// last for accelerometer compensation
} // LimitRollSum

void LimitPitchSum(void)
{
	PitchSum += SRS16(PitchRate, 1);
	PitchSum = Limit(PitchSum, -PitchIntLimit256, PitchIntLimit256);
	#ifdef ATTITUDE_ENABLE_DECAY
	PitchSum = Decay1(PitchSum); 
	#endif // ATTITUDE_ENABLE_DECAY
	PitchSum += FBIntCorr;
} // LimitPitchSum

void LimitYawSum(void)
{
	static int16 Temp;

	if ( F.CompassValid )
	{
		// + CCW
		Temp = DesiredYaw - YawTrim;
		Temp = Abs(Temp);
		HoldYaw = HardFilter(HoldYaw, Temp);
		if ( HoldYaw > COMPASS_MIDDLE ) // acquire new heading
		{
			DesiredHeading = Heading;
			HE = HEp = 0; 
		}
		else
		{
			HE = MakePi(DesiredHeading - Heading);
			HE = Limit(HE, -(MILLIPI/6), MILLIPI/6); // 30 deg limit
			HE = SRS32( (int32)(HEp * 3 + HE) * (int32)P[CompassKp], 12);  
			YE -= Limit(HE, -COMPASS_MAXDEV, COMPASS_MAXDEV);
		}
	}

	YawSum += YE;
	YawSum = Limit(YawSum, -YawIntLimit256, YawIntLimit256);

	YawSum = DecayX(YawSum, 2); 				// GKE added to kill gyro drift

} // LimitYawSum

void DoControl(void)
{				
	CalcGyroRates();
	CompensateRollPitchGyros();	
	InertialDamping();		

	// Roll
				
	RE = SRS16(RollRate, 2); // discard 2 bits of resolution!
	LimitRollSum();
 
	Rl  = SRS16(RE *(int16)P[RollKp] + (REp-RE) * (int16)P[RollKd], 4);
	Rl += SRS16(RollSum * (int16)P[RollKi], 9); 
	Rl -= DesiredRoll + SRS16((DesiredRoll - DesiredRollP) * ATTITUDE_FF_DIFF, 4);
	Rl -= LRComp;
	DesiredRollP = DesiredRoll;

	// Pitch

	PE = SRS16(PitchRate, 2); // discard 2 bits of resolution!
	LimitPitchSum();

	Pl  = SRS16(PE *(int16)P[PitchKp] + (PEp-PE) * (int16)P[PitchKd], 4);
	Pl += SRS16(PitchSum * (int16)P[PitchKi], 9);
	Pl -= DesiredPitch + SRS16((DesiredPitch - DesiredPitchP) * ATTITUDE_FF_DIFF, 4);
	Pl -= FBComp;
	DesiredPitchP = DesiredPitch;

	// Yaw

	YE = YawRate;	
	YE += DesiredYaw;
	LimitYawSum();

	Yl  = SRS16(YE *(int16)P[YawKp] + (YEp-YE) * (int16)P[YawKd], 4);
	Yl += SRS16(YawSum * (int16)P[YawKi], 8);
	Yl = Limit(Yl, -(int16)P[YawLimit], (int16)P[YawLimit]);	// effective slew limit

	#ifdef DEBUG_SENSORS
	Trace[THE] = HE;
	Trace[TRollRate] = RollRate;
	Trace[TPitchRate] = PitchRate;
	Trace[TYE] = YE;
	Trace[TRollSum] = RollSum;
	Trace[TPitchSum] = PitchSum;
	Trace[TYawSum] = YawSum;
	#endif

	REp = RE;
	PEp = PE;
	YEp = YE;
	HEp = HE;

	RollRate = PitchRate = 0;

} // DoControl

void UpdateControls(void)
{
	static int16 HoldRoll, HoldPitch, RollPitchScale;

	if ( F.RCNewValues )
	{
		F.RCNewValues = false;

		MapRC();								// remap channel order for specific Tx/Rx

		DesiredThrottle = RC[ThrottleC];
		if ( DesiredThrottle < RC_THRES_STOP )	// to deal with usual non-zero EPA
			DesiredThrottle = 0;

		#ifdef RX6CH
			DesiredCamPitchTrim = RC_NEUTRAL;
			// NavSensitivity set in ReadParametersEE
		#else
			DesiredCamPitchTrim = RC[CamPitchC] - RC_NEUTRAL;
			NavSensitivity = RC[NavGainC];
			NavSensitivity = Limit(NavSensitivity, 0, RC_MAXIMUM);
		#endif // !RX6CH

		#ifdef ATTITUDE_NO_LIMITS
		RollPitchScale = 128L;
		#else
		RollPitchScale = MAX_ROLL_PITCH - (NavSensitivity >> 2);
		#endif // ATTITUDE_NO_LIMITS

		DesiredRoll = SRS16((RC[RollC] - RC_NEUTRAL) * RollPitchScale, 7);
		DesiredPitch = SRS16((RC[PitchC] - RC_NEUTRAL) * RollPitchScale, 7);

		DesiredYaw = RC[YawC] - RC_NEUTRAL;

		F.ReturnHome = F.Navigate = false;
		if ( RC[RTHC] > ((3L*RC_MAXIMUM)/4) )
			#ifdef DEBUG_FORCE_NAV
			F.Navigate = true;
			#else
			F.ReturnHome = true;
			#endif // DEBUG_FORCE_NAV
		else
			if ( RC[RTHC] > (RC_NEUTRAL/2) )
				F.Navigate = true;

		if ( (! F.HoldingAlt) && (! (F.Navigate || F.ReturnHome )) ) // cancel any current altitude hold setting 
			DesiredAltitude = Altitude;

		HoldRoll = DesiredRoll - RollTrim;
		HoldRoll = Abs(HoldRoll);
		HoldPitch = DesiredPitch - PitchTrim;
		HoldPitch = Abs(HoldPitch);
		CurrMaxRollPitch = Max(HoldRoll, HoldPitch);

		if ( CurrMaxRollPitch > ATTITUDE_HOLD_LIMIT )
			if ( AttitudeHoldResetCount > ATTITUDE_HOLD_RESET_INTERVAL )
				F.AttitudeHold = false;
			else
			{
				AttitudeHoldResetCount++;
				F.AttitudeHold = true;
			}
		else
		{
			F.AttitudeHold = true;	
			if ( AttitudeHoldResetCount > 1 )
				AttitudeHoldResetCount -= 2;		// Faster decay
		}

		F.NewCommands = true;
	}
} // UpdateControls

void CaptureTrims(void)
{ 	// only used in detecting movement from neutral in hold GPS position
	// Trims are invalidated if Nav sensitivity is changed - Answer do not use trims ?
	RollTrim = Limit(DesiredRoll, -NAV_MAX_TRIM, NAV_MAX_TRIM);
	PitchTrim = Limit(DesiredPitch, -NAV_MAX_TRIM, NAV_MAX_TRIM);
	YawTrim = Limit(DesiredYaw, -NAV_MAX_TRIM, NAV_MAX_TRIM);

	HoldYaw = 0;
} // CaptureTrims

void StopMotors(void)
{
	static uint8 m;

	#ifdef MULTICOPTER
	for (m = 0; m < NoOfPWMOutputs; m++)
		PWM[m] = ESCMin;
	#else
	PWM[ThrottleC] = ESCMin;
	PWM[AileronC], PWM[ElevatorC] = PWM[RudderC] = OUT_NEUTRAL;

	#endif // MULTICOPTER

	PWM[CamRollC] = PWM[CamPitchC] = OUT_NEUTRAL;

	F.MotorsArmed = false;
} // StopMotors

void CheckThrottleMoved(void)
{
	if( mS[Clock] < mS[ThrottleUpdate] )
		ThrNeutral = DesiredThrottle;
	else
	{
		ThrLow = ThrNeutral - THROTTLE_MIDDLE;
		ThrLow = Max(ThrLow, THROTTLE_MIN_ALT_HOLD);
		ThrHigh = ThrNeutral + THROTTLE_MIDDLE;
		if ( ( DesiredThrottle <= ThrLow ) || ( DesiredThrottle >= ThrHigh ) )
		{
			mS[ThrottleUpdate] = mS[Clock] + THROTTLE_UPDATE_MS;
			F.ThrottleMoving = true;
		}
		else
			F.ThrottleMoving = false;
	}
} // CheckThrottleMoved

void LightsAndSirens(void)
{
	static uint24 Timeout;

	LEDYellow_TOG;
	if ( F.Signal ) LEDGreen_ON; else LEDGreen_OFF;

	Beeper_OFF; 
	Timeout = mS[Clock] + 500L; 					// mS.
	do
	{
		ProcessCommand();
		if( F.Signal )
		{
			LEDGreen_ON;
			if( F.RCNewValues )
			{
				UpdateControls();
				UpdateParamSetChoice();
				InitialThrottle = DesiredThrottle;
				DesiredThrottle = 0; 
				OutSignals();
				if( mS[Clock] > Timeout )
				{
					if ( F.Navigate || F.ReturnHome )
					{
						Beeper_TOG;					
						LEDRed_TOG;
					}
					else
						if ( Armed )
							LEDRed_TOG;
						
					Timeout += 500;
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
	while( (!F.Signal) || (Armed && FirstPass) || F.Navigate || F.ReturnHome ||
				( InitialThrottle >= RC_THRES_START) );

	FirstPass = false;

	Beeper_OFF;
	LEDRed_OFF;
	LEDGreen_ON;

	F.LostModel = false;
	mS[FailsafeTimeout] = mS[Clock] + FAILSAFE_TIMEOUT_MS;
	mS[UpdateTimeout] = mS[Clock] + (uint24)P[TimeSlots];
	FailState = MonitoringRx;

} // LightsAndSirens

void InitControl(void)
{
	RollRate = PitchRate = 0;
	RollTrim = PitchTrim = YawTrim = 0;
	AltComp = 0;
	DUComp = DUVel = LRVel = LRComp = FBVel = FBComp = 0;	
	AltSum = 0;
} // InitControl



