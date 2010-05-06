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

// 	GPS routines

#include "uavx.h"

void UpdateField(void);
int32 ConvertGPSToM(int32);
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
// Moving average of coordinates - Kalman Estimator probably needed
//#define GPSFilter NoFilter
#define GPSFilter SoftFilter				
//#define GPSFilter MediumFilter

#define GPSVelocityFilter SoftFilterU		// done after position filter

const rom uint8 NMEATag[6] = {"GPGGA"};

#pragma udata gpsbuff
struct {
	uint8 s[GPSRXBUFFLENGTH];
	uint8 length;
	} NMEA;
#pragma udata

#pragma udata gpsvars
int32 	GPSMissionTime, GPSStartTime;
int32 	GPSLatitude, GPSLongitude;
int32 	OriginLatitude, OriginLongitude;
int24 	GPSAltitude, GPSRelAltitude, GPSOriginAltitude;
int32 	DesiredLatitude, DesiredLongitude;
int32	LatitudeP, LongitudeP, HoldLatitude, HoldLongitude;
int16 	GPSLongitudeCorrection;
int16 	GPSVel, GPSROC;
uint8 	GPSNoOfSats;
uint8 	GPSFix;
int16 	GPSHDilute;
int32Q	GPSQ32;
#pragma udata

#ifdef SIMULATE
	int32 FakeGPSLongitude, FakeGPSLatitude;
#endif // SIMULATE

#pragma udata gpsvars1
uint8 nll, cc, lo, hi;
boolean EmptyField;
int16 ValidGPSSentences;
#pragma udata

int32 SumGPSRelAltitude, SumCompBaroPress;

int32 ConvertGPSToM(int32 c)
{	// approximately 1.8553257183 cm per LSB at the Equator
	// conversion max is 21Km
	return ( ((int32)c * (int32)1855)/((int32)100000) );
} // ConvertGPSToM

int32 ConvertMToGPS(int32 c)
{
	// conversion max is 21Km 
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
	// approximately 1.855 cm per LSB at the Equator.
	static int32 dd, mm, dm, r;
	static int8 dp;	
	
	r = 0;
	if ( !EmptyField )
	{
		dp = lo + 4; // could do this in initialisation for Lat and Lon?
		while ( NMEA.s[dp] != '.') dp++;

	    dd = ConvertInt(lo, dp - 3);
	    mm = ConvertInt(dp - 2 , dp - 1);
		if ( ( hi - dp ) > 4 )
			dm = ConvertInt(dp + 1, dp + 5);
		else
			dm = ConvertInt(dp + 1, dp + 4)* 10L;
			
	    r = dd * 6000000 + mm * 100000 + dm;
	}
	
	return(r);
} // ConvertLatLonM

/*	
int32 ConvertUTime(uint8 lo, uint8 hi)
{
	int32 ival, hh;
	int16 mm, ss;
	
	ival=0;
	if ( !EmptyField )
		ival=(int32)(ConvertInt(lo, lo+1))*3600+
				(int32)(ConvertInt(lo+2, lo+3)*60)+
				(int32)(ConvertInt(lo+4, hi));
	      
	return(ival);
} // ConvertUTime
*/

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

    UpdateField();
    
    UpdateField();   //UTime
	//GPSMissionTime=ConvertUTime(lo,hi);

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
	GPSAltitude = ( ConvertInt(lo, hi-2) * 10L + ConvertInt(hi, hi) ) * 10L; // Centimetres

    //UpdateField();   // AltUnit - assume Metres!

    //UpdateField();   // GHeight 
    //UpdateField();   // GHeightUnit 
 
    F.GPSValid = (GPSFix >= GPS_MIN_FIX) && ( GPSNoOfSats >= GPS_MIN_SATELLITES );

	if ( State == InFlight )
	{
		if ( GPSHDilute > Stats[MaxHDiluteS] )
		{ 
			Stats[MaxHDiluteS] = GPSHDilute;
			F.GPSFailure = GPSHDilute > 150; 
		}
		else 
			if ( GPSHDilute < Stats[MinHDiluteS] ) 
				Stats[MinHDiluteS] = GPSHDilute;

		if ( GPSNoOfSats > Stats[GPSMaxSatsS] )
			Stats[GPSMaxSatsS] = GPSNoOfSats;
		else
			if ( GPSNoOfSats < Stats[GPSMinSatsS] )
				Stats[GPSMinSatsS] = GPSNoOfSats; 
	}

	#ifdef TEST_GPS
	if ( F.GPSTestActive )
	{
		TxString("$GPGGA ");
		if ( !F.GPSValid )
		{
			TxString(" fx=");
			TxVal32(GPSFix, 0, ' ');
	
			TxString("s=");
			TxVal32(GPSNoOfSats, 0, ' ');
	
			TxString("hd=");
			TxVal32(GPSHDilute, 2, ' ');

			TxString("invalid\r\n");
		}
	}
	#endif // TEST_GPS
} // ParseGPGGASentence

void SetGPSOrigin(void)
{
	static int32 Temp;

	if ( ( ValidGPSSentences == GPS_ORIGIN_SENTENCES ) && F.GPSValid )
	{
		GPSStartTime = GPSMissionTime;
		OriginLatitude = DesiredLatitude = HoldLatitude = LatitudeP = GPSLatitude;
		OriginLongitude = DesiredLongitude = HoldLongitude = LongitudeP = GPSLongitude;
		GPSVel = 0;
		
		#ifdef SIMULATE
			FakeGPSLongitude = GPSLongitude;
			FakeGPSLatitude = GPSLatitude;
		#endif // SIMULATE

		mS[LastGPS] = mS[Clock];
			
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
	static int32 Temp;
	static int24 LongitudeDiff, LatitudeDiff;
	static int16 GPSVelP;
	static int24 GPSInterval;

 
	#ifdef SIMULATE
	static int16 CosH, SinH, A;

	#define FAKE_NORTH_WIND 	0L
	#define FAKE_EAST_WIND 		0L
    #define SCALE_VEL			64L
	#endif // SIMULATE

	cc = 0;
	nll = NMEA.length;

	ParseGPGGASentence(); 

	if ( F.GPSValid )
	{
		// all coordinates in 0.00001 Minutes or ~1.8553cm relative to Origin
		// There is a lot of jitter in position - could use Kalman Estimator?

		GPSInterval = mS[Clock] - mS[LastGPS];
		mS[LastGPS] = mS[Clock];

	    if ( ValidGPSSentences <  GPS_ORIGIN_SENTENCES )
		{   // repetition to ensure GPGGA altitude is captured

			if ( F.GPSTestActive )
			{
				TxVal32( GPS_ORIGIN_SENTENCES - ValidGPSSentences, 0, 0);
				TxNextLine();
			}

			F.GPSValid = false;

			if ( GPSHDilute <= GPS_MIN_HDILUTE )
				ValidGPSSentences++;
			else
				ValidGPSSentences = 0;
		}

		#ifdef SIMULATE

		if ( State == InFlight )
		{  // don't bother with GPS longitude correction for now?
			CosH = int16cos(Heading);
			SinH = int16sin(Heading);
			GPSLongitude = FakeGPSLongitude + ((int32)(-FakeDesiredPitch) * SinH)/SCALE_VEL;
			GPSLatitude = FakeGPSLatitude + ((int32)(-FakeDesiredPitch) * CosH)/SCALE_VEL;
								
			A = Make2Pi(Heading + HALFMILLIPI);
			CosH = int16cos(A);
			SinH = int16sin(A);
			GPSLongitude += ((int32)FakeDesiredRoll * SinH) / SCALE_VEL;
			GPSLongitude += FAKE_EAST_WIND; // wind	
			GPSLatitude += ((int32)FakeDesiredRoll * CosH) / SCALE_VEL;
			GPSLatitude += FAKE_NORTH_WIND; // wind
		
			FakeGPSLongitude = GPSLongitude;
			FakeGPSLatitude = GPSLatitude;
		}

		if ( F.NavValid )
		{
			GPSRelAltitude = RelBaroAltitude;
			GPSROC = BaroROC;
		}
		
		#else

		if (F.NavValid )
		{
			Temp = GPSAltitude - GPSOriginAltitude;	
			if ( mS[Clock] > mS[GPSROCUpdate] )
			{
				mS[GPSROCUpdate] = mS[Clock] + 1000; // 1Sec
				GPSROC = Temp - GPSRelAltitude;
			}
			GPSRelAltitude = Temp;
		}			

		#endif // SIMULATE

		#ifdef GPS_INC_GROUNDSPEED
		// nice to have but not essential 
		LongitudeDiff = SRS32((GPSLongitude - LongitudeP) * GPSLongitudeCorrection, 8);
		LatitudeDiff = GPSLatitude - LatitudeP;
		Temp = int32sqrt(LongitudeDiff*LongitudeDiff + LatitudeDiff*LatitudeDiff);
		GPSVel = ConvertGPSToM( (Temp*GPSInterval) / 100L ); // dM/Sec.
		GPSVel = GPSVelocityFilter(GPSVelP, GPSVel);
		GPSVelP = GPSVel;

		LatitudeP = GPSLatitude;
		LongitudeP = GPSLongitude;
		#endif // GPS_INC_GROUNDSPEED			

		if ( State == InFlight )
		{
			if ( GPSRelAltitude > MaxGPSAltitudeS )
				MaxGPSAltitudeS = GPSRelAltitude;

			if ( GPSVel > Stats[GPSVelS] )
				Stats[GPSVelS] = GPSVel;

			if (( GPSRelAltitude > 1000 ) && ( GPSRelAltitude < 2500 )) // 10-25M
			{
				SumGPSRelAltitude += GPSRelAltitude;
				SumCompBaroPress += CompBaroPress;
			}
		}
	}
	else
		if ( State == InFlight ) 
			Stats[GPSInvalidS]++;

} // ParseGPSSentence

void InitGPS(void)
{ 
	uint8 n;

	cc = 0;

	GPSLongitudeCorrection = 256; // 1.0
	GPSMissionTime = GPSFix = GPSNoOfSats = GPSHDilute = 0;
	GPSRelAltitude = GPSROC = GPSAltitude = 0;
	GPSVel = 0;

	OriginLatitude = DesiredLatitude = HoldLatitude = LatitudeP = GPSLatitude = 0;
	OriginLongitude = DesiredLongitude = HoldLongitude = LongitudeP = GPSLongitude = 0;

	Write32EE(NAV_ORIGIN_LAT, 0);
	Write32EE(NAV_ORIGIN_LON, 0);
	Write16EE(NAV_ORIGIN_ALT, 0);

	ValidGPSSentences = 0;

	SumGPSRelAltitude = SumCompBaroPress = 0;

	F.NavValid = F.GPSValid = F.GPSSentenceReceived = false;
  	GPSRxState = WaitGPSSentinel; 
  	
} // InitGPS

void UpdateGPS(void)
{
	if ( F.GPSSentenceReceived )
	{
		LEDBlue_ON;
		LEDRed_OFF;
		F.GPSSentenceReceived = false;  
		ParseGPSSentence(); // 7.5mS 18f2520 @ 16MHz
		if ( F.GPSValid )
		{
			F.NavComputed = false;
			mS[GPSTimeout] = mS[Clock] + GPS_TIMEOUT_MS;
		}
		else
		{
			NavPCorr = DecayX(NavPCorr, 2);
			NavRCorr = DecayX(NavRCorr, 2);
			NavYCorr = 0;
			EastDiffP = NorthDiffP = EastDiffSum = NorthDiffSum = 0;
		}

		if ( !F.GPSTestActive )			SendTelemetry();	// Tx overlapped with next GPS packet Rx
	}
	else
		if( mS[Clock] > mS[GPSTimeout] )
			F.GPSValid = false;

	LEDBlue_OFF;
	if ( F.GPSValid )
		LEDRed_OFF;
	else
		LEDRed_ON;	
} // UpdateGPS

