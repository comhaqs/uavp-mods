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
void DoPolarOrientation(void);
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

int16 NavCorr[3], NavCorrp[3], NavIntE[3];
int32 NavPosE[2], NavPosEp[2], NavVelp[2];
int32 NavScale[2];
int16 NavSlewLimit;
boolean NavNotSaturated[2];

boolean StartingNav = true;
int16 DescentComp;

#ifdef SIMULATE
int16 FakeDesiredRoll, FakeDesiredPitch, FakeDesiredYaw, FakeMagHeading;
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

int16 	NavPolarRadius, NavProximityRadius, NavNeutralRadius, NavProximityAltitude;
uint24 	NavRTHTimeoutmS;

int16 	NavSensitivity, RollPitchMax;

void DoShutdown(void)
{
	State = Shutdown;
	StopMotors();
} // DoShutdown

void DecayNavCorr(uint8 d)
{
		NavVelp[Roll] = NavVelp[Pitch] = 0;
		NavCorr[Roll] = NavCorrp[Roll] = DecayX(NavCorr[Roll], d);
		NavCorr[Pitch] = NavCorrp[Pitch] = DecayX(NavCorr[Pitch], d);

		NavIntE[Roll] = NavIntE[Pitch] = 0;

		NavCorr[Yaw] = 0;
		
		StartingNav = true;
} // DecayNavCorr

void FailsafeHoldPosition(void)
{
	DesiredRoll = DesiredPitch = DesiredYaw = 0;
	if ( F.GPSValid && F.CompassValid && F.AccelerationsValid ) 
		Navigate(HoldLatitude, HoldLongitude);
} // FailsafeHoldPosition

void SetDesiredAltitude(int24 NewDesiredAltitude) // cm
{
	ROCIntE = 0;
	DesiredAltitude = NewDesiredAltitude;
} // SetDesiredAltitude

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
		if ( mSClock() > mS[DescentUpdate] )
		{
			mS[DescentUpdate] = mSClock() + ALT_DESCENT_UPDATE_MS;
			DesiredThrottle = CruiseThrottle - DescentComp;
			if ( DesiredThrottle < IdleThrottle )
				StopMotors();
			else
				if ( DescentComp < CruiseThrottle )
					DescentComp++;		
		}
	}

} // DoFailsafeLanding

void AcquireHoldPosition(void)
{
	NavCorr[Pitch] = NavCorrp[Pitch] = NavCorr[Roll] = NavCorrp[Roll] = NavCorr[Yaw] =  0;
	F.NavComputed = false;

	HoldLatitude = GPSLatitude;
	HoldLongitude = GPSLongitude;
	F.WayPointAchieved = F.WayPointCentred = F.AcquireNewPosition = false;

	NavState = HoldingStation;
} // AcquireHoldPosition

void DoPolarOrientation(void)
{
	static int32 EastDiff, NorthDiff, Radius;
	static int16 DesiredRelativeHeading;
	static int16 P;
	static i32u Temp32;

	#ifndef TESTING
	F.UsingPolar = F.UsingPolarCoordinates && F.NavValid && ( NavState == HoldingStation );
	
	if ( F.UsingPolar ) // needs rethink - probably arm using RTH switch
	{
		Temp32.i32 = (OriginLongitude - GPSLongitude) * GPSLongitudeCorrection;
		EastDiff = Temp32.i3_1;
		NorthDiff = OriginLatitude - GPSLatitude;
	
		Radius = Max(Abs(EastDiff), Abs(NorthDiff));
		if ( Radius > NavPolarRadius )
		{ 
			DesiredRelativeHeading = Make2Pi(int32atan2((int32)EastDiff, (int32)NorthDiff) - MILLIPI - Heading );
		
			P = ( (int24)DesiredRelativeHeading * 24L + HALFMILLIPI )/ MILLIPI + Orientation;
		
			while ( P > 24 ) P -=24;
			while ( P < 0 ) P +=24;	
		}
		else
			P = 0; 
	}
	else
	#endif // !TESTING
		P = 0;

	PolarOrientation = P;
	
} // DoPolarOrientation

void DoCompScaling(void)
{
	if ( Abs(NavPosE[Roll]) > Abs(NavPosE[Pitch]) ) // expensive but gives straight path.
	{
		NavScale[Pitch] = (NavPosE[Pitch] * 256) / NavPosE[Roll];
		NavScale[Pitch] = Abs(NavScale[Pitch]);
		NavScale[Roll] = 256;
	}
	else
	{
		NavScale[Roll] = (NavPosE[Roll] * 256) / NavPosE[Pitch];
		NavScale[Roll] = Abs(NavScale[Roll]);
		NavScale[Pitch] = 256;
	}		
} // DoCompScaling

void Navigate(int32 NavLatitude, int32 NavLongitude )
{	// F.GPSValid must be true immediately prior to entry	
	// This routine does not point the quadrocopter at the destination
	// waypoint. It simply rolls/pitches towards the destination
	// cos/sin/arctan lookup tables are used for speed.
	// BEWARE magic numbers for integer arithmetic

	static uint8 a;

	#ifndef TESTING // not used for testing - make space!

	static int16 NavVel, NavAttitudeLimit;
	static int16 RelHeading;
    static int24 AltE;
	static int32 MaxDiff, Diff, LongitudeDiff, LatitudeDiff;
	static int32 SinHeading, CosHeading, NavP, NavI, NavD;
	static i32u Temp32;
	static i24u Temp24;
	static int16 NewCorr;

	DoPolarOrientation();

	#ifdef DEBUG_NAV
		NavLatitude = 0; // zzz
		NavLongitude = 0;
		GPSLatitude = 54L * 50;
		GPSLongitude = 0;
		Heading = 0;
		NavSensitivity = 100;
	#endif // DEBUG_NAV

	Temp24.i24 = (int24)NavSensitivity * NAV_MAX_ROLL_PITCH;
	NavAttitudeLimit = Temp24.i2_1; //  divide RC_MAXIMUM
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
		SinHeading = int16sin(Heading);
		CosHeading = int16cos(Heading);
		
		Temp32.i32 = -LatitudeDiff * SinHeading + LongitudeDiff * CosHeading;
		NavPosE[Roll] = Temp32.i3_1;
	
		Temp32.i32 = -LatitudeDiff * CosHeading - LongitudeDiff * SinHeading;
		NavPosE[Pitch] = Temp32.i3_1; 

		#ifdef NAV_WING
		
			NavCorr[Pitch] = 0; 
			// Just use simple rudder only for now.
			if ( !F.WayPointAchieved )
			{
				RelHeading = MakePi(WayHeading - Heading); // make +/- MilliPi
				NavCorr[Yaw] = -SRS16(RelHeading, 4); // ~1deg
				NavCorr[Yaw] = Limit1(NavCorr[Yaw], (int16)P[NavYawLimit]); // gently!
				//zzz Roll as well at 25%
			}

		#else // MULTICOPTER

			// revert to original simpler version from UAVP->UAVX transition
			
			// Roll & Pitch

			if ( MaxDiff > NAV_CLOSING_RADIUS )
				DoCompScaling();
				
			if ( StartingNav )
			{
				for ( a = 0; a < (uint8)2 ; a++ )
				{ 
					NavIntE[a] = 0;
					NavPosEp[a] = NavPosE[a];
				}
				StartingNav = false;
			}

			for ( a = 0; a < (uint8)2 ; a++ )
			{
				if ( MaxDiff > NAV_CLOSING_RADIUS )
				{
					Temp32.i32 = Sign(NavPosE[a]) * NavAttitudeLimit * NavScale[a];
					NavP = Temp32.i3_1;
				}
				else
				{
					NavP = Limit1(NavPosE[a], NAV_CLOSING_RADIUS) * NavAttitudeLimit;  
					NavP = SRS32(NavP, NAV_CLOSING_RADIUS_SHIFT); // shift is the effective closing radius in GPS units
				}

				#ifdef USE_BACK_INT_LIMIT

					NavI = NavIntE[a];
					if ( NavNotSaturated[a] )
					{	
						NavI += SRS32(NavP * (int16)P[NavKi], 4);
						NavI =  Limit1(NavI,(int16)P[NavIntLimit]);
						NavIntE[a] = NavI;
					}
					else
						DecayX(NavIntE[a],1);

				#else

					NavI = NavIntE[a];
					NavI += SRS32(NavP * (int16)P[NavKi], 4);
					NavI =  Limit1(NavI,(int16)P[NavIntLimit]);
					NavIntE[a] = NavI;

				#endif // USE_BACK_INT_LIMIT
				
				Diff = NavPosE[a] - NavPosEp[a];
				NavVel = MediumFilter((int32)NavVelp[a], Diff);
				NavD = SRS32(NavVel * (int16)P[NavKd], 6);
				NavD = Limit1(NavD, NavAttitudeLimit);

				NewCorr = NavP + NavI - NavD;

			  	NewCorr = SlewLimit(NavCorrp[a], NewCorr, NavSlewLimit);
			  	NavCorr[a] = Limit1(NewCorr, NavAttitudeLimit);
				NavNotSaturated[a] = NavCorr[a] == NewCorr;				

			  	NavCorrp[a] = NavCorr[a];
				NavPosEp[a] = NavPosE[a];
				NavVelp[a] = NavVel;
			}
	
			// Yaw
			if ( F.AllowTurnToWP && !F.WayPointAchieved )
			{
				RelHeading = MakePi(WayHeading - Heading); // make +/- MilliPi
				NavCorr[Yaw] = -SRS16(RelHeading, 4); // ~1deg
				NavCorr[Yaw] = SlewLimit(NavCorrp[Yaw], NavCorr[Yaw], NavSlewLimit);
				NavCorr[Yaw] = Limit1(NavCorr[Yaw], (int16)P[NavYawLimit]); // gently!
			}
			else
				NavCorr[Yaw] = 0;

		#endif // NAV_WING
	}	
	else
    #endif // !TESTING
		DecayNavCorr(4);

	F.NavComputed = true;

} // Navigate


void DoNavigation(void)
{
	#ifndef TESTING // not used for testing - make space!

	F.NavigationActive = F.GPSValid && F.CompassValid && F.AccelerationsValid && ( mSClock() > mS[NavActiveTime]);

	if ( F.NavigationActive )
	{
		F.LostModel = F.ForceFailsafe = false;

		if ( !F.NavComputed )
			switch ( NavState ) { // most case last - switches in C18 are IF chains not branch tables!
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
				break;
			} // switch NavState
	}
	#ifdef ENABLE_STICK_CHANGE_FAILSAFE
	else 
    	if ( F.ForceFailsafe && F.NewCommands  && ( StickThrottle >= IdleThrottle ) )
		{
			F.AltHoldEnabled = F.AllowNavAltitudeHold = true;
			F.LostModel = true;
			DoFailsafeLanding();
		}
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
				NavState = Descending;
				FailState = Terminating;
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
		DesiredRoll = DesiredPitch = DesiredYaw = DesiredThrottle = 0;

	#endif // !TESTING
			
} // DoFailsafe

void UAVXNavCommand(void)
{ 	// NavPlan adapted from ArduPilot ConfigTool GUI - quadrocopter must be disarmed

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
} // UAVXNavCommand

void GetWayPointEE(int8 wp)
{ 
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

	F.WayPointCentred =  F.WayPointAchieved = false;

} // GetWaypointEE


void InitNavigation(void)
{
	static uint8 wp, a;

	HoldLatitude = HoldLongitude = WayHeading = 0;
	for ( a = 0; a < (uint8)2 ; a++ )
	{
		NavNotSaturated[a] = true;			
		NavCorr[a] = NavCorrp[a] = NavPosEp[a] = NavIntE[a] = NavVelp[a] = 0;
	}
	NavCorr[Yaw] = NavCorrp[Yaw] = 0;

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

	NavPolarRadius = ConvertMToGPS(NAV_POLAR_RADIUS);

} // InitNavigation

