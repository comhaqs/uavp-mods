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

int24 BaroToCm(void);
void InitI2CBarometer(void);
void StartI2CBaroADC(boolean);
void GetI2CBaroAltitude(void);

#define MCP4725_MAX	 	4095 	// 12 bits

#define ADS7823_TIME_MS	50		// 20Hz
#define ADS7823_MAX	 	4095 	// 12 bits
#define ADS7823_WR		0x90 	// ADS7823 ADC
#define ADS7823_RD		0x91 	// ADS7823 ADC
#define ADS7823_CMD		0x00

#define MCP4725_WR		MCP4725_ID

#define MCP4725_RD		(MCP4725_ID+1)
#define MCP4725_CMD		0x40 	// write to DAC registor in next 2 bytes
#define MCP4725_EPROM	0x60    // write to DAC registor and eprom

void SetFreescaleMCP4725(int16);
void SetFreescaleOffset(void);
void ReadFreescaleBaro(void);
void GetFreescaleBaroAltitude(void);
boolean IsFreescaleBaroActive(void);
void InitFreescaleBarometer(void);

#define MS5611_TEMP_TIME_MS	10 
 
#define MS5611_PRESS    0x40
#define MS5611_TEMP 	0x50
#define MS5611_RESET    0x1E

// OSR (Over Sampling Ratio) constants
#define MS5611_OSR_256  0x00	//0.065 mBar
#define MS5611_OSR_512  0x02	//0.042
#define MS5611_OSR_1024 0x04	//0.027
#define MS5611_OSR_2048 0x06	//0.018
#define MS5611_OSR_4096 0x08	//0.012

#define OSR MS5611_OSR_4096

void ReadMS5611Baro(void);
boolean IsMS5611BaroActive(void);

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

#define BOSCH_BMP085_TEMP_COEFF		62L 	
#define BOSCH_SMD500_TEMP_COEFF		50L 	

void ReadBoschBaro(void);
boolean IsBoschBaroActive(void);

uint16 MS5611Constants[6];

uint32	BaroPressure, BaroTemperature;
boolean AcquiringPressure;
int16	BaroOffsetDAC;

#define BARO_MIN_CLIMB			1500	// dM minimum available barometer climb from origin
#define BARO_MIN_DESCENT		-500	// dM minimum available barometer descent from origin

const 	uint8 BaroError[(BaroUnknown+1)] = { 50, 100, 50, 20, 0 };// BaroBMP085, BaroSMD500, BaroMPX4115, BaroMS5611, BaroUnknown
uint8 	BaroStable;
int32	OriginBaroTemperature, OriginBaroPressure;
int24	BaroRelAltitude, BaroRelAltitudeP;
i24u	BaroVal;
uint8	BaroType;
int16 	BaroClimbAvailable, BaroDescentAvailable;
int16	BaroRetries;

int24	FakeBaroRelAltitude;
int8 	SimulateCycles;

uint8 	BaroPressureCycles;

int24 AltCF;
int16 TauCF = 7; // 5 FOR BMP085 increasing value reduces filtering
int32 AltF[3] = { 0, 0, 0};

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
		"BMP085","SMD500","MPX4115","MS5611", "None/unsupported"
		};		

void ShowBaroType(void)
{
	TxString(BaroName[BaroType]);
} // ShowBaroType

#ifdef TESTING

void BaroTest(void)
{
	TxString("\r\nAltitude test - ");

	ShowBaroType();
	
	TxString("\r\nInit Retries:\t");
	TxVal32(BaroRetries - 2, 0, ' '); // alway minimum of 2
	if ( BaroRetries >= BARO_INIT_RETRIES )
		TxString(" FAILED Init.\r\n");
	else
		TxNextLine();

	switch ( BaroType ) {
	#ifdef INC_MPX4115
	case BaroMPX4115:
		TxString("Range   :\t");		
		TxVal32((int32) BaroDescentAvailable, 2, ' ');
		TxString("-> ");
		TxVal32((int32) BaroClimbAvailable, 2, 'M');

		TxString(" {Offset ");TxVal32((int32)BaroOffsetDAC, 0,'}'); 
		if (( BaroClimbAvailable < BARO_MIN_CLIMB ) || (BaroDescentAvailable > BARO_MIN_DESCENT))
			TxString(" Bad climb or descent range - offset adjustment?");
		TxNextLine();
		break;
	#endif // INC_MPX4115
	#ifdef INC_MS5611
	case BaroMS5611:

		break;
	#endif // INC_MS5611
	default:
		break;
	} // switch

	if ( F.BaroAltitudeValid ) 
	{	
		F.NewBaroValue = false;
		while ( !F.NewBaroValue )
			GetBaroAltitude();	
		F.NewBaroValue = false;

		#ifdef FULL_BARO_TEST

			TxString("Raw P/T: \t");
			TxVal32(BaroPressure, 0, 0);
			TxString(" (");
			TxVal32(BaroPressure - OriginBaroPressure, 0, 0);
			TxString(") ");
			TxVal32(BaroTemperature, 0, 0); 
			TxString(" (");
			TxVal32(BaroTemperature - OriginBaroTemperature, 0, 0);
			TxString(")\r\n");
	
		#endif // FULL_BARO_TEST
	
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

} // BaroTest

#endif // TESTING

void GetBaroAltitude(void)
{
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

		#ifdef INC_MPX4115
		if ( BaroType == BaroMPX4115 )
			GetFreescaleBaroAltitude();
		else
		#endif // INC_MPX4115
			GetI2CBaroAltitude();
	
		if ( F.NewBaroValue )
		{
			BaroRelAltitude = AltitudeCF(BaroRelAltitude);	
	
			if ( State == InFlight ) 
			{
				StatsMax(BaroRelAltitude, BaroRelAltitudeS);	
				StatsMinMax(BaroROC, MinROCS, MaxROCS);
			}
		}

	#endif // SIMULATE
	
} // GetBaroAltitude

void InitBarometer(void)
{
	static uint8 i;
	static int32 Alt;

	BaroRelAltitude = OriginBaroPressure = SimulateCycles = BaroROC = BaroROCp = 0;
	BaroType = BaroUnknown;

	BaroStable = BaroError[BaroType];
	BaroPressureCycles = 0;
	AltCF = AltF[0] = AltF[1] = AltF[2] = 0;

	F.BaroAltitudeValid = true; // optimistic

	#ifdef INC_MPX4115
	if ( IsFreescaleBaroActive() )	
		InitFreescaleBarometer();
	else
	#endif // INC_MPX4115
	if ( IsBoschBaroActive() )
		InitI2CBarometer();
	else
		#ifdef INC_MS5611
		if ( IsMS5611BaroActive() )
		{
			WriteI2CByteAtAddr(MS5611_ID, MS5611_RESET, 0);
			InitI2CBarometer();
		}
		else
		#endif // INC_MS5611	
		{
			F.BaroAltitudeValid = F.HoldingAlt = false;
			Stats[BaroFailS]++;
		}

	for ( i = 0; i <(uint8)200; i++) // initialise CF
		Alt = AltitudeCF(BaroRelAltitude);

} // InitBarometer

// -----------------------------------------------------------

// Generic I2C Baro

void BaroFail(void)
{
	F.HoldingAlt = false;
	if ( State == InFlight ) 
	{
		Stats[BaroFailS]++; 
		F.BaroFailure = true;
	}
} // BaroFail

void StartI2CBaroADC(boolean ReadPressure)
{
	static uint8 TempOrPress;

	switch ( BaroType ) {
	case BaroMS5611:
		if ( ReadPressure )
		{
			mS[BaroUpdate] = mSClock() + BARO_UPDATE_MS; 
			WriteI2CByteAtAddr(MS5611_ID, 0, MS5611_PRESS | OSR);
		}
		else
		{
			mS[BaroUpdate] = mSClock() + MS5611_TEMP_TIME_MS;	
			WriteI2CByteAtAddr(MS5611_ID, 0, MS5611_TEMP | OSR);
		}
		break;
	case BaroSMD500:
	case BaroBMP085:
		if ( ReadPressure )
		{
			TempOrPress = BOSCH_PRESS;
			mS[BaroUpdate] = mSClock() + BARO_UPDATE_MS; 
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
		break;
	default:
		break;
	} // switch	

} // StartI2CBaroADC

void ReadI2CBaro(void)
{
	#ifdef INC_MS5611
	if ( BaroType == BaroMS5611 )
		ReadMS5611Baro();
	else
	#endif // INC_MS5611
		ReadBoschBaro();

} // ReadI2CBaro

void GetI2CBaroAltitude(void)
{
	static int24 Temp;

	F.NewBaroValue = false;
	if ( mSClock() >= mS[BaroUpdate] )
	{
 		ReadI2CBaro();
		if ( F.BaroAltitudeValid )
			if ( AcquiringPressure )
			{
				BaroPressure = (int24)BaroVal.u24;

				// decreasing pressure is increase in altitude negate and rescale to decimetre altitude
				BaroRelAltitude = BaroToCm();

				if ( Abs( BaroRelAltitude - BaroRelAltitudeP ) > BARO_SANITY_CHECK_CM )
					Stats[BaroFailS]++;

				BaroRelAltitude = SlewLimit(BaroRelAltitudeP, BaroRelAltitude, BARO_SLEW_LIMIT_CM);
				BaroRelAltitudeP = BaroRelAltitude;
				if ( BaroPressureCycles-- <= (uint8)0 )
				{
					AcquiringPressure = false;
					BaroPressureCycles = 1000L/BARO_UPDATE_MS;
				}	
				F.NewBaroValue = true;
			}
			else
			{
				BaroTemperature = (int24)BaroVal.u24;			
				AcquiringPressure = true;			
			}
		else
		{
			AcquiringPressure = true;
			BaroFail();
		}
		StartI2CBaroADC(AcquiringPressure);
	}			
} // GetI2CBaroAltitude

int24 BaroToCm(void)
{
	// These calculations are intended to produce approximate altitudes
	// within the code and performance limits of the processor  
	static int32 A, Td, Pd;

	Td = (int32)BaroTemperature - OriginBaroTemperature;
	Pd = (int32)BaroPressure - OriginBaroPressure;

	switch ( BaroType ) {
	case BaroBMP085:
		A = Td * (-14); // 14.07
		A += Pd * (-25); // 24.95 
		break;
	case BaroSMD500:
		A = Td * (-97); // 97.45
		A += Pd * (-31); // 31.34 
		break;
	#ifdef INC_MS5611
	case BaroMS5611:
		//	A = Td * (-0.05435); 
		//	A += Pd * (-0.15966); 
		A = Td / (-18); // 18.399
		A += Pd / (-6); // 6.263
		break;
	#endif // INC_MS5611
	#ifdef INC_MPX4115
	case BaroMPX4115:
		A = -(Pd * 80)/56; // internally temperature compensated
		break;
	#endif // INC_MPX4115
	default:
		break;
	} // switch
	
	return ( A );

} // BaroToCm

void InitI2CBarometer(void)
{
	F.NewBaroValue = false;
	OriginBaroPressure = OriginBaroTemperature = BaroRetries = 0;
	F.NewBaroValue = false;

	while ( mSClock() < mS[BaroUpdate] );
	AcquiringPressure = true;
	StartI2CBaroADC(AcquiringPressure); // Pressure
		
	do // wait until we get a sensible launch altitude!!
	{		
		while ( mSClock() < mS[BaroUpdate] );
		ReadBoschBaro(); // Pressure	
		BaroPressure = BaroVal.u24;
			
		AcquiringPressure = !AcquiringPressure;
		StartI2CBaroADC(AcquiringPressure); // Temperature

		while ( mSClock() < mS[BaroUpdate] );
		ReadBoschBaro();
		BaroTemperature = BaroVal.u24;
	
		AcquiringPressure = !AcquiringPressure;
		StartI2CBaroADC(AcquiringPressure); 

		BaroRelAltitude = BaroToCm();	

		OriginBaroTemperature = BaroTemperature;
		OriginBaroPressure = BaroPressure;

	} while ((++BaroRetries<BARO_INIT_RETRIES)&&(Abs(BaroRelAltitude)>BaroStable));
	
	F.BaroAltitudeValid = BaroRetries < BARO_INIT_RETRIES;		
	BaroRelAltitudeP = BaroRelAltitude;

	#ifdef SIMULATE
		FakeBaroRelAltitude = 0;
	#endif // SIMULATE

} // InitI2CBarometer


// -----------------------------------------------------------

#ifdef INC_MPX4115

// Freescale ex Motorola MPX4115 Barometer with ADS7823 12bit ADC

void SetFreescaleMCP4725(int16 d)
{	
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
	while ( (BaroVal.u24 < (uint16)(((uint24)ADS7823_MAX*4L*7L)/10L) ) 
		&& (BaroOffsetDAC > 20) )				// first loop gets close
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

	while( (BaroVal.u24 < (uint16)(((uint24)ADS7823_MAX*4L*3L)/4L) ) && (BaroOffsetDAC > 2) )
	{
		BaroOffsetDAC -= 2;
		SetFreescaleMCP4725(BaroOffsetDAC);
		Delay1mS(10);
		ReadFreescaleBaro();
		LEDYellow_TOG;
	}

	OriginBaroPressure = BaroVal.u24;
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
		BaroVal.u24 = B[0] + B[1] + B[2] + B[3];
		BaroVal.u24 = (uint24)16380 - BaroVal.u24; // inverting op-amp
	}
	else
		BaroFail();

} // ReadFreescaleBaro

void GetFreescaleBaroAltitude(void)
{
	if ( mSClock() >= mS[BaroUpdate] ) // could run faster with MPX
	{
		ReadFreescaleBaro();
		if ( F.BaroAltitudeValid )
		{
			BaroPressure = (int24)BaroVal.u24; // sum of 4 samples		
			BaroRelAltitude = BaroToCm();
	
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
	static boolean r;
	
	r = I2CResponse(ADS7823_ID);
	if ( r )
		BaroType = BaroMPX4115;

	return (r);

} // IsFreescaleBaroActive

void InitFreescaleBarometer(void)
{
	static int16 MinAltitude;
	static int32 BaroPressureP;

	BaroTemperature = OriginBaroTemperature = BaroPressure =  0;

	BaroPressure = 0;
	BaroRetries = 0;
	do {
		BaroPressureP = BaroPressure;	
		SetFreescaleOffset();	
		Delay1mS(BARO_UPDATE_MS);
		ReadFreescaleBaro();		
		BaroPressure = (int24)BaroVal.u24;

		BaroRelAltitude = BaroToCm();

	} while ( ( ++BaroRetries < BARO_INIT_RETRIES ) 
			&& ( Abs( BaroPressure - BaroPressureP ) > BaroStable ) );

	F.BaroAltitudeValid = BaroRetries < BARO_INIT_RETRIES;

	BaroRelAltitude = BaroRelAltitudeP = 0;
	
	BaroPressure = (int24)ADS7823_MAX*4;
	MinAltitude = BaroToCm();
	BaroDescentAvailable = MinAltitude;
	BaroPressure = OriginBaroPressure * 2; // cancel out origin offset in altitude calculation
	BaroClimbAvailable = -BaroToCm();

	//F.BaroAltitudeValid &= (( BaroClimbAvailable >= BARO_MIN_CLIMB ) 
		// && (BaroDescentAvailable <= BARO_MIN_DESCENT));

	#ifdef SIMULATE
	FakeBaroRelAltitude = 0;
	#endif // SIMULATE

} // InitFreescaleBarometer

#endif // INC_MPX4115

// -----------------------------------------------------------

#ifdef INC_MS5611

// Measurement Specialities MS5611 Barometer

void ReadMS5611Baro(void)
{
	I2CStart();
		WriteI2CByte(MS5611_ID);
		WriteI2CByte(0);
	I2CStart();	
		BaroVal.b2 = ReadI2CByte(I2C_ACK);
		BaroVal.b1 = ReadI2CByte(I2C_ACK);
		BaroVal.b0 = ReadI2CByte(I2C_NACK);
	I2CStop();
} // ReadMS5611Baro

boolean IsMS5611BaroActive(void)
{ // check for MS Barometer
	boolean r;
	
	r = I2CResponse(MS5611_ID);
	if ( r )
		BaroType = BaroMS5611;
	
	return (r);

} // IsMS5611BaroActive

#endif // INC_MS5611

// -----------------------------------------------------------

// Bosch SMD500 and BMP085 Barometers


void ReadBoschBaro(void)
{
	// Possible I2C protocol error - split read of ADC
	BaroVal.b2 = 0;
	BaroVal.b1 = ReadI2CByteAtAddr(BOSCH_ID, BOSCH_ADC_MSB);
	BaroVal.b0 = ReadI2CByteAtAddr(BOSCH_ID, BOSCH_ADC_LSB);

} // ReadBoschBaro

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








