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

// Compass 100KHz I2C

#include "uavx.h"

void ShowCompassType(void);
int16 GetCompass(void);
void GetHeading(void);
int16 MinimumTurn(int16);
void GetCompassParameters(void);
void DoCompassTest(void);
void CalibrateCompass(void);
void InitHeading(void);
void InitCompass(void);

int16 GetHMC58X3Magentometer(void);
void DoTestHMC58X3Magnetometer(void);
void CalibrateHMC58X3Magnetometer(void);
void InitHMC58X3Magnetometer(void);
boolean HMC58X3CompassActive(void);

int16 GetHMC6352Compass(void);
void DoTestHMC6352Compass(void);
void CalibrateHMC6352Compass(void);
void InitHMC6352Compass(void);
boolean HMC6352CompassActive(void);

i24u 	Compass;
int16 	MagHeading, Heading, HeadingP, DesiredHeading, CompassOffset;
int8 	CompassType;
int8 	MagRetries;

const rom char * CompassName[CompassUnknown+1] = {
		"HMC58X3","HMC6352","None"
		};

void ShowCompassType(void)
{
	TxString(CompassName[CompassType]);
} // ShowCompassType		

int16 GetCompass()
{
	if ( CompassType == HMC58X3Magnetometer )
		return (GetHMC58X3Magnetometer());
	else
		#ifdef INC_HMC6352
		if ( CompassType == HMC6352Compass )
			return (GetHMC6352Compass());
		else
		#endif // INC_HMC6352
			return(0);	
} // GetCompass
 
void GetHeading(void)
{
	static int16 NewHeading, HeadingChange, Temp;

	if ( SpareSlotTime && F.NormalFlightMode && ( mSClock() >= mS[CompassUpdate] ))
	{
		SpareSlotTime = false;
		mS[CompassUpdate] = mSClock() + COMPASS_TIME_MS;

		#ifdef SIMULATE

			if ( State == InFlight )
			{
				#ifdef  NAV_WING
					Temp = SRS16( A[Yaw].FakeDesired - A[Roll].FakeDesired, 1);	
					if ( Abs(A[Yaw].FakeDesired - A[Roll].FakeDesired) > 5 )
						FakeMagHeading -= Limit1(Temp, NAV_MAX_FAKE_COMPASS_SLEW);
				#else
					Temp = A[Yaw].FakeDesired * 5; // ~90 deg/sec
					if ( Abs(A[Yaw].FakeDesired) > 5 )
						FakeMagHeading -= Limit1(Temp, NAV_MAX_FAKE_COMPASS_SLEW);				
				#endif // NAV_WING
				
				FakeMagHeading = Make2Pi((int16)FakeMagHeading);
			}

			MagHeading = FakeMagHeading;

		#else

			if( F.CompassValid ) // continuous mode but Compass only updates avery 50mS
				MagHeading = GetCompass();
			else
				MagHeading = 0;

		#endif // SIMULATE

		Heading = Make2Pi(MagHeading - CompassOffset);

		if ( A[Yaw].Hold <= COMPASS_MIDDLE )
		{	
			HeadingChange = Abs( Heading - HeadingP );
			if ( HeadingChange < MILLIPI )		
			{
				if (( HeadingChange > COMPASS_MAX_SLEW ) && ( State == InFlight )) 
				     Heading = SlewLimit(HeadingP, Heading, COMPASS_MAX_SLEW);    
				Heading = HeadingFilter(HeadingP, Heading);
				Heading = Make2Pi(Heading);
			}
		}
		HeadingP = Heading;
	}	

} // GetHeading

int16 MinimumTurn(int16 A ) {

    static int16 AbsA;

    AbsA = Abs(A);
    if ( AbsA > MILLIPI )
        A = ( AbsA - TWOMILLIPI ) * Sign(A);

    return ( A );

} // MinimumTurn

void InitHeading(void)
{
	MagHeading = GetCompass();
	Heading = HeadingP = Make2Pi( MagHeading - CompassOffset );

	#ifdef SIMULATE
		FakeMagHeading = Heading = 0;
	#endif // SIMULATE
	DesiredHeading = Heading;

} // InitHeading

void InitCompass(void)
{
	if ( HMC58X3MagnetometerActive() )
	{
		CompassType = HMC58X3Magnetometer;
		InitHMC58X3Magnetometer();
	}
	else
		#ifdef INC_HMC6352
		if ( HMC6352CompassActive() )
		{
			CompassType = HMC6352Compass;
			InitHMC6352Compass();
		}
		else
		#endif // INC_HMC6352
			CompassType = CompassUnknown;

} // InitCompass

#ifdef TESTING 

void DoCompassTest(void)
{
	TxString("\r\nCompass test (");
	
	if ( CompassType == HMC58X3Magnetometer )
		DoTestHMC58X3Magnetometer();
	else
		#ifdef INC_HMC6352
		if ( CompassType == HMC6352Compass )
			DoTestHMC6352Compass();
		else
		#endif // INC_HMC6352
			TxString("not installed?)\r\n");
} // DoCompassTest

void CalibrateCompass(void)
{
	if ( CompassType == HMC58X3Magnetometer )
		CalibrateHMC58X3Magnetometer();
	#ifdef INC_HMC6352
	else
		if ( CompassType == HMC6352Compass )
			CalibrateHMC6352Compass();
	#endif // INC_HMC6352
} // CalibrateCompass

#endif // TESTING
//________________________________________________________________________________________

// HMC58X3 3 Axis Magnetometer

MagStruct Mag[3];
uint8 HMC58X3_ID;

#define HMC58X3_CONFIG_A	0x00
#define HMC58X3_CONFIG_B	0x01
#define HMC58X3_MODE		0x02
#define HMC58X3_DATA		0x03
#define HMC58X3_STATUS		0x09

int16 GetHMC58X3Magnetometer(void) {

	static int16 b[3];
	static i32u Temp;
    static uint8 a, r;
    static int16 mx, my;
    static int16 MRoll, MPitch, CRoll, SRoll, CPitch, SPitch;
	static int16 CompassVal;
	static MagStruct * M;

	F.CompassValid = ReadI2Ci16v(HMC58X3_ID, HMC58X3_DATA, b, 3, true);

	if( F.CompassValid )//&& !((b[X]==b[Y])&&(b[Y]==b[Z]))) 
	{
		for ( a = X; a<=(uint8)Z; a++ )
		{
			M = &Mag[a];
			M->G = b[a];
			M->Min = Min(M->Min, b[a]);
			M->Max = Max(M->Max, b[a]);	
			M->G -= SRS16(M->Max + M->Min, 1);
		}
	
		// Compass X right, Y forward, Z up
		#ifdef HMC58X3_FULL

			switch ( P[SensorHint] ) { // pitch up and roll LEFT!
			case FreeIMU:
				MRoll = A[Roll].Angle;
				MPitch = A[Pitch].Angle;
				break;
			case Drotek:
				MRoll = A[Roll].Angle;
				MPitch = -A[Pitch].Angle;
				break;
			default: // SF Magnetometer sub for HMC6352
				MRoll = A[Roll].Angle;
				MPitch = A[Pitch].Angle;
				break;
			} // switch	

			MRoll = SRS16(MRoll, 2); // internal angles to milliradian 78*180/3142 = 4.46
			MPitch = SRS16(MPitch, 2); 

		    CRoll = int16cos(MRoll);
		    SRoll = int16sin(MRoll);
		    CPitch = int16cos(MPitch);
		    SPitch = int16sin(MPitch);	
	
			// Tilt compensated Magnetic field X:
			Temp.i32 = (int32)Mag[Y].G * SPitch * SRoll + (int32)Mag[Z].G * CPitch * SRoll;
			Temp.i32 = (int24)Mag[X].G * CRoll + Temp.i3_1;
			mx = Temp.i3_1;
			
			// Tilt compensated Magnetic field Y:
			Temp.i32 =  (int24)Mag[Y].G * CPitch - (int24)Mag[Z].G * SPitch;
			my = Temp.i3_1;

		    // Magnetic Heading
		    CompassVal = int32atan2( my, mx );
	
		#else
	
	    	CompassVal = int32atan2( Mag[Y].G, Mag[X].G );
	
		#endif // HMC58X3_FULL
	}
	else
		Stats[CompassFailS]++;

	return ( CompassVal );

} // GetHMC58X3Magnetometer

void WriteMagCalEE(void)
{
	static uint8 a;
	static MagStruct * M;

	if ( CompassType == HMC58X3Magnetometer )
		for ( a = X; a<=(uint8)Z; a++)
		{
			M = &Mag[a];
			Write16EE(MAG_BIAS_ADDR_EE + (a*4), M->Min);
			Write16EE(MAG_BIAS_ADDR_EE + (a*4+2), M->Max);
		}
} // WriteMagCalEE

void ReadMagCalEE(void)
{
	static uint8 a;
	static MagStruct * M;

	if ( CompassType == HMC58X3Magnetometer )
		for ( a = X; a<=(uint8)Z; a++)
		{
			M = &Mag[a];
			M->Min = Read16EE(MAG_BIAS_ADDR_EE + (a*4));
			M->Max = Read16EE(MAG_BIAS_ADDR_EE + (a*4+2));
		}
} // ReadMagCalEE

#ifdef TESTING

void CalibrateHMC58X3Magnetometer(void) {

	static uint8 a;
	static int16 b[3];
	static boolean r;
	static MagStruct * M;

	TxString("\r\n");
	ShowCompassType();
	TxString(" - Reset Bias\r\n");
	for ( a = X; a<=(uint8)Z; a++)
	{
		M = &Mag[a];
		M->Max = 850;
		M->Min = -850;
	}
		
	WriteMagCalEE();

	DoTestHMC58X3Magnetometer();
				
} // CalibrateHMC58X3Magnetometer

void DoTestHMC58X3Magnetometer(void) 
{
	static int32 Temp;
	static uint8 a, i, status;
	static MagStruct * M;

	ShowCompassType();

	status = ReadI2CByteAtAddr(HMC58X3_ID,HMC58X3_STATUS);
	MagHeading = GetCompass();
	Heading = Make2Pi( MagHeading - CompassOffset );

	if ( F.CompassValid )
	{
		TxString("\r\n\r\nStatus:\t0x");
		TxValH(status);
		TxString("\r\nRetries:\t");
		TxVal32(MagRetries - 1 ,0,0);
		
	    TxString("\r\n\t\tMag \tMin \tMax \tBias \tRef.\r\n");
		for ( a = X; a<=(uint8)Z; a++ )
		{
			M = &Mag[a];
			TxChar(HT);
			TxChar(a+'X');
			TxString(":\t");
		    TxVal32(M->G, 0, HT);
			TxVal32(M->Min, 0, HT);
			TxVal32(M->Max, 0, HT);
			TxVal32(SRS16(M->Max + M->Min, 1), 0, HT);
			TxVal32(M->Scale, 0, HT);
		    TxNextLine();
		}
	
	    TxVal32(ConvertMPiToDDeg(MagHeading), 1, 0);
	    TxString(" deg (Compass)\r\n");
	    TxVal32(ConvertMPiToDDeg(Heading), 1, 0);
	    TxString(" deg (True)\r\n");
	}
	else
		TxString(" Fail)\r\n");

} // DoHMC58X3Test

#endif // TESTING

void InitHMC58X3Magnetometer(void) 
{
	static uint8 a;
	static MagStruct * M;
	static int16 C[3];
	static boolean r;
	
	MagRetries = 0;

	do {

		Delay1mS(100);
	
		WriteI2CByteAtAddr(HMC58X3_ID, HMC58X3_CONFIG_A, 0b00010001); // 10Hz normal mode
		WriteI2CByteAtAddr(HMC58X3_ID, HMC58X3_MODE, 0b00010101); 
	
		Delay1mS(10);
	
		r = ReadI2Ci16v(HMC58X3_ID, HMC58X3_DATA, C, 3, true);
	
		for ( a = X; a<=(uint8)Z; a++)
			Mag[a].Scale = C[a];
	
		Delay1mS(10);
	
		WriteI2CByteAtAddr(HMC58X3_ID, HMC58X3_CONFIG_A, 0b00010100); // 20Hz normal mode
		WriteI2CByteAtAddr(HMC58X3_ID, HMC58X3_CONFIG_B, 0b00100000); // default gain
		WriteI2CByteAtAddr(HMC58X3_ID, HMC58X3_MODE, 0x00); // Set continuous mode (default to 10Hz)

		Delay1mS(100);

	} while ( (++MagRetries < MAG_INIT_RETRIES) && (C[X] == C[Y]) && (C[Y] == C[Z]) );

	F.CompassValid = MagRetries < MAG_INIT_RETRIES;

	mS[CompassUpdate] = mSClock();

	ReadMagCalEE();

} // InitHMC58X3Magnetometer

boolean HMC58X3MagnetometerActive(void) 
{
	HMC58X3_ID = HMC58X3_3DOF;

	F.CompassValid = I2CResponse(HMC58X3_ID);
	if ( !F.CompassValid )
	{
		HMC58X3_ID = HMC58X3_9DOF;
	  	F.CompassValid = I2CResponse(HMC58X3_ID);
	}

	return (F.CompassValid);
} //  HMC58X3MagnetometerActive

//________________________________________________________________________________________

#ifdef INC_HMC6352

// HMC6352 Bosch Compass

void WriteHMC6352Command(uint8 c)
{
	UseI2C100KHz = true;
	I2CStart(); // Do Bridge Offset Set/Reset now
		WriteI2CByte(HMC6352_ID);
		WriteI2CByte(c);
	I2CStop();
	UseI2C100KHz = false;
} // WriteHMC6352Command

int16 GetHMC6352Compass(void)
{
	static i16u CompassVal;

	UseI2C100KHz = true;
	I2CStart();
		F.CompassMissRead = WriteI2CByte(HMC6352_ID+1) != I2C_ACK; 
		CompassVal.b1 = ReadI2CByte(I2C_ACK);
		CompassVal.b0 = ReadI2CByte(I2C_NACK);
	I2CStop();
	UseI2C100KHz = false;

	return ( ConvertDDegToMPi( CompassVal.i16 ) );
} // GetHMC6352Compass

static uint8 CP[9];

#ifdef TESTING

#define TEST_COMP_OPMODE 0b01110000	// standby mode to reliably read EEPROM

void GetHMC6352Parameters(void)
{
	#ifdef FULL_TEST

	static uint8 r;

	UseI2C100KHz = true;
	I2CStart();
		WriteI2CByte(HMC6352_ID);
		WriteI2CByte('G');
		WriteI2CByte(0x74);
		WriteI2CByte(TEST_COMP_OPMODE);
	I2CStop();

	Delay1mS(COMPASS_TIME_MS);

	for (r = 0; r <= (uint8)8; r++) // do NOT use a block read
	{
		CP[r] = 0xff;

		Delay1mS(10);

		I2CStart();
			WriteI2CByte(HMC6352_ID);
			WriteI2CByte('r');
			WriteI2CByte(r);
		I2CStop();

		Delay1mS(10);

		I2CStart();
			if( WriteI2CByte(HMC6352_ID+1);
			CP[r] = ReadI2CByte(I2C_NACK);
		I2CStop();
	}

	UseI2C100KHz = false;

	Delay1mS(7);

	#endif // FULL_TEST

} // GetHMC6352Parameters

void DoTestHMC6352Compass(void)
{
	static uint16 v, prev;
	static int16 Temp;
	static uint8 i;
	static boolean r;

	TxString("HMC6352)\r\n");

	UseI2C100KHz = true;

	#ifdef FULL_TEST
	I2CStart();
		WriteI2CByte(HMC6352_ID);
		WriteI2CByte('G');
		WriteI2CByte(0x74);
		WriteI2CByte(TEST_COMP_OPMODE);
	I2CStop();

	UseI2C100KHz = false;

	Delay1mS(1);

	WriteHMC6352Command('O'); // reset

	Delay1mS(7);

	GetHMC6352Parameters();

	TxString("Registers\r\n");
	TxString("0:\tI2C"); 
	TxString("\t 0x"); TxValH(CP[0]); 
	if ( CP[0] != (uint8)0x42 ) 
		TxString("\t Error expected 0x42 for HMC6352");
	TxNextLine();

	Temp = (CP[1]*256)|CP[2];
	TxString("1:2:\tXOffset\t"); 
	TxVal32((int32)Temp, 0, 0); 
	TxNextLine(); 

	Temp = (CP[3]*256)|CP[4];
	TxString("3:4:\tYOffset\t"); 
	TxVal32((int32)Temp, 0, 0); 
	TxNextLine(); 

	TxString("5:\tDelay\t"); 
	TxVal32((int32)CP[5], 0, 0); 
	TxNextLine(); 

	TxString("6:\tNSum\t"); TxVal32((int32)CP[6], 0, 0);
	TxNextLine(); 

	TxString("7:\tSW Ver\t"); 
	TxString(" 0x"); TxValH(CP[7]); 
	TxNextLine(); 

	TxString("8:\tOpMode:");
	switch ( ( CP[8] >> 5 ) & 0x03 ) {
		case 0: TxString("  1Hz"); break;
		case 1: TxString("  5Hz"); break;
		case 2: TxString("  10Hz"); break;
		case 3: TxString("  20Hz"); break;
		}
 
	if ( CP[8] & 0x10 ) TxString(" S/R"); 

	switch ( CP[8] & 0x03 ) {
		case 0: TxString(" Standby"); break;
		case 1: TxString(" Query"); break;
		case 2: TxString(" Continuous"); break;
		case 3: TxString(" Not-allowed"); break;
		}

	Delay1mS(500);

	#endif // FULL_TEST

	TxNextLine();

	MagHeading = GetCompass();
	Heading = Make2Pi( MagHeading - CompassOffset );

    TxVal32(ConvertMPiToDDeg(MagHeading), 1, 0);
    TxString(" deg (Compass)\r\n");
    TxVal32(ConvertMPiToDDeg(Heading), 1, 0);
    TxString(" deg (True)\r\n");

} // DoTestHMC6352Compass

void CalibrateHMC6352Compass(void)
{	// calibrate the compass by rotating the ufo through 720 deg smoothly

	TxString("\r\nCalib. compass - Press the CONTINUE button (x) to continue\r\n");	
	while( PollRxChar() != 'x' ); // UAVPSet uses 'x' for CONTINUE button
		
	WriteHMC6352Command('O'); // Do Set/Reset now	
	Delay1mS(7);	 
	WriteHMC6352Command('C'); // set Compass device to Calibration mode

	TxString("\r\nRotate horizontally 720 deg in ~30 sec. - Press the CONTINUE button (x) to FINISH\r\n");
	while( PollRxChar() != 'x' );

	WriteHMC6352Command('E'); // set Compass device to End-Calibration mode 
	TxString("\r\nCalibration complete\r\n");
	Delay1mS(COMPASS_TIME_MS);

	InitCompass();

} // CalibrateHMC6352Compass

#endif // TESTING

void InitHMC6352Compass(void) 
{
	// 20Hz continuous read with periodic reset.
	#ifdef SUPPRESS_COMPASS_SR
		#define COMP_OPMODE 0b01100010
	#else
		#define COMP_OPMODE 0b01110010
	#endif // SUPPRESS_COMPASS_SR

	UseI2C100KHz = true;
	// Set device to Compass mode 
	I2CStart();
		WriteI2CByte(HMC6352_ID);
		WriteI2CByte('G');
		WriteI2CByte(0x74);
		WriteI2CByte(COMP_OPMODE);
	I2CStop();
	UseI2C100KHz = false;

	Delay1mS(1);

	WriteHMC6352Command('L'); // save operation mode in EEPROM
	Delay1mS(1); 
	WriteHMC6352Command('O'); // Do Bridge Offset Set/Reset now
	Delay1mS(COMPASS_TIME_MS);

	// use default heading mode (1/10th degrees)

} // InitHMC6352Compass

boolean HMC6352CompassActive(void) 
{
	UseI2C100KHz = true;
	F.CompassValid = I2CResponse(HMC6352_ID);
	UseI2C100KHz = false;
	return(F.CompassValid);

} // HMC6352CompassActive

#endif // INC_HMC6352



