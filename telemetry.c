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

void SendPacketHeader(void);
void SendPacketTrailer(void);
void CheckTelemetry(void);
void SendCycle(void);
void SendControl(void);
void SendMinPacket(void);
void SendFlightPacket(void);
void SendNavPacket(void);
void SendControlPacket(void);
void SendStatsPacket(void);
void SendParamPacket(uint8, uint8);
void SendParameters(uint8);
void SendArduStation(void);
void SendCustom(void);

uint8 UAVXCurrPacketTag;

void CheckTelemetry(void) 
{
	if ( ( mSClock() >= mS[TelemetryUpdate] ) && SpareSlotTime )
	{				
		switch ( P[TelemetryType] ) {
		case UAVXTelemetry:
			SpareSlotTime = false;
			mS[TelemetryUpdate] = mSClock() + UAVX_TEL_INTERVAL_MS;
			SendCycle(); 	
			break;
		case UAVXMinTelemetry:
			SpareSlotTime = false;
			mS[TelemetryUpdate] = mSClock() + UAVX_MIN_TEL_INTERVAL_MS; 
			SendMinPacket(); 	
			break;
		case UAVXControlTelemetry:
			SpareSlotTime = false;
			mS[TelemetryUpdate] = mSClock() + UAVX_CONTROL_TEL_INTERVAL_MS; 
			SendControlPacket(); 	
			break;
		case ArduStationTelemetry:	
			// abandoned
			mS[TelemetryUpdate] = mSClock() + 0x7fffff;	
			break;
		case CustomTelemetry:
			SpareSlotTime = false; 
			mS[TelemetryUpdate] = mSClock() + CUSTOM_TEL_INTERVAL_MS;
			SendCustom(); 
			break;
		case GPSTelemetry: break;
		}
	}
} // DoTelemetry


#define NAV_STATS_INTERLEAVE	10
static int8 StatsNavAlternate = 0; 

void SendPacketHeader(void) {

	static uint8 b;

	F.TxToBuffer = true;

	#ifdef TELEMETRY_PREAMBLE
	for (b=10;b;b--) 
		TxChar(0x55);
	#endif // TELEMETRY_PREAMBLE
	      
	TxChar(0xff); // synchronisation to "jolt" USART	
	TxChar(SOH);	
	TxCheckSum = 0;
} // SendPacketHeader

void SendPacketTrailer(void) {

	TxESCu8(TxCheckSum);	
	TxChar(EOT);
	
	TxChar(CR);
	TxChar(LF); 

	F.TxToBuffer = false; 
} // SendPacketTrailer

void ShowAttitude(void) {

	TxESCi16(A[Roll].Desired);
	TxESCi16(A[Pitch].Desired);
	TxESCi16(A[Yaw].Desired);

	TxESCi16(A[Roll].Rate);
	TxESCi16(A[Pitch].Rate);

	TxESCi16(A[Yaw].Rate);

	#ifdef INC_RAW_ANGLES

	TxESCi16(A[Roll].RawAngle);
	TxESCi16(A[Pitch].RawAngle);

	#else

	TxESCi16(A[Roll].Angle);
	TxESCi16(A[Pitch].Angle);

	#endif // INC_RAW_ANGLES

	TxESCi16(A[Yaw].RateEp); // rate error as analogue for heading arror

	TxESCi16(A[Roll].Acc);
    TxESCi16(A[Pitch].Acc);
	TxESCi16(A[Yaw].Acc);

} // ShowAttitude

void SendFlightPacket(void) {
	static uint8 b;

	SendPacketHeader();

	TxESCu8(UAVXFlightPacketTag);
	TxESCu8(48 + TELEMETRY_FLAG_BYTES);
	for ( b = 0; b < (uint8)TELEMETRY_FLAG_BYTES; b++ )
		TxESCu8(F.AllFlags[b]); 
		
	TxESCu8(State);	

	TxESCi16(BatteryVoltsADC);
	TxESCi16(BatteryCurrentADC);
	TxESCi16(BatteryChargeUsedmAH);
 
	TxESCi16(RCGlitches);			
	TxESCi16(DesiredThrottle);

	ShowAttitude();

	TxESCi8((int8)A[Roll].AngleCorr);
	TxESCi8((int8)A[Pitch].AngleCorr);
	TxESCi8((int8)AccAltComp);
	TxESCi8((int8)AltComp);

	for ( b = 0; b < (uint8)6; b++ ) // motor/servo channels
	 	TxESCu8((uint8)Limit(PWM[b],0,255));

	TxESCi24(mSClock() - mS[StartTime]);

	SendPacketTrailer();
} // SendFlightPacket

void SendControlPacket(void){
	static uint8 b;

	SendPacketHeader();

	TxESCu8(UAVXControlPacketTag);
	TxESCu8(36);

	TxESCi16(DesiredThrottle);
 			
	ShowAttitude();

	TxESCu8(UAVXAirframe);

	for ( b = 0; b < (uint8)6; b++ ) // motor/servo channels
		TxESCu8((uint8)Limit(PWM[b],0,255));

	TxESCi24(mSClock() - mS[StartTime]);

	SendPacketTrailer();

} // SendControlPacket

void SendNavPacket(void){

	SendPacketHeader();

	TxESCu8(UAVXNavPacketTag);
	TxESCu8(59);
		
	TxESCu8(NavState);
	TxESCu8(FailState);
	TxESCu8(GPSNoOfSats);
	TxESCu8(GPSFix);
	
	TxESCu8(CurrWP);	
	
	TxESCi16(ROC); 	// was BaroROC				// cm/S
	TxESCi24(BaroRelAltitude);
	
	TxESCi16(NewCruiseThrottle); 				
	TxESCi16(RangefinderAltitude); 				// cm
	
	TxESCi16(GPSHDilute);
	TxESCi16(Heading);
	TxESCi16(WayHeading);
	
	TxESCi16(GPSVel);
	TxESCi16(GPSHeading); 						// GPS ROC dm/S
	
	TxESCi24(GPSRelAltitude); 					// cm
	TxESCi32(GPSLatitude); 						// 5 decimal minute units
	TxESCi32(GPSLongitude); 
	
	TxESCi24(DesiredAltitude);
	TxESCi32(DesiredLatitude); 
	TxESCi32(DesiredLongitude);
	
	TxESCi24(mS[NavStateTimeout] - mSClock());	// mS
	
	TxESCi16(AmbientTemperature.i16);			// 0.1C
	TxESCi32(GPSMissionTime);

	TxESCu8(NavSensitivity);
	TxESCi8(A[Roll].NavCorr);
	TxESCi8(A[Pitch].NavCorr);
	TxESCi8(A[Yaw].NavCorr);

	SendPacketTrailer();

} // SendNavPacket

void SendStatsPacket(void) 
{
	static uint8 i;

	SendPacketHeader();

    TxESCu8(UAVXStatsPacketTag);
    TxESCu8(MAX_STATS * 2 + 2);

    for ( i = 0; i < (uint8)MAX_STATS ; i++)
        TxESCi16(Stats[i]);

    TxESCu8(UAVXAirframe);
    TxESCu8(Orientation);

	SendPacketTrailer();

} // SendStatsPacket

void SendMinPacket(void)
{
	static uint8 b;

	SendPacketHeader();

	TxESCu8(UAVXMinPacketTag);
	TxESCu8(33 + TELEMETRY_FLAG_BYTES);
	for ( b = 0; b < (uint8)TELEMETRY_FLAG_BYTES; b++ )
		TxESCu8(F.AllFlags[b]); 
		
	TxESCu8(State);	
	TxESCu8(NavState);	
	TxESCu8(FailState);

	TxESCi16(BatteryVoltsADC);
	TxESCi16(BatteryCurrentADC);
	TxESCi16(BatteryChargeUsedmAH);	

	TxESCi16(A[Roll].Angle);
	TxESCi16(A[Pitch].Angle);
		
	TxESCi24(BaroRelAltitude);
	TxESCi16(RangefinderAltitude); 		

	TxESCi16(Heading);
	
	TxESCi32(GPSLatitude); 					
	TxESCi32(GPSLongitude);

	TxESCu8(UAVXAirframe);
	TxESCu8(Orientation);

	TxESCi24(mSClock() - mS[StartTime]);

	SendPacketTrailer();

} // SendMinPacket

void SendParamPacket(uint8 s, uint8 p) {

    SendPacketHeader();

    TxESCu8(UAVXParamPacketTag);
    TxESCu8(3);
    TxESCu8(s);
	TxESCu8(p);
    TxESCi8( ReadEE( (s-1) * MAX_PARAMETERS + p ) );
	SendPacketTrailer();
 
} // SendParamPacket

void SendParameters(uint8 s) {

	static uint8 p;

	for ( p = 0; p < (uint8)MAX_PARAMETERS; p++ )
		SendParamPacket(s, p );
	SendParamPacket(0, MAX_PARAMETERS);
} // SendParameters

void SendCycle(void) 
{	// 
	// 0.8mS at 40MHz
	
	switch ( UAVXCurrPacketTag ) {
	case UAVXFlightPacketTag:
		SendFlightPacket();
		UAVXCurrPacketTag = UAVXNavPacketTag;
		break;	
	case UAVXNavPacketTag:
		if ( ++StatsNavAlternate < NAV_STATS_INTERLEAVE )
			SendNavPacket();		
		else
		{
			SendStatsPacket();
			StatsNavAlternate = 0;
		}
		UAVXCurrPacketTag = UAVXFlightPacketTag;
		break;	
	default:
		UAVXCurrPacketTag = UAVXFlightPacketTag;
		break;		
	} // switch
			
} // SendCycle

void SendCustom(void) {
 	// user defined telemetry human readable OK for small amounts of data < 1mS

	F.TxToBuffer = true;
	
	// insert values here using TxVal32(n, dp, separator)
	// dp is the scaling to decimal places, separator
	// separator may be a single 'char', HT for tab, or 0 (no space)
	// -> 

	// add user specific code

	// <-

	TxChar(CR);
	TxChar(LF);

	F.TxToBuffer = false;
} // SendCustom





