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


// Barometers Freescale TI ADC and Bosch BMP085 3.8MHz, Bosch SMD500 400KHz  

#include "uavx.h"

int24 AltitudeCF(int24); 
void GetBaroAltitude(void);
void InitBarometer(void);

void BaroTest(void);

uint16	BaroPressure, BaroTemperature;
boolean AcquiringPressure;
int16	BaroOffsetDAC;

#define BARO_MIN_CLIMB			1500	// dM minimum available barometer climb from origin
#define BARO_MIN_DESCENT		-500	// dM minimum available barometer descent from origin

int32	OriginBaroPressure, CompBaroPressure;
int24	BaroRelAltitude, BaroRelAltitudeP;
i16u	BaroVal;
int8	BaroType;
int16 	BaroClimbAvailable, BaroDescentAvailable;
int8	BaroRetries;

int24	FakeBaroRelAltitude;
int8 	SimulateCycles;

int24 AltCF;
int16 TauCF = 7; // 5 FOR BMP085 increasing value reduces filtering
int32 AltF[3] = { 0, 0, 0};

// -----------------------------------------------------------

// Freescale ex Motorola MPX4115 Barometer with ADS7823 12bit ADC

void SetFreescaleMCP4725(int16);
void SetFreescaleOffset(void);
void ReadFreescaleBaro(void);
int24 FreescaleToCm(int24);
void GetFreescaleBaroAltitude(void);
boolean IsFreescaleBaroActive(void);
void InitFreescaleBarometer(void);

void SetFreescaleMCP4725(int16 d)
{	
	static int8 r;
	static i16u dd;

	dd.u16 = d << 4; // left align
	I2CStart();
		WriteI2CByte(MCP4725_WR);
		WriteI2CByte(MCP4725_CMD);
		WriteI2CByte(dd.b1);
		WriteI2CByte(dd.b0);
	I2CStop();
} // SetFreescaleMCP4725

//#define GKEOFFSET

void SetFreescaleOffset(void)
{ 	// Steve Westerfeld
	// 470 Ohm, 1uF RC 0.47mS use 2mS for settling?

	BaroOffsetDAC = MCP4725_MAX;

	SetFreescaleMCP4725(BaroOffsetDAC); 
	Delay1mS(20); // initial settling
	ReadFreescaleBaro();
	while ( (BaroVal.u16 < (uint16)(((uint24)ADS7823_MAX*4L*7L)/10L) ) 
		&& (BaroOffsetDAC > 20) )	// first loop gets close
	{
		BaroOffsetDAC -= 20;					// approach at 20 steps out of 4095
		SetFreescaleMCP4725(BaroOffsetDAC); 
		Delay1mS(20);
		ReadFreescaleBaro();
		LEDYellow_TOG;
	}
	
	BaroOffsetDAC += 200;						// move back up to come at it a little slower
	SetFreescaleMCP4725(BaroOffsetDAC);
	Delay1mS(100);
	ReadFreescaleBaro();

	while( (BaroVal.u16 < (uint16)(((uint24)ADS7823_MAX*4L*3L)/4L) ) && (BaroOffsetDAC > 2) )
	{
		BaroOffsetDAC -= 2;
		SetFreescaleMCP4725(BaroOffsetDAC);
		Delay1mS(10);
		ReadFreescaleBaro();
		LEDYellow_TOG;
	}

	Delay1mS(200); // wait for caps to settle
	F.BaroAltitudeValid = BaroOffsetDAC > 0;
	
} // SetFreescaleOffset

void ReadFreescaleBaro(void)
{
	static int16 B[4];

	mS[BaroUpdate] = mSClock()+ BARO_UPDATE_MS;

	F.BaroAltitudeValid = ReadI2Ci16v(ADS7823_ID, ADS7823_CMD,  B, 4, true);
	if ( F.BaroAltitudeValid )
	{	
		BaroVal.u16 = B[0] + B[1] + B[2] + B[3];
		BaroVal.u16 = (uint16)16380 - BaroVal.u16; // inverting op-amp
	}
	else
	{	
		F.HoldingAlt = false;
		if ( State == InFlight ) 
		{
			Stats[BaroFailS]++; 
			F.BaroFailure = true;
		}
	}

} // ReadFreescaleBaro

int24 FreescaleToCm (int24 p)
{ // decreasing pressure is increase in altitude negate and rescale to cm altitude
	return(-(p * 80 )/ (int16)P[BaroScale] );
}  // FreescaleToCm

void GetFreescaleBaroAltitude(void)
{
	static int24 BaroPressure;

	if ( mSClock() >= mS[BaroUpdate] ) // could run faster with MPX
	{
		ReadFreescaleBaro();
		if ( F.BaroAltitudeValid )
		{
			BaroPressure = (int24)BaroVal.u16; // sum of 4 samples		
			BaroRelAltitude = FreescaleToCm(BaroPressure - OriginBaroPressure);
	
			if ( Abs( BaroRelAltitude - BaroRelAltitudeP ) > BARO_SANITY_CHECK_CM )
				Stats[BaroFailS]++;
			BaroRelAltitude = SlewLimit(BaroRelAltitudeP, BaroRelAltitude, BARO_SLEW_LIMIT_CM);
			BaroRelAltitudeP = BaroRelAltitude;
	
			F.NewBaroValue = true;
		}
	}
				
} // GetFreescaleBaroAltitude

boolean IsFreescaleBaroActive(void)
{ // check for Freescale Barometer
	boolean r;
	
	r = I2CResponse(ADS7823_ID);
	if ( r )
		BaroType = BaroMPX4115;

	return (r);

} // IsFreescaleBaroActive

void InitFreescaleBarometer(void)
{
	static int16 BaroOriginAltitude, MinAltitude;
	static int24 BaroPressureP;

	BaroTemperature = 0;
	BaroPressure =  0;

	BaroRetries = 0;
	do {
		BaroPressureP = BaroPressure;	
		SetFreescaleOffset();	
		Delay1mS(BARO_UPDATE_MS);
		ReadFreescaleBaro();
		BaroPressure = (int24)BaroVal.u16;
	} while ( ( ++BaroRetries < BARO_INIT_RETRIES ) 
			&& ( Abs( FreescaleToCm(BaroPressure - BaroPressureP ) ) > 20 ) );

	F.BaroAltitudeValid = BaroRetries < BARO_INIT_RETRIES;

	OriginBaroPressure = BaroPressure;
	BaroRelAltitude = BaroRelAltitudeP = 0;
	
	MinAltitude = FreescaleToCm((int24)ADS7823_MAX*4);
	BaroOriginAltitude = FreescaleToCm(OriginBaroPressure);
	BaroDescentAvailable = MinAltitude - BaroOriginAltitude;
	BaroClimbAvailable = -BaroOriginAltitude;

	//F.BaroAltitudeValid &= (( BaroClimbAvailable >= BARO_MIN_CLIMB ) 
		// && (BaroDescentAvailable <= BARO_MIN_DESCENT));

	#ifdef SIMULATE
	FakeBaroRelAltitude = 0;
	#endif // SIMULATE

} // InitFreescaleBarometer

// -----------------------------------------------------------

// Bosch SMD500 and BMP085 Barometers

void StartBoschBaroADC(boolean);
int24 BoschToCm(int24);
int24 CompensatedBoschPressure(uint16, uint16);

void GetBoschBaroAltitude(void);
boolean IsBoschBaroActive(void);
void InitBoschBarometer(void);

#define BOSCH_ID_BMP085		((uint8)(0x55))
#define BOSCH_TEMP_BMP085	0x2e
#define BOSCH_TEMP_SMD500	0x6e
#define BOSCH_PRESS			0xf4
#define BOSCH_CTL			0xf4				// OSRS=3 for BMP085 25.5mS, SMD500 34mS				
#define BOSCH_ADC_MSB		0xf6
#define BOSCH_ADC_LSB		0xf7
#define BOSCH_ADC_XLSB		0xf8				// BMP085
#define BOSCH_TYPE			0xd0

// SMD500 9.5mS (T) 34mS (P)  
// BMP085 4.5mS (T) 25.5mS (P) OSRS=3
#define BOSCH_TEMP_TIME_MS			11	// 10 increase to make P+T acq time ~50mS
//#define BMP085_PRESS_TIME_MS 		26
//#define SMD500_PRESS_TIME_MS 		34
#define BOSCH_PRESS_TIME_MS			38
#define BOSCH_PRESS_TEMP_TIME_MS	BARO_UPDATE_MS	// MUST BE 20Hz pressure and temp time + overheads 	

uint8 BaroPressureCycles;

void StartBoschBaroADC(boolean ReadPressure)
{
	static uint8 TempOrPress;

	if ( ReadPressure )
	{
		TempOrPress = BOSCH_PRESS;
		mS[BaroUpdate] = mSClock() + BARO_UPDATE_MS; // reading pressure every cycle and temp every 1Sec. BOSCH_PRESS_TIME_MS;
	}
	else
	{
		mS[BaroUpdate] = mSClock() + BOSCH_TEMP_TIME_MS;	
		if ( BaroType == BaroBMP085 )
			TempOrPress = BOSCH_TEMP_BMP085;
		else
			TempOrPress = BOSCH_TEMP_SMD500;
	}

	WriteI2CByteAtAddr(BOSCH_ID, BOSCH_CTL, TempOrPress); // select 32kHz input

} // StartBoschBaroADC

void ReadBoschBaro(void)
{
	static uint8 r;

	#define BOSCH_SPLIT_READ

	#ifdef BOSCH_SPLIT_READ
		// Possible I2C protocol error - split read of ADC
		BaroVal.b1 = ReadI2CByteAtAddr(BOSCH_ID, BOSCH_ADC_MSB);
		BaroVal.b0 = ReadI2CByteAtAddr(BOSCH_ID, BOSCH_ADC_LSB);
	#else
		F.BaroAltitudeValid = ReadI2Ci16v(BOSCH_ID, BOSCH_ADC_MSB, BaroVal.i16, 1);
	#endif // BOSCH_SPLIT_READ
} // ReadBoschBaro

int24 BoschToCm(int24 CP) {
	return(-((int24)CP * 980) / (int32)P[BaroScale]);
} // BoschToCm

#define BOSCH_BMP085_TEMP_COEFF		62L 	
#define BOSCH_SMD500_TEMP_COEFF		50L

int24 CompensatedBoschPressure(uint16 BaroPress, uint16 BaroTemp)
{
	static int24 BaroTempComp;

	if ( BaroType == BaroBMP085 )
		BaroTempComp = (BaroTemp * BOSCH_BMP085_TEMP_COEFF + 64L) >> 7;
	else
		BaroTempComp = (BaroTemp * BOSCH_SMD500_TEMP_COEFF + 8L) >> 4;
	
	return ((int24)BaroPress + BaroTempComp - OriginBaroPressure);

} // CompensatedBoschPressure

void GetBoschBaroAltitude(void)
{
	static int24 Temp;

	F.NewBaroValue = false;

	if ( mSClock() >= mS[BaroUpdate] )
	{
		ReadBoschBaro();
		if ( F.BaroAltitudeValid )
			if ( AcquiringPressure )
			{
				BaroPressure = (int24)BaroVal.u16;
				
				CompBaroPressure = CompensatedBoschPressure(BaroPressure, BaroTemperature);

				// decreasing pressure is increase in altitude negate and rescale to decimetre altitude
				BaroRelAltitude = BoschToCm(CompBaroPressure);

				if ( Abs( BaroRelAltitude - BaroRelAltitudeP ) > BARO_SANITY_CHECK_CM )
					Stats[BaroFailS]++;

				BaroRelAltitude = SlewLimit(BaroRelAltitudeP, BaroRelAltitude, BARO_SLEW_LIMIT_CM);
				BaroRelAltitudeP = BaroRelAltitude;
				if ( BaroPressureCycles-- <= 0 )
				{
					AcquiringPressure = false;
					BaroPressureCycles = 1000L/BARO_UPDATE_MS;
				}	
				F.NewBaroValue = true;
			}
			else
			{
				BaroTemperature = (int24)BaroVal.u16;			
				AcquiringPressure = true;			
			}
		else
			if ( State == InFlight )
			{
				AcquiringPressure = true;
				Stats[BaroFailS]++;
			}
		StartBoschBaroADC(AcquiringPressure);
	}			
} // GetBoschBaroAltitude

boolean IsBoschBaroActive(void)
{ // check for Bosch Barometers
	boolean r;

	r = I2CResponse(BOSCH_ID);

	if ( r )
		if (ReadI2CByteAtAddr(BOSCH_ID, BOSCH_TYPE) == BOSCH_ID_BMP085 )
			BaroType = BaroBMP085;
		else
			BaroType = BaroSMD500;

	return(r);

} // IsBoschBaroActive

void InitBoschBarometer(void)
{
	int24 Diff, CompBaroPressureP;
	int16 BaroStable;

	if ( BaroType == BaroBMP085 )
		BaroStable = 50;
	else
		BaroStable = 100;

	F.NewBaroValue = false;
	CompBaroPressure = 0;

	BaroRetries = 0;
	do // occasional I2C misread of Temperature so keep doing it until the Origin is stable!!
	{	
		CompBaroPressureP = CompBaroPressure;
		CompBaroPressure = 0;
		
		AcquiringPressure = true;
		StartBoschBaroADC(AcquiringPressure); // Pressure
	
		while ( mSClock() < mS[BaroUpdate] );
		ReadBoschBaro(); // Pressure	
		BaroPressure = BaroVal.u16;
			
		AcquiringPressure = !AcquiringPressure;
		StartBoschBaroADC(AcquiringPressure); // Temperature
		while ( mSClock() < mS[BaroUpdate] );
		ReadBoschBaro();
		BaroTemperature = BaroVal.u16;
		
		CompBaroPressure = CompensatedBoschPressure(BaroPressure, BaroTemperature);
	
		AcquiringPressure = !AcquiringPressure;
		StartBoschBaroADC(AcquiringPressure); 	

	} while ((++BaroRetries<BARO_INIT_RETRIES)&&(Abs(BoschToCm(CompBaroPressure-CompBaroPressureP))>BaroStable));
	
	OriginBaroPressure = CompBaroPressure;
	F.BaroAltitudeValid = BaroRetries < BARO_INIT_RETRIES;		
	BaroRelAltitude = BaroRelAltitudeP = 0;

	#ifdef SIMULATE
		FakeBaroRelAltitude = 0;
	#endif // SIMULATE

} // InitBoschBarometer

// -----------------------------------------------------------


int24 AltitudeCF(int24 Alt) 
{	// Complementary Filter originally authored by RoyLB for attitude estimation
	// http://www.rcgroups.com/forums/showpost.php?p=12082524&postcount=1286
	// adapted for baro compensation by G.K. Egan 2011 

	const int16 BaroAccScale = 2;
	const int16 TauCF = 7; // 5 FOR BMP085 increasing value reduces filtering

	static i32u Temp;
	static int32 AltD;
	static int24 AltCFp;	

	AltD = Alt - AltCF;
	AltCFp = AltCF;

    AltF[0] = AltD * Sqr(TauCF);
   	Temp.i32 = AltF[2] * 256 + AltF[0];
	AltF[2] = Temp.i3_1;

  	AltF[1] = AltF[2] + AltD * 2 * TauCF; // ABANDON ACC TOO NOISY  -(Acc[DU] - 1024) * BaroAccScale; 
 	Temp.i32 = AltCF * 256 + AltF[1];
	AltCF = Temp.i3_1;

	BaroROC = ( AltCF - AltCFp) * ALT_UPDATE_HZ; 
	BaroROC = MediumFilter(BaroROCp, BaroROC);
	BaroROC = Limit1(BaroROC, 800);
	// BaroROC = HardFilter(BaroROCp, BaroROC);
	BaroROCp = BaroROC;

	AccAltComp = AltCF - Alt; // for debugging

	return( AltCF ); 
} // AltitudeCF

const rom char * BaroName[BaroUnknown+1] = {
		"BMP085","SMD500","MPX4115","None"
		};		

void ShowBaroType(void)
{
	TxString(BaroName[BaroType]);
} // ShowBaroType

#ifdef TESTING

void BaroTest(void)
{
	TxString("\r\nAltitude test\r\n");

	TxString("Type:\t"); ShowBaroType();
	
	TxString("\r\nInit Retries:\t");
	TxVal32((int32)BaroRetries - 2, 0, ' '); // alway minimum of 2
	if ( BaroRetries >= BARO_INIT_RETRIES )
		TxString(" FAILED Init.\r\n");
	else
		TxNextLine();

	if ( BaroType == BaroMPX4115 )
	{
		TxString("Range   :\t");		
		TxVal32((int32) BaroDescentAvailable, 2, ' ');
		TxString("-> ");
		TxVal32((int32) BaroClimbAvailable, 2, 'M');

		TxString(" {Offset ");TxVal32((int32)BaroOffsetDAC, 0,'}'); 
		if (( BaroClimbAvailable < BARO_MIN_CLIMB ) || (BaroDescentAvailable > BARO_MIN_DESCENT))
			TxString(" Bad climb or descent range - offset adjustment?");
		TxNextLine();
	}

	if ( F.BaroAltitudeValid ) 
	{	
		F.NewBaroValue = false;
		while ( !F.NewBaroValue )
			GetBaroAltitude();	
		F.NewBaroValue = false;
	
		TxString("Alt.:     \t");
		TxVal32((int32)BaroRelAltitude, 2, ' ');
		TxString("M\r\n");
	
		TxString("\r\nR.Finder: \t");
		if ( F.RangefinderAltitudeValid )
		{
			GetRangefinderAltitude();
			TxVal32((int32)RangefinderAltitude, 2, ' ');
			TxString("M\r\n");
		}
		else
			TxString("no rangefinder\r\n");
		
		#ifdef INC_TEMPERATURE
		TxString("\r\nAmbient :\t");
		TxVal32((int32)AmbientTemperature.i16, 1, ' ');
		TxString("C\r\n");
		#endif // INC_TEMPERATURE
	}
	else
		TxString("FAIL\r\n");
} // BaroTest

#endif // TESTING

void GetBaroAltitude(void)
{
	static int24 Temp;

	#ifdef SIMULATE

		if ( mSClock() >= mS[BaroUpdate])
		{
			mS[BaroUpdate] = mSClock() + BARO_UPDATE_MS;

			if ( State == InFlight )
				FakeBaroRelAltitude += (DesiredThrottle - CruiseThrottle + AltComp * 2 );
			else
				FakeBaroRelAltitude = DecayX(FakeBaroRelAltitude, 5);

			if ( FakeBaroRelAltitude < -50 ) 
				FakeBaroRelAltitude = 0;

			BaroRelAltitude = AltitudeCF(FakeBaroRelAltitude);

			F.NewBaroValue = true;
		}

	#else

		if ( BaroType == BaroMPX4115 )
			GetFreescaleBaroAltitude();
		else
			GetBoschBaroAltitude();
	
		if ( F.NewBaroValue )
		{
			BaroRelAltitude = AltitudeCF(BaroRelAltitude);	
	
			if ( State == InFlight ) 
			{
				if ( BaroRelAltitude > Stats[BaroRelAltitudeS] )
					Stats[BaroRelAltitudeS] = BaroRelAltitude;
	
				if ( BaroROC > Stats[MaxROCS] )
						Stats[MaxROCS] = BaroROC;
				else
					if ( BaroROC < Stats[MinROCS] )
						Stats[MinROCS] = BaroROC;
			}
		}

	#endif // SIMULATE
	
} // GetBaroAltitude

void InitBarometer(void)
{
	BaroRelAltitude = CompBaroPressure = OriginBaroPressure = SimulateCycles = BaroROC = BaroROCp = 0;
	BaroType = BaroUnknown;
	BaroPressureCycles = 0;
	AltCF = AltF[0] = AltF[1] = AltF[2] = 0;

	F.BaroAltitudeValid = true; // optimistic

	if ( IsFreescaleBaroActive() )	
		InitFreescaleBarometer();
	else
		if ( IsBoschBaroActive() )
			InitBoschBarometer();
		else	
		{
			F.BaroAltitudeValid = F.HoldingAlt = false;
			Stats[BaroFailS]++;
		}
} // InitBarometer



