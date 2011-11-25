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

// 	GPS routines

#include "uavx.h"

void UpdateField(void);
int32 ConvertGPSToM(int32);
int32 ConvertGPSTodM(int32);
int32 ConvertMToGPS(int32);
int24 ConvertInt(uint8, uint8);
int32 ConvertLatLonM(uint8, uint8);
int32 ConvertUTime(uint8, uint8);
void ParseGPRMCSentence(void);
void ParseGPGGASentence(void);
void SetGPSOrigin(void);
void ParseGPSSentence(void);
void InitGPS(void);
void UpdateGPS(void);

// Defines
// Moving average of coordinates needed - or Kalman Estimator probably

#define GPSVelocityFilter SoftFilterU		// done after position filter

const uint8 NMEATags[MAX_NMEA_SENTENCES][5]= {
    // NMEA
    {'G','P','G','G','A'}, // full positioning fix
    {'G','P','R','M','C'}, // main current position and heading
};

#pragma udata gpsbuff
NMEAStruct NMEA;
#pragma udata

#pragma udata gpsvars
uint8 	GPSPacketTag;
int32 	GPSMissionTime, GPSStartTime;
int32 	GPSLatitude, GPSLongitude, GPSLatitudeP, GPSLongitudeP;
int16	GPSEastDiff, GPSNorthDiff;
int32 	OriginLatitude, OriginLongitude;
int24 	GPSAltitude, GPSRelAltitude, GPSOriginAltitude;
int32 	DesiredLatitude, DesiredLongitude;
int32	HoldLatitude, HoldLongitude;
int16 	GPSLongitudeCorrection;
int16	GPSHeading;
int16 	GPSVel, GPSVelP;
int8 	GPSNoOfSats;
int8 	GPSFix;
int16 	GPSHDilute;
#pragma udata

int32 FakeGPSLongitude, FakeGPSLatitude;

#pragma udata gpsvars1
uint8 nll, cc, lo, hi;
boolean EmptyField;
int16 ValidGPSSentences;
#pragma udata

int32 ConvertGPSToM(int32 c)
{	// approximately 1.8553257183 cm per NMEA LSD with 5 decimal minutes at the Equator
	// 32bit is 0.933 cm less than a factor of 2
	// conversion max is 21Km
	return ( ((int32)c * (int32)1855)/((int32)100000) );
} // ConvertGPSToM

int32 ConvertGPSTodM(int32 c)
{	
	// conversion max is 21Km
	return ( ((int32)c * (int32)1855)/((int32)10000) );
} // ConvertGPSTodM

int32 ConvertMToGPS(int32 c)
{
	return ( ((int32)c * (int32)100000)/((int32)1855) );
} // ConvertMToGPS

int24 ConvertInt(uint8 lo, uint8 hi)
{
	static uint8 i;
	static int24 r;

	r = 0;
	if ( !EmptyField )
		for (i = lo; i <= hi ; i++ )
			r = r * 10 + NMEA.s[i] - '0';

	return (r);
} // ConvertInt

int32 ConvertLatLonM(uint8 lo, uint8 hi)
{ 	// NMEA coordinates normally assumed as DDDMM.MMMMM ie 5 decimal minute digits
	// but code can deal with 4 and 5 decimal minutes 
	// Positions are stored at 5 decimal minute NMEA resolution which is
	// approximately 1.855 cm per LSD at the Equator.
	static int32 dd, mm, dm, r;
	static uint8 dp;	
	
	r = 0;
	if ( !EmptyField )
	{
		dp = lo + 4; // could do this in initialisation for Lat and Lon?
		while ( NMEA.s[dp] != '.') dp++;

	    dd = ConvertInt(lo, dp - 3);
	    mm = ConvertInt(dp - 2 , dp - 1);
		if ( ( hi - dp ) > (uint8)4 )
			dm = ConvertInt(dp + 1, dp + 5);
		else
			dm = ConvertInt(dp + 1, dp + 4) * 10L;
			
	    r = dd * 6000000 + mm * 100000 + dm;
	}
	
	return(r);
} // ConvertLatLonM

int32 ConvertUTime(uint8 lo, uint8 hi)
{
	static int32 ival;
	
	ival=0;
	if ( !EmptyField )
		ival=(int32)(ConvertInt(lo, lo+1))*3600+
				(int32)(ConvertInt(lo+2, lo+3)*60)+
				(int32)(ConvertInt(lo+4, hi));
	      
	return(ival);
} // ConvertUTime

void UpdateField(void)
{
	static uint8 ch;

	lo = cc;

	ch = NMEA.s[cc];
	while (( ch != ',' ) && ( ch != '*' ) && ( cc < nll )) 
		ch = NMEA.s[++cc];

	hi = cc - 1;
	cc++;
	EmptyField = hi < lo;
} // UpdateField

void ParseGPGGASentence(void)
{ 	// full position $GPGGA fix
	// 0.745mS @ 16Mhz
	// 0.29mS @ 40MHz 

    UpdateField();
    
    UpdateField();   //UTime
	GPSMissionTime = ConvertUTime(lo,hi);

	UpdateField();   	//Lat
    GPSLatitude = ConvertLatLonM(lo, hi);
    UpdateField();   //LatH
    if (NMEA.s[lo] == 'S') GPSLatitude = -GPSLatitude;

    UpdateField();   	//Lon   
    GPSLongitude = ConvertLatLonM(lo, hi);
    UpdateField();   	//LonH
	if (NMEA.s[lo] == 'W') GPSLongitude = -GPSLongitude;
        
    UpdateField();   	//Fix 
    GPSFix = (uint8)(ConvertInt(lo,hi));

    UpdateField();   	//Sats
    GPSNoOfSats = (uint8)(ConvertInt(lo,hi));

    UpdateField();   	// HDilute
	GPSHDilute = ConvertInt(lo, hi-3) * 100 + ConvertInt(hi-1, hi); 

    UpdateField();   	// Alt

	GPSAltitude = ConvertInt(lo, hi-2) * 10L + ConvertInt(hi, hi) * 10; // Decimetres

    //UpdateField();   // AltUnit - assume Metres!

    //UpdateField();   // GHeight 
    //UpdateField();   // GHeightUnit 

   	F.GPSValid = (GPSFix >= GPS_MIN_FIX) && ( GPSNoOfSats >= GPS_MIN_SATELLITES );

	if ( State == InFlight )
	{
		if ( GPSHDilute > Stats[MaxHDiluteS] )
			Stats[MaxHDiluteS] = GPSHDilute;
		else 
			if ( GPSHDilute < Stats[MinHDiluteS] ) 
				Stats[MinHDiluteS] = GPSHDilute;

		if ( GPSNoOfSats > Stats[GPSMaxSatsS] )
			Stats[GPSMaxSatsS] = GPSNoOfSats;
		else
			if ( GPSNoOfSats < Stats[GPSMinSatsS] )
				Stats[GPSMinSatsS] = GPSNoOfSats; 

		F.GPSFailure = GPSHDilute > 150; 
	}
} // ParseGPGGASentence

void ParseGPRMCSentence() 
{ 	// main current position and heading
	// 0.28mS @ 16MHz
	// 0.1mS @ 40MHz

	static i32u Temp32;

    UpdateField();

    UpdateField();   //UTime

    UpdateField();
    if ( NMEA.s[lo] == 'A' ) {
	
		F.GPSValid = true;

        UpdateField();   //Lat
        GPSLatitude = ConvertLatLonM(lo,hi);
        UpdateField();   //LatH
        if (NMEA.s[lo] == 'S')
        	GPSLatitude= -GPSLatitude;

        UpdateField();   //Lon
        GPSLongitude = ConvertLatLonM(lo,hi);

        UpdateField();   //LonH
        if ( NMEA.s[lo] == 'W' )
        	GPSLongitude = -GPSLongitude;

        UpdateField();   // Groundspeed (Knots)
		Temp32.i32 = ((int32)ConvertInt(lo, hi-3) * 100L + ConvertInt(hi-1, hi)) * 13L;//  5.144444 dMPS/Kt
		GPSVel = Temp32.i3_1;

        UpdateField();   // True course made good (Degrees)
		if ( GPSVel > 10 )
		{ 
			Temp32.i32 = ((int32)ConvertInt(lo, hi-3) * 100L + ConvertInt(hi-1, hi)) * 45L; // MilliRadians 3142/18000; 
			GPSHeading = Temp32.i3_1;
		}
		else
			GPSHeading = 0;
      	/*
        UpdateField();   //UDate

        UpdateField();   // Magnetic Deviation (Degrees)
        GPSMagDeviation = ConvertReal(lo, hi) * MILLIRAD;

        UpdateField();   // East/West
 
		if ( NMEA.s[lo] == 'W')
            GPSMagHeading = GPSHeading - GPSMagDeviation; // need to check sign????
        else
            GPSMagHeading = GPSHeading +  GPSMagDeviation;
		*/
        F.HaveGPRMC = true;
    } else
        F.HaveGPRMC = false;

} // ParseGPRMCSentence

void SetGPSOrigin(void)
{
	static int32 Temp;

	if ( ( ValidGPSSentences == GPS_ORIGIN_SENTENCES ) && F.GPSValid )
	{
		GPSStartTime = GPSMissionTime;
		OriginLatitude = DesiredLatitude = HoldLatitude = GPSLatitudeP = GPSLatitude;
		OriginLongitude = DesiredLongitude = HoldLongitude = GPSLongitudeP = GPSLongitude;

		GPSVel = 0;
		
		#ifdef SIMULATE
			FakeGPSLongitude = GPSLongitude;
			FakeGPSLatitude = GPSLatitude;
		#endif // SIMULATE

		mS[LastGPS] = mSClock();
			
		Temp = GPSLatitude/600000L; // to degrees * 10
		Temp = Abs(Temp);
		Temp = ConvertDDegToMPi(Temp);
		GPSLongitudeCorrection = int16cos(Temp);
	
		GPSOriginAltitude = GPSAltitude;

		Write16EE(NAV_ORIGIN_ALT, (int16)(GPSAltitude/100));
		Write32EE(NAV_ORIGIN_LAT, GPSLatitude);
		Write32EE(NAV_ORIGIN_LON, GPSLongitude);

		if ( !F.NavValid )
		{
			DoBeep100mSWithOutput(2,0);
			Stats[NavValidS] = true;
			F.NavValid = true;
		}
		F.AcquireNewPosition = true;		
	}
} // SetGPSOrigin

void ParseGPSSentence(void)
{
	// 0.94mS @ 16MHz GPGGA
	// 0.37mS @ 40MHz GPGGA

	static i32u Temp32u;
	static int24 MaxDiff, LongitudeDiff, LatitudeDiff;
	static int24 GPSInterval;

	#ifdef SIMULATE
		static int16 CosH, SinH, H;
	#endif // SIMULATE
	
	#define FAKE_NORTH_WIND 	0L
	#define FAKE_EAST_WIND 		0L
	#define SCALE_VEL			6  // was 5


	cc = 0;
	nll = NMEA.length;

	switch ( GPSPacketTag ) {
    case GPGGAPacketTag:
		ParseGPGGASentence();
		break;
	case GPRMCPacketTag:
		ParseGPRMCSentence();
		break;
	} 

	if ( F.GPSValid )
	{
		// all coordinates in 0.00001 Minutes or ~1.8553cm relative to Origin
		// There is a lot of jitter in position - could use Kalman Estimator?

		GPSInterval = mSClock() - mS[LastGPS];
		mS[LastGPS] = mSClock();

	    if ( ValidGPSSentences <  GPS_ORIGIN_SENTENCES )
		{   // repetition to ensure GPGGA altitude is captured
			F.GPSValid = false;

			if ( GPSHDilute <= GPS_MIN_HDILUTE )
				ValidGPSSentences++;
			else
				ValidGPSSentences = 0;
		}

		#ifdef SIMULATE

		if ( State == InFlight )
		{  // don't bother with GPS longitude correction for now?
			GPSRelAltitude = BaroRelAltitude;

			CosH = int16cos(Heading);
			SinH = int16sin(Heading);

			FakeGPSLongitude += FAKE_EAST_WIND; 
			FakeGPSLatitude += FAKE_NORTH_WIND; 

			#ifdef NAV_WING

				FakeGPSLongitude += ConvertMToGPS(SIM_CRUISE_MPS*SinH)/(GPS_UPDATE_HZ*256);
				FakeGPSLatitude += ConvertMToGPS(SIM_CRUISE_MPS*CosH)/(GPS_UPDATE_HZ*256);

			#else

//	Temp32.i32 = (DesiredLongitude - GPSLongitude) * GPSLongitudeCorrection;
//	LongitudeDiff = Temp32.i3_1;
	
				FakeGPSLongitude -= SRS32((int32)A[Pitch].FakeDesired * SinH * 256 / GPSLongitudeCorrection, SCALE_VEL);
				FakeGPSLatitude -= SRS32((int32)A[Pitch].FakeDesired * CosH, SCALE_VEL);
									
				H = Make2Pi(Heading + HALFMILLIPI);
				CosH = int16cos(H);
				SinH = int16sin(H);
				FakeGPSLongitude += SRS32((int32)A[Roll].FakeDesired * SinH  * 256 / GPSLongitudeCorrection, SCALE_VEL);
				FakeGPSLatitude += SRS32((int32)A[Roll].FakeDesired * CosH, SCALE_VEL);

			#endif // NAV_WING

			LongitudeDiff = FakeGPSLongitude - GPSLongitudeP;
			LatitudeDiff = FakeGPSLatitude - GPSLatitudeP;

			GPSVel = int32sqrt(Sqr(LongitudeDiff)+Sqr(LatitudeDiff));
			GPSVel = ConvertGPSTodM(GPSVel);

			GPSLongitude = FakeGPSLongitude;
			GPSLatitude = FakeGPSLatitude;

			GPSLatitudeP = GPSLatitude;
			GPSLongitudeP = GPSLongitude;
		}
		
		#else
		if (F.NavValid )
		{
			GPSRelAltitude  = GPSAltitude - GPSOriginAltitude;

			LatitudeDiff = GPSLatitude - GPSLatitudeP; // do again after slew limit
			LongitudeDiff = GPSLongitude - GPSLongitudeP;

			MaxDiff = (Abs(LatitudeDiff),Abs(LongitudeDiff));

			if ( MaxDiff > GPS_OUTLIER_LIMIT )
			{
				Stats[BadS]++;
				if ( MaxDiff > Stats[BadNumS] ) 
					Stats[BadNumS] = MaxDiff;
				GPSLatitude = SlewLimit(GPSLatitudeP, GPSLatitude, GPS_OUTLIER_SLEW_LIMIT );
				GPSLongitude = SlewLimit(GPSLongitudeP, GPSLongitude, GPS_OUTLIER_SLEW_LIMIT );
				LatitudeDiff = GPSLatitude - GPSLatitudeP; // do again after slew limit
				LongitudeDiff = GPSLongitude - GPSLongitudeP;
			}

			GPSLatitudeP = GPSLatitude;
			GPSLongitudeP = GPSLongitude;	
		}

		#endif // SIMULATE

		if ( State == InFlight )
		{
			if ( GPSRelAltitude > Stats[GPSAltitudeS] )
				Stats[GPSAltitudeS] = GPSRelAltitude;

			if ( GPSVel > Stats[GPSVelS] )
				Stats[GPSVelS] = GPSVel;
		}
	}
	else
		if ( State == InFlight ) 
			Stats[GPSInvalidS]++;

} // ParseGPSSentence

void UpdateGPS(void)
{
	if ( SpareSlotTime && F.NormalFlightMode )
		if ( F.PacketReceived )
		{
			SpareSlotTime = false;
			LEDBlue_ON;
			LEDRed_OFF;
			F.PacketReceived = false;  
			ParseGPSSentence(); // 3mS 18f2620 @ 40MHz
			if ( F.GPSValid )
			{
				LEDRed_OFF;
				F.NavComputed = false;
				mS[GPSTimeout] = mSClock() + GPS_TIMEOUT_MS;
			}
			else
				LEDRed_ON;
			LEDBlue_OFF;
		}
		else
		{
			if( mSClock() > mS[GPSTimeout] )
			{
				F.GPSValid = false;
				LEDRed_ON;
			}
			LEDBlue_OFF;
		}

} // UpdateGPS

void InitGPS(void)
{
	cc = 0;

	GPSLongitudeCorrection = 256; // 1.0
	GPSMissionTime = GPSFix = GPSNoOfSats = GPSHDilute = 0;
	GPSRelAltitude = GPSAltitude = 0;
	GPSHeading = GPSVel = GPSVelP = 0;

	OriginLatitude = DesiredLatitude = HoldLatitude = GPSLatitudeP = GPSLatitude = 0;
	OriginLongitude = DesiredLongitude = HoldLongitude = GPSLongitudeP = GPSLongitude = 0;

	Write32EE(NAV_ORIGIN_LAT, 0);
	Write32EE(NAV_ORIGIN_LON, 0);
	Write16EE(NAV_ORIGIN_ALT, 0);

	ValidGPSSentences = 0;

	F.NavValid = F.GPSValid = F.PacketReceived = false;
  	RxState = WaitSentinel; 

} // InitGPS
