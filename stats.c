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

void ZeroStats(void);
void ReadStatsEE(void);
void WriteStatsEE(void);
void ShowStats(void);

#pragma udata stats
int16 	Stats[MAX_STATS];
#pragma udata

#define INIT_MIN 1000L

void ZeroStats(void)
{
	int8 s;

	for (s = 0 ; s < MAX_STATS ; s++ )
		Stats[s] = 0;

	Stats[MinHDiluteS] = INIT_MIN;
	Stats[MaxHDiluteS] = 0;
	Stats[MinBaroROCS] = INIT_MIN;
	Stats[MaxBaroROCS] = 0;
	Stats[GPSMinSatsS] = INIT_MIN;
	Stats[GPSMaxSatsS] = 0;
	Stats[MinTempS] = INIT_MIN;
	Stats[MaxTempS] = 0;

} // ZeroStats

void ReadStatsEE(void)
{
	int8 s;

	for (s = 0 ; s < MAX_STATS ; s++ )
		Stats[s] = Read16EE(STATS_ADDR_EE + s*2);
} // InitStats

void WriteStatsEE()
{
	int8 s, i;
	int16 Temp;

	if ( P[ESCType] != ESCPPM )
		for ( i = 0; i < NoOfPWMOutputs; i++ )
			Stats[ESCI2CFailS] += ESCI2CFail[i];

	for (s = 0 ; s < MAX_STATS ; s++ )
		Write16EE(STATS_ADDR_EE + s*2, Stats[s]);

	Temp = ToPercent(CruiseThrottle, OUT_MAXIMUM);
	WriteEE(PercentCruiseThr, Temp);

} // WriteStatsEE

void ShowStats(void)
{
	TxString("\r\nFlight Statistics\r\n");

	if ( Stats[BadS] != 0 )
	{
		TxString("Misc(gke):     \t");TxVal32((int32)Stats[BadS],0,' '); TxVal32((int32)Stats[BadNumS],0,' ');TxNextLine();
	}

	TxString("\r\nSensor/Rx Failures (Count)\r\n");
	TxString("I2CBus:\t");TxVal32((int32)Stats[I2CFailS],0,0); TxNextLine();
	TxString("GPS:   \t");TxVal32((int32)Stats[GPSInvalidS],0,0); TxNextLine();
	TxString("Acc:   \t");TxVal32((int32)Stats[AccFailS], 0, 0); TxNextLine();
	TxString("Gyro:  \t");TxVal32((int32)Stats[GyroFailS], 0, 0); TxNextLine();
	TxString("Comp:  \t");TxVal32((int32)Stats[CompassFailS], 0, 0); TxNextLine();
	TxString("Baro:  \t");TxVal32((int32)Stats[BaroFailS],0 , 0); TxNextLine();
	if ( P[ESCType] != ESCPPM )
	{
		TxString("I2CESC:   \t");TxVal32((int32)Stats[ESCI2CFailS],0 , 0); TxNextLine();
	}
	TxString("Rx:       \t");TxVal32((int32)Stats[RCGlitchesS],0,' '); TxNextLine(); 
	TxString("Failsafes:\t");TxVal32((int32)Stats[RCFailsafesS],0,' '); TxNextLine();
	
	TxString("\r\nBaro\r\n"); // can only display to 3276M
	TxString("Alt:      \t");TxVal32((int32)Stats[BaroRelAltitudeS], 1, ' '); TxString("M \r\n");
	if ( Stats[MinBaroROCS] < INIT_MIN )
	{
		TxString("ROC:      \t");TxVal32((int32)Stats[MinBaroROCS], 1, ' '); 
							TxVal32((int32)Stats[MaxBaroROCS], 1, ' '); TxString("M/S\r\n");
	}
	
	if ( Stats[MinTempS] < INIT_MIN )
	{
		TxString("Ambient:  \t");TxVal32((int32)Stats[MinTempS], 1, ' '); 
							TxVal32((int32)Stats[MaxTempS], 1, ' '); TxString("C\r\n");
	}

	TxString("\r\nGPS\r\n");
	TxString("Alt:      \t");TxVal32((int32)Stats[GPSAltitudeS], 1, ' '); TxString("M\r\n");
	#ifdef GPS_INC_GROUNDSPEED 
	TxString("Vel:      \t");TxVal32(ConvertGPSToM((int32)Stats[GPSVelS]), 1, ' '); TxString("M/S\r\n"); 
	#endif // GPS_INC_GROUNDSPEED

	if ( Stats[GPSMinSatsS] < INIT_MIN )
	{
		TxString("Sats:     \t");TxVal32((int32)Stats[GPSMinSatsS], 0, ' ');
		TxVal32((int32)Stats[GPSMaxSatsS], 0, 0); TxNextLine();
	}

	if ( Stats[MinHDiluteS] < INIT_MIN )
	{
		TxString("HDilute:  \t");TxVal32((int32)Stats[MinHDiluteS], 2, ' ');
		TxVal32((int32)Stats[MaxHDiluteS], 2, 0); TxNextLine();
	}

	if ( Stats[NavValidS] )
		TxString("Navigation ENABLED\r\n");	
	else
		TxString("Navigation DISABLED (No fix at launch)\r\n");
} // ShowStats


