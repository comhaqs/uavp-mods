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

void DoRxPolarity(void);
void InitRC(void);
void MapRC(void);
void CheckSticksHaveChanged(void);
void UpdateControls(void);
void CaptureTrims(void);
void CheckThrottleMoved(void);

int8 Map[CONTROLS], RMap[CONTROLS];
boolean PPMPosPolarity;

int16 RC[CONTROLS], RCp[CONTROLS];
int16 CruiseThrottle, NewCruiseThrottle, MaxCruiseThrottle, DesiredThrottle, IdleThrottle, InitialThrottle, StickThrottle;
int16 DesiredCamPitchTrim;
int16 ThrLow, ThrHigh, ThrNeutral;

void DoRxPolarity(void)
{
	if ( F.UsingSerialPPM  ) // serial PPM frame from within an Rx
		CCP1CONbits.CCP1M0 = PPMPosPolarity;
	else
		CCP1CONbits.CCP1M0 = true;	
}  // DoRxPolarity

void InitRC(void)
{
	static int8 c, q;

	DoRxPolarity();

 	mS[StickChangeUpdate] = mSClock();
	mS[RxFailsafeTimeout] = mSClock() + RC_NO_CHANGE_TIMEOUT_MS;
	F.ForceFailsafe = false;

	SignalCount = -RC_GOOD_BUCKET_MAX;
	F.Signal = F.RCNewValues = false;
	
	for (c = 0; c < CONTROLS; c++)
	{
		PPM[c].i16 = 0;
		RC[c] = RCp[c] = 0;
		Map[c] = RMap[c] = c;
	}
	RC[ThrottleRC] = RCp[ThrottleRC] = 0; 
	A[Roll].Desired = A[Pitch].Desired = A[Yaw].Desired = DesiredThrottle = StickThrottle = 0;
	A[Roll].Trim = A[Pitch].Trim = A[Yaw].Trim = 0; 
	PPM_Index = PrevEdge = RCGlitches = 0;
} // InitRC

void MapRC(void) // re-arrange arithmetic reduces from 736uS to 207uS @ 40MHz
{  // re-maps captured PPM to Rx channel sequence
	static int8 c, i;
	static int16 LastThrottle, Temp;
	static i32u Temp2;

	LastThrottle = RC[ThrottleRC];

	for (c = 0 ; c < NoOfControls ; c++) 
	{
		i = Map[c];
		#ifdef CLOCK_16MHZ
			Temp = PPM[i].b0; // clip to bottom byte 0..255
		#else // CLOCK_40MHZ
			//Temp = ((int32)PPM[i].i16 * RC_MAXIMUM + 625L)/1250L; // scale to 4uS res. for now
			Temp2.i32 = ((int32)PPM[i].i16 * (RC_MAXIMUM*53L)  );
			Temp = Temp2.iw1;
		#endif // CLOCK_16MHZ

		RC[c] = RxFilter(RC[c], Temp);			
	}

	if ( THROTTLE_SLEW_LIMIT > 0 )
		RC[ThrottleRC] = SlewLimit(LastThrottle, RC[ThrottleRC], THROTTLE_SLEW_LIMIT);

} // MapRC

void CheckSticksHaveChanged(void)
{
	#ifndef TESTING

	static boolean Change;
	static uint8 c;

	if ( F.FailsafesEnabled )
	{
		if ( F.ReturnHome || F.Navigate  )
		{
			Change = true;
			mS[RxFailsafeTimeout] = mSClock() + RC_NO_CHANGE_TIMEOUT_MS;			
			F.ForceFailsafe = false;
		}
		else
		{
			if ( mSClock() > mS[StickChangeUpdate] )
			{
				mS[StickChangeUpdate] = mSClock() + 500;
		
				Change = false;
				for ( c = ThrottleC; c <= (uint8)RTHRC; c++ )
				{
					Change |= Abs( RC[c] - RCp[c]) > RC_STICK_MOVEMENT;
					RCp[c] = RC[c];
				}
			}
		
			if ( Change )
			{
				mS[RxFailsafeTimeout] = mSClock() + RC_NO_CHANGE_TIMEOUT_MS;
				mS[NavStateTimeout] = mSClock();
				F.ForceFailsafe = false;
	
				if ( FailState == MonitoringRx )
				{
					if ( F.LostModel )
					{
						Beeper_OFF;
						F.LostModel = false;
						DescentComp = 0;
					}
				}
			}
			else
				if ( mSClock() > mS[RxFailsafeTimeout] )
				{
					if ( !F.ForceFailsafe && ( State == InFlight ) )
					{
						//Stats[RCFailsafesS]++;
						mS[NavStateTimeout] = mSClock() + NAV_RTH_LAND_TIMEOUT_MS;
						mS[DescentUpdate]  = mSClock() + ALT_DESCENT_UPDATE_MS;
						DescentComp = 0; // for no Baro case
						F.ForceFailsafe = true;
					}
				}
		}
	}
	else
		F.ForceFailsafe = false;

	#else

	F.ForceFailsafe = false;

	#endif // ENABLE_STICK_CHANGE_FAILSAFE

} // CheckSticksHaveChanged

void UpdateControls(void)
{
	static boolean NewCh5Active;

	F.RCNewValues = false;

	MapRC();								// remap channel order for specific Tx/Rx

	StickThrottle = RC[ThrottleRC];

	//_________________________________________________________________________________________

	// Navigation

	F.ReturnHome = F.Navigate = false;
	NewCh5Active = RC[RTHRC] > RC_NEUTRAL;

	if ( F.UsingPositionHoldLock )
		if ( NewCh5Active && !F.Ch5Active )
			F.AllowTurnToWP = true;
		else
			F.AllowTurnToWP = SaveAllowTurnToWP;
	else
		if ( RC[RTHRC] > ((2L*RC_MAXIMUM)/3L) )
			#ifdef FORCE_NAV
				F.Navigate = true;
			#else
				F.ReturnHome = true;
			#endif // FORCE_NAV
		else
			if ( RC[RTHRC] > (RC_NEUTRAL/3L) )
				F.Navigate = true;
	
	F.Ch5Active = NewCh5Active;

	if ( NoOfControls < 7 )
	{
		DesiredCamPitchTrim = RC_NEUTRAL;
		// NavSensitivity set in ReadParametersEE
		F.AccelerometersEnabled = true;
	}
	else
	{
		DesiredCamPitchTrim = RC[CamPitchRC] - RC_NEUTRAL;
		NavSensitivity = RC[NavGainRC];
		NavSensitivity = Limit(NavSensitivity, 0, RC_MAXIMUM);
	
		if ( F.NormalFlightMode )
		{
			F.AccelerometersEnabled = true;
			F.AltHoldEnabled = NavSensitivity > NAV_SENS_ALTHOLD_THRESHOLD;
		}
		else
		{
			F.AccelerometersEnabled = NavSensitivity > (RC_MAXIMUM/4);
			F.AltHoldEnabled = NavSensitivity > RC_NEUTRAL;
		}
	}

	//_________________________________________________________________________________________

	// Altitude Hold


	if ( NavState == HoldingStation )
	{ // Manual
		if ( StickThrottle < RC_THRES_STOP )	// to deal with usual non-zero EPA
			StickThrottle = 0;
	}
	else // Autonomous
		if ( F.AllowNavAltitudeHold &&  F.AltHoldEnabled )
			StickThrottle = CruiseThrottle;

	if ( (! F.HoldingAlt) && (!(F.Navigate || F.ReturnHome )) ) // cancel any current altitude hold setting 
		SetDesiredAltitude(Altitude);	

	//_________________________________________________________________________________________
			
	// Attitude
		
	A[Roll].Desired = RC[RollRC] - RC_NEUTRAL;
	A[Pitch].Desired = RC[PitchRC] - RC_NEUTRAL;		
	A[Yaw].Desired = RC[YawRC] - RC_NEUTRAL;

						
	A[Roll].Hold = A[Roll].Desired - A[Roll].Trim;
	A[Roll].Hold = Abs(A[Roll].Hold);
	A[Pitch].Hold = A[Pitch].Desired - A[Pitch].Trim;
	A[Pitch].Hold = Abs(A[Pitch].Hold);
	A[Yaw].Hold = A[Yaw].Desired - A[Yaw].Trim;
	A[Yaw].Hold = Abs(A[Yaw].Hold);
	CurrMaxRollPitch = Max(A[Roll].Hold, A[Pitch].Hold);
		
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

	//_________________________________________________________________________________________
			
	// Rx has gone to failsafe

	CheckSticksHaveChanged();
	
	F.NewCommands = true;

} // UpdateControls

void CaptureTrims(void)
{ 	// only used in detecting movement from neutral in hold GPS position
	// Trims are invalidated if Nav sensitivity is changed - Answer do not use trims ?
	#ifndef TESTING
	A[Roll].Trim = Limit1(A[Roll].Desired, NAV_MAX_TRIM);
	A[Pitch].Trim = Limit1(A[Pitch].Desired, NAV_MAX_TRIM);
	A[Yaw].Trim = Limit1(A[Yaw].Desired, NAV_MAX_TRIM);
	#endif // TESTING

	HoldYaw = 0;
} // CaptureTrims

void CheckThrottleMoved(void)
{
	if( mSClock() < mS[ThrottleUpdate] )
		ThrNeutral = DesiredThrottle;
	else
	{
		ThrLow = ThrNeutral - THROTTLE_MIDDLE;
		ThrLow = Max(ThrLow, THROTTLE_MIN_ALT_HOLD);
		ThrHigh = ThrNeutral + THROTTLE_MIDDLE;
		if ( ( DesiredThrottle <= ThrLow ) || ( DesiredThrottle >= ThrHigh ) )
		{
			mS[ThrottleUpdate] = mSClock() + THROTTLE_UPDATE_MS;
			F.ThrottleMoving = true;
		}
		else
			F.ThrottleMoving = false;
	}
} // CheckThrottleMoved




