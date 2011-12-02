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

// Autonomous flight routines

#include "uavx.h"

void DoShutdown(void);
void DecayNavCorr(uint8);
void DoCompScaling(void);
void Navigate(int32, int32);
void SetDesiredAltitude(int24);
void DoFailsafeLanding(void);
void AcquireHoldPosition(void);
void NavGainSchedule(int16);
void DoNavigation(void);
void DoFailsafe(void);
void UAVXNavCommand(void);
void GetWayPointEE(int8);
void InitNavigation(void);

#pragma udata nav_vars
int32 NavScale[2];
int16 NavSlewLimit;
boolean NavNotSaturated[2];
#pragma udata

boolean StartingNav = true;
int16 DescentComp;

#ifdef SIMULATE
int16 FakeMagHeading;
#endif // SIMULATE

#pragma udata uavxnav_waypoints
typedef union { 
	uint8 b[256];
	struct {
		uint8 u0;
		int16 u1;
		int8 u3;
		int8 u4;
		uint16 u5;
		uint16 u6;
		uint8 NoOfWPs;
		uint8 ProximityAltitude;
		uint8 ProximityRadius;
		int16 OriginAltitude;
		int32 OriginLatitude;
		int32 OriginLongitude;
		int16 RTHAltitude;
		struct {
			int32 Latitude;
			int32 Longitude;
			int16 Altitude;
			int8 Loiter;
			} WP[23];
		};
	} UAVXNav;
#pragma udata

#pragma udata eebuffer
uint8 BufferEE[256];
#pragma udata
		
int8 	CurrWP;
int8 	NoOfWayPoints;
int16	WPAltitude;
int32 	WPLatitude, WPLongitude;
int24 	WPLoiter;
int16	WayHeading;

int16 	NavProximityRadius, NavNeutralRadius, NavProximityAltitude;
uint24 	NavRTHTimeoutmS;

int16 	NavSensitivity, RollPitchMax;

void DoShutdown(void)
{
	State = Shutdown;
	StopMotors();
} // DoShutdown

void DecayNavCorr(uint8 d)
{
	A[Roll].NavCorr = DecayX(A[Roll].NavCorr, d);
	A[Pitch].NavCorr = DecayX(A[Pitch].NavCorr, d);

	A[Roll].NavIntE = A[Pitch].NavIntE = 0;

	A[Yaw].NavCorr = 0;
		
	StartingNav = true;
} // DecayNavCorr

void SetDesiredAltitude(int24 NewDesiredAltitude) // cm
{
	ROCIntE = 0;
	DesiredAltitude = NewDesiredAltitude;
} // SetDesiredAltitude

#ifndef TESTING

void FailsafeHoldPosition(void)
{
	A[Roll].Desired = A[Pitch].Desired = A[Yaw].Desired = 0;
	if ( F.GPSValid && F.CompassValid && F.AccelerationsValid ) 
		Navigate(HoldLatitude, HoldLongitude);
} // FailsafeHoldPosition

void DoFailsafeLanding(void)
{ // InTheAir micro switch RC0 Pin 11 to ground when landed

	SetDesiredAltitude(-200);
	if ( F.BaroAltitudeValid )
	{
		if ( Altitude > LAND_CM )
			mS[NavStateTimeout] = mSClock() + NAV_RTH_LAND_TIMEOUT_MS;

		if ( !InTheAir || (( mSClock() > mS[NavStateTimeout] ) 
			&& ( F.ForceFailsafe || ( NavState == Touchdown ) || (FailState == Terminated))) )
			DoShutdown();
		else
			DesiredThrottle = CruiseThrottle;
	}
	else
	{
		DesiredThrottle = CruiseThrottle - DescentComp;
		if ( mSClock() > mS[DescentUpdate] )
		{
			mS[DescentUpdate] = mSClock() + ALT_DESCENT_UPDATE_MS;
			if ( DesiredThrottle < IdleThrottle )
				DoShutdown();
			else
				DescentComp += ALT_DESCENT_STEP;		
		}
	}

} // DoFailsafeLanding

void DoForcedFailsafe(void)
{
	F.AltHoldEnabled = F.AllowNavAltitudeHold = true;
	F.LostModel = true;
	DoFailsafeLanding();
} // DoForcedFailsafe

void AcquireHoldPosition(void)
{
	A[Pitch].NavCorr = A[Roll].NavCorr = A[Yaw].NavCorr =  0;
	F.NavComputed = false;

	HoldLatitude = GPSLatitude;
	HoldLongitude = GPSLongitude;
	F.WayPointAchieved = F.WayPointCentred = F.AcquireNewPosition = false;

	NavState = HoldingStation;
} // AcquireHoldPosition


void DoCompScaling(void)
{
	if ( Abs(A[Roll].NavPosE) > Abs(A[Pitch].NavPosE) ) // expensive but gives straight path.
	{
		NavScale[Pitch] = (A[Pitch].NavPosE * 256) / A[Roll].NavPosE;
		NavScale[Pitch] = Abs(NavScale[Pitch]);
		NavScale[Roll] = 256;
	}
	else
	{
		NavScale[Roll] = (A[Roll].NavPosE * 256) / A[Pitch].NavPosE;
		NavScale[Roll] = Abs(NavScale[Roll]);
		NavScale[Pitch] = 256;
	}		
} // DoCompScaling

#endif // !TESTING

void Navigate(int32 NavLatitude, int32 NavLongitude )
{	// 1.9mS @ 16MHz
	// 0.8mS @ 40MHz

	// F.GPSValid must be true immediately prior to entry	
	// This routine does not point the quadrocopter at the destination
	// waypoint. It simply rolls/pitches towards the destination
	// cos/sin/arctan lookup tables are used for speed.
	// BEWARE magic numbers for integer arithmetic

	static uint8 a;

	#ifndef TESTING // not used for testing - make space!

	static int16 NavVel, NavAttitudeLimit, EffNavSensitivity;
	static int16 RelHeading;
    static int24 AltE;
	static int32 MaxDiff, Diff, LongitudeDiff, LatitudeDiff;
	static int32 SinHeading, CosHeading, NavP, NavI, NavD;
	static i32u Temp32;
	static i24u Temp24;
	static int16 NewCorr;
	static int32 NavPosEp[2];
	static AxisStruct *C;

	SpareSlotTime = false;
	
	EffNavSensitivity = NavSensitivity - NAV_SENS_THRESHOLD;
	
	Temp24.i24 = (int24)EffNavSensitivity * NAV_MAX_ROLL_PITCH;
	NavAttitudeLimit = Temp24.i2_1; //  ~ divide by RC_MAXIMUM
	NavAttitudeLimit = Limit(NavAttitudeLimit, 0, NAV_MAX_ROLL_PITCH);
	
	DesiredLatitude = NavLatitude; // for telemetry tracking
	DesiredLongitude = NavLongitude;
	
	Temp32.i32 = (DesiredLongitude - GPSLongitude) * GPSLongitudeCorrection;
	LongitudeDiff = Temp32.i3_1;
	LatitudeDiff = DesiredLatitude - GPSLatitude;
	
	WayHeading = Make2Pi(int32atan2((int32)LongitudeDiff, (int32)LatitudeDiff));
	
	MaxDiff = Max(Abs(LongitudeDiff), Abs(LatitudeDiff));
	if ( MaxDiff < NavProximityRadius )
	{
		AltE = DesiredAltitude - Altitude;
		F.WayPointAchieved =  Abs(AltE) < NavProximityAltitude;
		F.WayPointCentred = MaxDiff < NavNeutralRadius;
	}
	else
		F.WayPointAchieved = F.WayPointCentred = false;
	
	if ( F.AttitudeHold && ( NavSensitivity > NAV_SENS_THRESHOLD ) && !F.WayPointCentred )
	{
			if ( StartingNav )
			{
				A[Roll].NavIntE = NavPosEp[Roll] = 0;
				A[Pitch].NavIntE = NavPosEp[Pitch] = 0;
			}
			else
			{
				NavPosEp[Roll] = A[Roll].NavPosE;
				NavPosEp[Pitch] = A[Pitch].NavPosE;
			}

		StartingNav = false;

		SinHeading = int16sin(Heading);
		CosHeading = int16cos(Heading);

		Temp32.i32 = -LatitudeDiff * SinHeading + LongitudeDiff * CosHeading;
		A[Roll].NavPosE = Temp32.i3_1;
		
		Temp32.i32 = -LatitudeDiff * CosHeading - LongitudeDiff * SinHeading;
		A[Pitch].NavPosE = Temp32.i3_1; 
	
		#ifdef NAV_WING
			
			A[Pitch].NavCorr = 0; 
			// Just use simple rudder only for now.
			if ( !F.WayPointAchieved )
			{
				RelHeading = MakePi(WayHeading - Heading); // make +/- MilliPi
				A[Yaw].NavCorr = -SRS16(RelHeading, 4); // ~1deg
				A[Yaw].NavCorr = Limit1(A[Yaw].NavCorr, (int16)P[NavYawLimit]); // gently!
				//zzz Roll as well at 25%
			}
	
		#else // MULTICOPTER
	
			// revert to original simpler version from UAVP->UAVX transition
				
			// Roll & Pitch
	
			if ( MaxDiff > NAV_CLOSING_RADIUS )
				DoCompScaling();
	
			for ( a = Roll; a<=(uint8)Pitch ; a++ )
			{
				C = &A[a];
				if ( MaxDiff > NAV_CLOSING_RADIUS )
				{
					Temp32.i32 = Sign(C->NavPosE) * NavAttitudeLimit * NavScale[a];
					NavP = Temp32.i3_1;
				}
				else
				{
					NavP = Limit1(C->NavPosE, NAV_CLOSING_RADIUS) * NavAttitudeLimit;  
					NavP = SRS32(NavP, NAV_CLOSING_RADIUS_SHIFT); // shift is the effective closing radius in GPS units
				}
	
				#ifdef USE_BACK_INT_LIMIT
	
					if ( NavNotSaturated[a] )
					{
						C->NavIntE += NavP;
						C->NavIntE = Limit1(C->NavIntE,(int16)P[NavIntLimit]);	
					}
					else
						DecayX(C->NavIntE,1);
	
				#else

					C->NavIntE += NavP;
					C->NavIntE = Limit1(C->NavIntE,(int16)P[NavIntLimit]);	
	
				#endif // USE_BACK_INT_LIMIT

				NavI = SRS32(C->NavIntE * (int16)P[NavKi], 4);

				// If we are doing a sustained rapid yaw the velocity will not make sense		
				Diff = C->NavPosE - NavPosEp[a];
				C->NavVel = MediumFilter((int32)C->NavVel, Diff);
				NavD = SRS32(C->NavVel * (int16)P[NavKd], 6);
				NavD = Limit1(NavD, NavAttitudeLimit);
	
				NewCorr = NavP + NavI - NavD;
	
				NewCorr = SlewLimit(C->NavCorr, NewCorr, NavSlewLimit);
				C->NavCorr = Limit1(NewCorr, NavAttitudeLimit);
				NavNotSaturated[a] = C->NavCorr == NewCorr;					
			}
		
			// Yaw
			if ( F.AllowTurnToWP && !F.WayPointAchieved )
			{
				RelHeading = MakePi(WayHeading - Heading); // make +/- MilliPi
				NewCorr = -SRS16(RelHeading, 4); // ~1deg
				NewCorr = SlewLimit(A[Yaw].NavCorr, NewCorr, NavSlewLimit);
				A[Yaw].NavCorr = Limit1(NewCorr, (int16)P[NavYawLimit]); // gently!
			}
			else
				A[Yaw].NavCorr = 0;
	
		#endif // NAV_WING
	}	
	else
	#endif // !TESTING
		DecayNavCorr(6);

	F.NavComputed = true;

} // Navigate


void DoNavigation(void)
{
	#ifndef TESTING // not used for testing - make space!

	F.NavigationActive = F.GPSValid && F.CompassValid && F.AccelerationsValid && ( mSClock() > mS[NavActiveTime]);

	if ( F.NavigationActive )
	{
		F.LostModel = F.ForceFailsafe = false;

		if (( !F.NavComputed ) && SpareSlotTime )
			switch ( NavState ) { // most frequent case last - switches in C18 are IF chains not branch tables!
			case Touchdown:
				Navigate(OriginLatitude, OriginLongitude);
				DoFailsafeLanding();
				break;
			case Descending:
				Navigate( OriginLatitude, OriginLongitude );
				if ( F.ReturnHome || F.Navigate )
				#ifdef NAV_WING
				{
					// needs more thought - runway direction?
					DoFailsafeLanding();
				}
				#else
				{
					if ( Altitude < LAND_CM )
						NavState = Touchdown;
					DoFailsafeLanding();
				}
				#endif // NAV_WING
				else
					AcquireHoldPosition();
				break;
			case AtHome:
				Navigate(OriginLatitude, OriginLongitude);

				if ( F.ReturnHome || F.Navigate )
					if ( F.WayPointAchieved ) // check still @ Home
					{
						if ( F.UsingRTHAutoDescend && ( mSClock() > mS[NavStateTimeout] ) )
						{
							mS[NavStateTimeout] = mSClock() + NAV_RTH_LAND_TIMEOUT_MS;
							NavState = Descending;
						}
					}
					else
						NavState = ReturningHome;
				else
					AcquireHoldPosition();
				break;
			case ReturningHome:		
				Navigate(OriginLatitude, OriginLongitude);				
				if ( F.ReturnHome || F.Navigate ) 
				{
					if ( F.WayPointAchieved )
					{
						mS[NavStateTimeout] = mSClock() + NavRTHTimeoutmS;
						SetDesiredAltitude((int16)P[NavRTHAlt]*100);			
						NavState = AtHome;
					}	
				}
				else
					AcquireHoldPosition();					
				break;
			case Loitering:
				Navigate(WPLatitude, WPLongitude);
				if ( F.Navigate )
				{
					if ( F.WayPointAchieved && (mSClock() > mS[NavStateTimeout]) )
						if ( CurrWP == NoOfWayPoints )
						{
							CurrWP = 1;
							SetDesiredAltitude((int16)P[NavRTHAlt]*100);
							NavState = ReturningHome;	
						}
						else
						{
							GetWayPointEE(++CurrWP); 
							SetDesiredAltitude(WPAltitude*100);	
							NavState = Navigating;
						}
				}
				else
					AcquireHoldPosition();
				break;
			case Navigating:
				Navigate(WPLatitude, WPLongitude);	
				if ( F.Navigate )
				{
					if ( F.WayPointAchieved )
					{
						mS[NavStateTimeout] = mSClock() + WPLoiter;
						SetDesiredAltitude(WPAltitude*100);
						NavState = Loitering;
					}		
				}
				else
					AcquireHoldPosition();					
				break;
			case HoldingStation:
				if ( F.AttitudeHold )
				{		
					if ( F.AcquireNewPosition && !( F.Ch5Active & F.UsingPositionHoldLock ) )
					{
						F.AllowTurnToWP = SaveAllowTurnToWP;
						AcquireHoldPosition();
						#ifdef NAV_ACQUIRE_BEEPER
						if ( !F.BeeperInUse )
						{
							mS[BeeperTimeout] = mSClock() + 500L;
							Beeper_ON;				
						} 
						#endif // NAV_ACQUIRE_BEEPER
					}	
				}
				else
					F.AcquireNewPosition = true;
					
				Navigate(HoldLatitude, HoldLongitude);
			
				if ( F.NavValid ) // zzz && F.NearLevel )  // Origin must be valid for ANY navigation!
				{
					if ( F.Navigate )
					{
						GetWayPointEE(CurrWP); // resume from previous WP
						SetDesiredAltitude(WPAltitude*100);
						NavState = Navigating;
					}
					else
						if ( F.ReturnHome )
						{
							SetDesiredAltitude((int16)P[NavRTHAlt]*100);
							NavState = ReturningHome;
						}
				}
				else
					if ( F. ReturnHome )
					{ // force a false origin
						OriginLatitude = HoldLatitude;
						OriginLongitude = HoldLongitude;
						SetDesiredAltitude((int16)P[NavRTHAlt]*100);
						NavState = AtHome;
					}										
				break;
			} // switch NavState
	}
	#ifdef ENABLE_STICK_CHANGE_FAILSAFE
	else 
    	if ( F.ForceFailsafe ) //&& F.NewCommands  && ( StickThrottle >= IdleThrottle ) )
			DoForcedFailsafe();
	#endif // ENABLE_STICK_CHANGE_FAILSAFE
		else 
			DecayNavCorr(6);

	F.NewCommands = false;	// Navigate modifies Desired Roll, Pitch and Yaw values.

	#endif // !TESTING
} // DoNavigation

void CheckFailsafeAbort(void)
{

	#ifndef TESTING

	if ( mSClock() > mS[AbortTimeout] )
	{
		if ( F.Signal )
		{
			LEDGreen_ON;
			mS[NavStateTimeout] = 0;
			mS[FailsafeTimeout] = mSClock() + FAILSAFE_TIMEOUT_MS; // may be redundant?
			NavState = HoldingStation;
			FailState = MonitoringRx;
		}
	}
	else
		mS[AbortTimeout] += ABORT_UPDATE_MS;

	#endif // !TESTING
} // CheckFailsafeAbort

void DoFailsafe(void)
{ // only relevant to PPM Rx or Quad NOT synchronising with Rx and sticks unchanged

	#ifndef TESTING // not used for testing - make space!

	if ( State == InFlight )
		switch ( FailState ) { // FailStates { MonitoringRx, Aborting, Terminating, Terminated, RxTerminate }
		case Terminated: // Basic assumption is that aircraft is being flown over a safe area!
			FailsafeHoldPosition();
			DoFailsafeLanding();	
			break;
		case Terminating:
			FailsafeHoldPosition();
			if ( Altitude < LAND_CM )
				FailState = Terminated;
			DoFailsafeLanding();
			break;
		case Aborting:
			FailsafeHoldPosition();
			F.AltHoldEnabled = true;
			SetDesiredAltitude((int16)P[NavRTHAlt]*100);
			if( mSClock() > mS[NavStateTimeout] )
			{
				F.LostModel = true;
				LEDGreen_OFF;
				LEDRed_ON;
	
				mS[NavStateTimeout] = mSClock() + NAV_RTH_LAND_TIMEOUT_MS;
				if ( DesiredThrottle > IdleThrottle )
				{
					DescentComp = CruiseThrottle - DesiredThrottle;
					if ( DescentComp < 1 ) DescentComp = 1;
					NavState = Descending;
					FailState = Terminating;
				}
				else
				{
					NavState = Descending;
					FailState = Terminated;
					DoShutdown();
				}
			}
			else
				CheckFailsafeAbort();		
			break;
		case MonitoringRx:
			if ( mSClock() > mS[FailsafeTimeout] ) 
			{
				// use last "good" throttle
				Stats[RCFailsafesS]++;
				if ( F.GPSValid && F.CompassValid && F.AccelerationsValid )
					mS[NavStateTimeout] = mSClock() + ABORT_TIMEOUT_GPS_MS;
				else
					mS[NavStateTimeout] = mSClock() + ABORT_TIMEOUT_NO_GPS_MS;
				mS[AbortTimeout] = mSClock() + ABORT_UPDATE_MS;
				FailState = Aborting; 
			}
			break;
		case RxTerminate: break; // invoked if Rx controls are not changing
		default:;
		} // Switch FailState
	else
		A[Roll].Desired = A[Pitch].Desired = A[Yaw].Desired = DesiredThrottle = 0;

	#endif // !TESTING
			
} // DoFailsafe

void UAVXNavCommand(void)
{ 	// NavPlan adapted from ArduPilot ConfigTool GUI - quadrocopter must be disarmed

	#ifndef TESTING

	static int16 b;
	static uint8 c, d, csum;

	c = RxChar();
	LEDBlue_ON;

	switch ( c ) {
	case '0': // hello
		TxChar(ACK);
		break;
	case '1': // write
		csum = 0;
		for ( b = 0; b < 256; b++) // cannot write fast enough so buffer
		{
			d = RxChar();
			csum ^= d;
			BufferEE[b] = d;
		}
		if ( csum == (uint8)0 )
		{
			for ( b = 0; b < 256; b++)
				WriteEE(NAV_ADDR_EE + b, BufferEE[b]);
			TxChar(ACK);
		}
		else
			TxChar(NAK);

		InitNavigation();
	
		break;
	case '2':
		csum = 0;
		for ( b = 0; b < 255; b++)
		{	
			d = ReadEE(NAV_ADDR_EE + b);
			csum ^= d;
			BufferEE[b] = d;
		}
		BufferEE[255] = csum;
		for ( b = 0; b < 256; b++)
			TxChar(BufferEE[b]);
		TxChar(ACK);
		break;
	case '3':
		csum = 0;
		for ( b = 0; b < 63; b++)
		{	
			d = ReadEE(STATS_ADDR_EE + b);
			csum ^= d;
			BufferEE[b] = d;
		}
		BufferEE[63] = csum;
		for ( b = 0; b < 64; b++)
			TxChar(BufferEE[b]);
		TxChar(ACK);
		break;
	default:
		break;
	} // switch

	LEDBlue_OFF;

	#endif // !TESTING

} // UAVXNavCommand

void GetWayPointEE(int8 wp)
{ 
	#ifndef TESTING

	static uint16 w;
	
	if ( wp > NoOfWayPoints )
		CurrWP = wp = 0;
	if ( wp == 0 ) 
	{  // force to Origin
		WPLatitude = OriginLatitude;
		WPLongitude = OriginLongitude;
		WPAltitude = (int16)P[NavRTHAlt];
		WPLoiter = 30000; // mS
	}
	else
	{	
		w = NAV_WP_START + (wp-1) * WAYPOINT_REC_SIZE;
		WPLatitude = Read32EE(w + 0);
		WPLongitude = Read32EE(w + 4);			
		WPAltitude = Read16EE(w + 8);

		#ifdef NAV_ENFORCE_ALTITUDE_CEILING
		if ( WPAltitude > NAV_CEILING )
			WPAltitude = NAV_CEILING;
		#endif // NAV_ENFORCE_ALTITUDE_CEILING
		WPLoiter = (int16)ReadEE(w + 10) * 1000L; // mS
	}

	#endif // !TESTING

	F.WayPointCentred =  F.WayPointAchieved = false;

} // GetWaypointEE

void InitNavigation(void)
{
	static uint8 wp, a;

	HoldLatitude = HoldLongitude = WayHeading = 0;
	for ( a = Roll; a<=(uint8)Pitch; a++ )
	{
		NavNotSaturated[a] = true;			
		A[a].NavCorr = A[a].NavIntE = 0;
	}
	A[Yaw].NavCorr = 0;

	NavState = HoldingStation;
	AttitudeHoldResetCount = 0;
	CurrMaxRollPitch = 0;
	F.WayPointAchieved = F.WayPointCentred = false;
	F.NavComputed = false;

//	if ( ReadEE(NAV_NO_WP) <= 0 )
	{
		NavProximityRadius = ConvertMToGPS(NAV_PROXIMITY_RADIUS); 
		NavProximityAltitude = NAV_PROXIMITY_ALTITUDE * 10L; // Decimetres
	}
/* zzz
	else
	{
		// need minimum values in UAVXNav?
		NavProximityRadius = ConvertMToGPS(ReadEE(NAV_PROX_RADIUS));
		NavProximityAltitude = ReadEE(NAV_PROX_ALT) * 10L; // Decimetres
	}
*/

	NoOfWayPoints = ReadEE(NAV_NO_WP);
	if ( NoOfWayPoints <= 0 )
		CurrWP = 0;
	else
		CurrWP = 1;
	GetWayPointEE(0);

} // InitNavigation

