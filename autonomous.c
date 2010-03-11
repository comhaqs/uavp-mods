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
//    If not, see http://www.gnu.org/licenses/.

// Autonomous flight routines

#include "uavx.h"

void Navigate(int24, int24);
void SetDesiredAltitude(int16);
void DoFailsafeLanding(void);
void AcquireHoldPosition(void);
void NavGainSchedule(int16);
void DoNavigation(void);
void DoPPMFailsafe(void);
void LoadNavBlockEE(void);
void GetWayPointEE(uint8);
void InitNavigation(void);

int16 NavRCorr, NavRCorrP, NavPCorr, NavPCorrP, NavYCorr, SumNavYCorr;
int16 EffNavSensitivity;
int16 EastP, EastDiffSum, EastI, EastCorr, NorthP, NorthDiffSum, NorthI, NorthCorr;
int24 EastD, EastDiffP, NorthD, NorthDiffP;

#pragma udata ardupilot_waypoints
typedef union { // Ardupilot uses 512 bytes - truncated to 256 for UAVX
	uint8 b[256];
	struct {
		uint8 Config;
		int16 AirspeedOffset;
		int8 RollTrim;
		int8 PitchTrim;
		uint16 MaxAltitude;
		uint16 MaxAirspeed;
		uint8 NoOfWPs;
		uint8 CurrWP;
		uint8 Radius;
		int16 OriginAltitude;
		int32 OriginLatitude;
		int32 OriginLongitude;
		int16 OriginAltHoldAlt;
		struct {
			int32 Latitude;
			int32 Longitude;
			int16 Altitude;
			} WP[23];
		};
	} ArduPilotNav;
#pragma udata
		
uint8 	CurrWP;
int8 	NoOfWayPoints;
int24 	WPNorth, WPEast;
int16	WPAltitude;
uint8 	WPLoiter;

int16 	NavClosingRadius, NavNeutralRadius, NavCloseToNeutralRadius, NavProximityRadius, NavProximityAltitude, CompassOffset, NavRTHTimeoutmS;

uint8 	NavState;
int16 	NavSensitivity, RollPitchMax;
int16 	AltSum;
int32	NavRTHTimeout;

void SetDesiredAltitude(int16 NewDesiredAltitude) // Metres
{
	if ( F.NavAltitudeHold )
	{
		DesiredThrottle = HoverThrottle;
		DesiredAltitude = NewDesiredAltitude * 100L;
	}
	else
	{
		// manual control of altitude
	}
} // SetDesiredAltitude

void DoFailsafeLanding(void)
{ // InTheAir micro switch RC0 Pin 11 to ground when landed

	DesiredAltitude = -200;
	if ( !InTheAir || (( mS[Clock] > mS[LandingTimeout]) && ( NavState == Touchdown )) )
	{
		DesiredThrottle = 0;
		StopMotors();
	}
	else
		DesiredThrottle = HoverThrottle;
} // DoFailsafeLanding

void AcquireHoldPosition(void)
{
	NavPCorr = NavPCorrP = NavRCorr = NavRCorrP = NavYCorr =  0;
	F.NavComputed = false;

	GPSNorthHold = GPSNorth;
	GPSEastHold = GPSEast;
	F.Proximity = F.CloseProximity = F.AcquireNewPosition = false;

	NavState = HoldingStation;
} // AcquireHoldPosition

void Navigate(int24 GPSNorthWay, int24 GPSEastWay )
{	// F.GPSValid must be true immediately prior to entry	
	// This routine does not point the quadrocopter at the destination
	// waypoint. It simply rolls/pitches towards the destination
	// cos/sin/arctan lookup tables are used for speed.
	// BEWARE magic numbers for integer arithmetic

	static int16 SinHeading, CosHeading;
	static int24 Temp, EastDiff, NorthDiff;
	static int16 WayHeading, RelHeading;

	F.NewCommands = false;	// Navigate modifies Desired Roll, Pitch and Yaw values.

	if ( !F.NavComputed ) 
	{
		EastDiff = GPSEastWay - GPSEast;
		NorthDiff = GPSNorthWay - GPSNorth;

		F.CloseProximity = (Abs(EastDiff)<NavNeutralRadius )&&(Abs(NorthDiff) < NavNeutralRadius);
		//	EffNavSensitivity = (NavSensitivity * ( ATTITUDE_HOLD_LIMIT * 4 - 
		// 							CurrMaxRollPitch )) / (ATTITUDE_HOLD_LIMIT * 4);
		EffNavSensitivity = SRS16(NavSensitivity * ( 32 - Limit(CurrMaxRollPitch, 0, 32) ) + 16, 5);

		if ( ( EffNavSensitivity > NAV_GAIN_THRESHOLD ) && !F.CloseProximity )
		{	// direct solution make North and East coordinate errors zero
			WayHeading = int32atan2((int32)EastDiff, (int32)NorthDiff);
	
			SinHeading = int16sin(Heading);
			CosHeading = int16cos(Heading);

			F.Proximity = (Max(Abs(NorthDiff), Abs(EastDiff)) < NavProximityRadius) && ( Abs(DesiredAltitude - Altitude) < NavProximityAltitude );
	
			// East
			if ( Abs(EastDiff) < NavClosingRadius )
			{
				Temp = ( EastDiff * NAV_MAX_ROLL_PITCH )/ NavCloseToNeutralRadius;
				EastP = Limit(Temp, -NAV_MAX_ROLL_PITCH, NAV_MAX_ROLL_PITCH);
	
				EastDiffSum += EastDiff;
				EastDiffSum = Limit(EastDiffSum, -NAV_INT_WINDUP_LIMIT, NAV_INT_WINDUP_LIMIT);
			}
			else
			{
				EastP = Limit(EastDiff, -NAV_MAX_ROLL_PITCH, NAV_MAX_ROLL_PITCH);
				EastDiffSum = Limit(EastDiff, -NAV_INT_WINDUP_LIMIT, NAV_INT_WINDUP_LIMIT);
			}
	
			EastI = SRS16((int32)EastDiffSum * (int16)P[NavKi], 6); 
			EastI = Limit(EastI, (int16)(-P[NavIntLimit]), (int16)P[NavIntLimit]);
			EastDiffSum = Decay1(EastDiffSum);
	
			EastD = SRS32((int32)(EastDiffP - EastDiff) * (int16)P[NavKd], 8);
			EastDiffP = EastDiff;
			EastD = Limit(EastD, -NAV_DIFF_LIMIT, NAV_DIFF_LIMIT);
		
			EastCorr = SRS16((EastP + EastI + EastD) * EffNavSensitivity, 8);
	
			// North
			if ( Abs(NorthDiff) < NavClosingRadius )
			{
				Temp = ( NorthDiff * NAV_MAX_ROLL_PITCH )/ NavCloseToNeutralRadius;
				NorthP = Limit(Temp, -NAV_MAX_ROLL_PITCH, NAV_MAX_ROLL_PITCH);
	
				NorthDiffSum += NorthDiff;
				NorthDiffSum = Limit(NorthDiffSum, -NAV_INT_WINDUP_LIMIT, NAV_INT_WINDUP_LIMIT);
			}
			else
			{
				NorthP = Limit(NorthDiff, -NAV_MAX_ROLL_PITCH, NAV_MAX_ROLL_PITCH);
				NorthDiffSum = Limit(NorthDiff, -NAV_INT_WINDUP_LIMIT, NAV_INT_WINDUP_LIMIT);
			}

			NorthI = SRS16((int32)NorthDiffSum * (int16)P[NavKi], 6);
			NorthI = Limit(NorthI, (int16)(-P[NavIntLimit]), (int16)P[NavIntLimit]);
			NorthDiffSum = Decay1(NorthDiffSum);
	
			NorthD = SRS32((int32)(NorthDiffP - NorthDiff) * (int16)P[NavKd], 8); 
			NorthDiffP = NorthDiff;
			NorthD = Limit(NorthD, -NAV_DIFF_LIMIT, NAV_DIFF_LIMIT);
	
			NorthCorr = SRS16((NorthP + NorthI + NorthD) * EffNavSensitivity, 8); 
				
			// Roll & Pitch
			NavRCorr = SRS16(CosHeading * EastCorr - SinHeading * NorthCorr, 8);
			NavRCorr = Limit(NavRCorr, -NAV_MAX_ROLL_PITCH, NAV_MAX_ROLL_PITCH);	
			NavRCorr = SlewLimit(NavRCorrP, NavRCorr, NAV_CORR_SLEW_LIMIT);
			NavRCorrP = NavRCorr;
	
			NavPCorr = SRS16(-SinHeading * EastCorr - CosHeading * NorthCorr, 8);
			NavPCorr = Limit(NavPCorr, -NAV_MAX_ROLL_PITCH, NAV_MAX_ROLL_PITCH);
			NavPCorr = SlewLimit(NavPCorrP, NavPCorr, NAV_CORR_SLEW_LIMIT);
			NavPCorrP = NavPCorr;
				
			// Yaw
			if ( F.TurnToWP && !F.Proximity )
			{
				RelHeading = MakePi(WayHeading - Heading); // make +/- MilliPi
				NavYCorr = -(RelHeading * NAV_YAW_LIMIT) / HALFMILLIPI;
				NavYCorr = Limit(NavYCorr, -NAV_YAW_LIMIT, NAV_YAW_LIMIT); // gently!
			}
			else
				NavYCorr = 0;	
		}	
		else 
		{
			// Neutral Zone - no GPS influence
			NavPCorr = DecayX(NavPCorr, 2);
			NavRCorr = DecayX(NavRCorr, 2);
			NavYCorr = 0;
			EastDiffP = NorthDiffP = EastDiffSum = NorthDiffSum = 0;
		}
	}

	DesiredRoll = DesiredRoll + NavRCorr;
	DesiredPitch = DesiredPitch + NavPCorr;
	DesiredYaw += NavYCorr;
	F.NavComputed = true;

} // Navigate

void FakeFlight()
{
	#ifdef FAKE_FLIGHT

	static int16 CosH, SinH, A;

	#define FAKE_NORTH_WIND 	20L
	#define FAKE_EAST_WIND 		0L
    #define SCALE_VEL			256L

	if ( Armed )
	{
		Heading = MILLIPI;
	
		GPSEast = 5000L; GPSNorth = 5000L;
		InitNavigation();

		TxString(" Sens MRP Eff East North Head DRoll DPitch DYaw ");
		TxString("EP EI ES ED EC | NP NI NS ND NC | RC PC | P CP\r\n");
	
		while ( Armed )
		{
			UpdateControls();

			DesiredRoll = DesiredPitch = DesiredYaw = 0;
			Heading += DesiredYaw / 5;

			F.GPSValid = F.NavValid = F.CompassValid = true;
			F.NavComputed = false;
			if (( NavSensitivity > NAV_GAIN_THRESHOLD ) && F.NewCommands )
			{
				Navigate(0,0);

				Delay100mSWithOutput(5);
				CosH = int16cos(Heading);
				SinH = int16sin(Heading);
				GPSEast += ((int32)(-DesiredPitch) * SinH * 10L) / SCALE_VEL;
				GPSNorth += ((int32)(-DesiredPitch) * CosH * 10L) / SCALE_VEL;
				
				A = Make2Pi(Heading + HALFMILLIPI);
				CosH = int16cos(A);
				SinH = int16sin(A);
				GPSEast += ((int32)DesiredRoll * SinH * 10L) / SCALE_VEL;
				GPSEast += FAKE_EAST_WIND; // wind	
				GPSNorth += ((int32)DesiredRoll * CosH * 10L) / SCALE_VEL;
				GPSNorth += FAKE_NORTH_WIND; // wind
			
				TxVal32((int32)NavSensitivity,0,' ');
				TxVal32((int32)CurrMaxRollPitch,0,' ');
				TxVal32((int32)EffNavSensitivity,0,' ');
				TxVal32(GPSEast, 0, ' '); TxVal32(GPSNorth, 0, ' '); TxVal32(Heading, 0, ' '); 
				TxVal32((int32)DesiredRoll, 0, ' '); TxVal32((int32)DesiredPitch, 0, ' '); 
				TxVal32((int32)DesiredYaw, 0, ' ');
				TxVal32((int32)EastP, 0, ' '); TxVal32((int32)EastI, 0, ' '); 
				TxVal32((int32)EastDiffSum, 0, ' '); TxVal32((int32)EastD, 0, ' '); 
				TxVal32((int32)EastCorr, 0, ' ');
				TxString("| ");
				TxVal32((int32)NorthP, 0, ' '); TxVal32((int32)NorthI, 0, ' '); 
				TxVal32((int32)NorthDiffSum, 0, ' '); TxVal32((int32)NorthD, 0, ' ');
				TxVal32((int32)NorthCorr, 0, ' ');
				TxString("| ");
				TxVal32((int32)NavRCorr, 0, ' ');
				TxVal32((int32)NavPCorr, 0, ' ');
				TxString("| ");
				TxVal32((int32)F.Proximity, 0, ' ');
				TxVal32((int32)F.CloseProximity, 0, ' ');	
				TxNextLine();
			}
		}
	}
	#endif // FAKE_FLIGHT
} // FakeFlight

void DoNavigation(void)
{
	switch ( NavState ) { // most case last - switches in C18 are IF chains not branch tables!
	case Touchdown:
		Navigate(0, 0);
		if ( F.Navigate )
			DoFailsafeLanding();
		else
			AcquireHoldPosition();
		break;
	case Descending:
		Navigate(0, 0);
		if ( F.Navigate )
			if ( RelBaroAltitude < LAND_CM )
			{
				mS[LandingTimeout] = mS[Clock] + NAV_RTH_LAND_TIMEOUT_MS;
				NavState = Touchdown;
			}
			else
				DoFailsafeLanding();
		else
			AcquireHoldPosition();
		break;
	case AtHome:
		Navigate(0, 0); // Origin
		if ( F.Navigate || F.ReturnHome )
			if ( F.UsingRTHAutoDescend && ( mS[Clock] > mS[RTHTimeout] ) )
				NavState = Descending;
			else
				SetDesiredAltitude((int16)P[NavRTHAlt]);
		else
			AcquireHoldPosition();
		break;
	case Loitering:
		Navigate(WPNorth, WPEast);
		if ( F.Navigate || F.ReturnHome )
		{
			if ( mS[Clock] > mS[LoiterTimeout] )
			{	
				GetWayPointEE(++CurrWP);
				NavState = Navigating;
			}
		}
		else
			AcquireHoldPosition();
		break;
	case Navigating:	
		if ( F.Navigate || F.ReturnHome )
		{
			SetDesiredAltitude(WPAltitude); // at least hold altitude!
	 		if ( Max(Abs(RollSum), Abs(PitchSum)) < NAV_RTH_LOCKOUT ) // nearly level to engage!
			{
				Navigate(WPNorth, WPEast);
				if ( F.Proximity )
					if ( CurrWP == 0)
					{
						mS[RTHTimeout] = mS[Clock] + NavRTHTimeout;				
						NavState = AtHome;
					}
					else
					{
						mS[LoiterTimeout] = mS[Clock] + WPLoiter * 1000L;
						NavState = Loitering;
					}		
			}
		}
		else
			AcquireHoldPosition();					
		break;
	case HoldingStation:
		if ( F.AttitudeHold )
		{		
			if ( F.AcquireNewPosition )
			{
				AcquireHoldPosition();
				#ifdef NAV_ACQUIRE_BEEPER
				if ( !F.BeeperInUse )
				{
					mS[BeeperTimeout] = mS[Clock] + 500L;
					Beeper_ON;				
				} 
				#endif // NAV_ACQUIRE_BEEPER
			}	
		}
		else
			F.AcquireNewPosition = true;
			
		Navigate(GPSNorthHold, GPSEastHold);

		if ( F.ReturnHome )
		{
			AltSum = 0;
			GetWayPointEE(0);
			NavState = Navigating;
		}
		else
			if ( F.Navigate )
			{
				AltSum = 0;
				GetWayPointEE(1);
				NavState = Navigating;
			}		
		break;
	} // switch NavState

	DesiredRoll = Limit(DesiredRoll, -MAX_ROLL_PITCH, MAX_ROLL_PITCH);
	DesiredPitch = Limit(DesiredPitch, -MAX_ROLL_PITCH, MAX_ROLL_PITCH);

} // DoNavigation

void DoPPMFailsafe(void)
{ // only relevant to PPM Rx or Quad NOT synchronising with Rx
	if ( State == InFlight )
		switch ( FailState ) {
		case Terminated:
			DesiredRoll = DesiredPitch = DesiredYaw = 0;
			DoFailsafeLanding();
			if ( mS[Clock ] > mS[AbortTimeout] )
				if ( F.Signal )
				{
					LEDRed_OFF;
					LEDGreen_ON;
					FailState = Waiting;
				}
				else
					mS[AbortTimeout] += ABORT_UPDATE_MS;
			break;
		#ifdef NAV_PPM_FAILSAFE_RTH
		case Returning:
			DesiredRoll = DesiredPitch = DesiredYaw = DesiredThrottle = 0;
			F.NewCommands = true;
			if ( F.NavValid && F.GPSValid && F.CompassValid )
			{
				DoNavigation();	
				if ( mS[Clock ] > mS[AbortTimeout] )
					if ( F.Signal )
					{
						LEDRed_OFF;
						LEDGreen_ON;
						FailState = Waiting;
					}
					else
						mS[AbortTimeout] += ABORT_UPDATE_MS;
			}
			else
			{
				mS[LandingTimeout] = mS[Clock] + NAV_RTH_LAND_TIMEOUT_MS;
				FailState = Terminated;
			}
			break;
		#endif // PPM_FAILSAFE_RTH
		case Aborting:
			if( mS[Clock] > mS[AbortTimeout] )
			{
				F.LostModel = true;
				LEDGreen_OFF;
				LEDRed_ON;

				F.NavAltitudeHold = true;
				SetDesiredAltitude(WPAltitude);
				mS[AbortTimeout] += ABORT_TIMEOUT_MS;

				#ifdef NAV_PPM_FAILSAFE_RTH
					NavSensitivity = RC_NEUTRAL;	// 50% gain
					F.Navigate = F.TurnToWP = true;
					AltSum = 0;
					GetWayPointEE(0);
					NavState = Navigating;
					FailState = Returning;
				#else
					mS[LandingTimeout] = mS[Clock] + NAV_RTH_LAND_TIMEOUT_MS;
					FailState = Terminated;
				#endif // PPM_FAILSAFE_RTH
			}
			break;
		case Waiting:
			if ( mS[Clock] > mS[FailsafeTimeout] ) 
			{
				LEDRed_ON;
				mS[AbortTimeout] = mS[Clock] + ABORT_TIMEOUT_MS;
				DesiredRoll = DesiredPitch = DesiredYaw = 0;
				FailState = Aborting;
				// use last "good" throttle; 
			}
			break;
		} // Switch FailState
	else
		DesiredRoll = DesiredRollP = DesiredPitch = DesiredPitchP = DesiredYaw = DesiredThrottle = 0;			
} // DoPPMFailsafe

#pragma udata eebuffer
uint8 BufferEE[256];
#pragma udata

void LoadNavBlockEE(void)
{ 	// NavPlan adapted from ArduPilot ConfigTool GUI - quadrocopter must be disarmed

	static uint16 b;
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
		if ( csum == 0 )
		{
			for ( b = 0; b < 256; b++)
				WriteEE(NAV_ADDR_EE + b, BufferEE[b]);
			TxChar(ACK);
		}
		else
			TxChar(NAK);	
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
} // LoadNavBlockEE

void GetWayPointEE(uint8 wp)
{ 
	static uint16 w;
	
	CurrWP = wp;
	NoOfWayPoints = ReadEE(NAV_NO_WP);
	if ( wp > NoOfWayPoints ) 
	{  // force to Origin
		WPEast = WPNorth = 0;
		WPAltitude = (int16)P[NavRTHAlt];
		WPLoiter = 0;
		CurrWP = 0;
	}
	else
	{	
		w = NAV_WP_START + (wp-1) * WAYPOINT_REC_SIZE;
		WPEast = Read32EE(w) - GPSOriginLongitude;
		WPNorth = Read32EE(w + 4) - GPSOriginLatitude;
		WPAltitude = Read16EE(w + 8);
		WPLoiter = Read16EE(w + 10);
	}

} // GetWaypointEE

void InitNavigation(void)
{
	static uint8 wp;

	GPSNorthHold = GPSEastHold = 0;
	NavPCorr = NavPCorrP = NavRCorr = NavRCorrP = NavYCorr = 0;
	EastDiffP, NorthDiffP = 0;

	NavState = HoldingStation;
	AttitudeHoldResetCount = 0;
	CurrMaxRollPitch = 0;
	F.Proximity = F.CloseProximity = true;
	F.NavComputed = false;
	
	WPNorth = WPEast = 0; WPAltitude = (int16)P[NavRTHAlt]; WPLoiter = 0;
	CurrWP = 0;

} // InitNavigation

