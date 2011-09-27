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

int16 GetHMC5843Magentometer(void);
void DoTestHMC5843Magnetometer(void);
void CalibrateHMC5843Magnetometer(void);
void InitHMC5843Magnetometer(void);
boolean HMC5843CompassActive(void);

int16 GetHMC6352Compass(void);
void DoTestHMC6352Compass(void);
void CalibrateHMC6352Compass(void);
void InitHMC6352Compass(void);
boolean HMC6352CompassActive(void);

i24u 	Compass;
i32u 	HeadingValF;
int16 	MagHeading, Heading, DesiredHeading, CompassOffset;
int8 	CompassType;

const rom char * CompassName[CompassUnknown+1] = {
		"HMC5843","HMC6352","None"
		};

void ShowCompassType(void)
{
	TxString(&CompassName[CompassType]);
} // ShowCompassType		

int16 GetCompass()
{
	if ( CompassType == HMC6352Compass )
		return (GetHMC6352Compass());
	else
		if ( CompassType == HMC5843Magnetometer )
			return (GetHMC5843Magnetometer());
		else
			return(0);	
} // GetCompass
 
void GetHeading(void)
{
	static int16 HeadingChange, Temp;
	
	#ifdef SIMULATE

		if ( mSClock() > mS[CompassUpdate] )
		{
			mS[CompassUpdate] += COMPASS_TIME_MS;
			if ( State == InFlight )
			{
				#ifdef  NAV_WING
					Temp = SRS16( FakeDesiredYaw - FakeDesiredRoll, 1);	
					if ( Abs(FakeDesiredYaw - FakeDesiredRoll) > 5 )
						FakeMagHeading -= Limit1(Temp, NAV_MAX_FAKE_COMPASS_SLEW);
				#else
					Temp = FakeDesiredYaw * 5; // ~90 deg/sec
					if ( Abs(FakeDesiredYaw) > 5 )
						FakeMagHeading -= Limit1(Temp, NAV_MAX_FAKE_COMPASS_SLEW);				
				#endif // NAV_WING
				
				FakeMagHeading = Make2Pi((int16)FakeMagHeading);
			}
		}

		MagHeading = FakeMagHeading;

	#else

		if( F.CompassValid ) // continuous mode but Compass only updates avery 50mS
			MagHeading = GetCompass();
		else
			MagHeading = 0;

	#endif // SIMULATE

	Heading = Make2Pi(MagHeading - CompassOffset);

	HeadingChange = Abs( Heading - HeadingValF.iw1 );
	if ( HeadingChange > MILLIPI )// wrap 0 -> TwoPI
		HeadingValF.iw1 = Heading;
	else
		if (( HeadingChange > COMPASS_MAX_SLEW ) && ( State == InFlight )) 
		{
		     Heading = SlewLimit(HeadingValF.iw1, Heading, COMPASS_MAX_SLEW);    
		     Stats[CompassFailS]++;
		}

	LPFilter16(&Heading, &HeadingValF, YawFilterA);
	Heading = Make2Pi(Heading);

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
	Heading = Make2Pi( MagHeading - CompassOffset );
	HeadingValF.iw1 = Heading; 

	#ifdef SIMULATE
		FakeMagHeading = Heading = 0;
	#endif // SIMULATE
	DesiredHeading = Heading;

} // InitHeading

void InitCompass(void)
{
#ifdef PREFER_HMC5843
	if ( HMC5843MagnetometerActive() )
	{
		CompassType = HMC5843Magnetometer;
		InitHMC5843Magnetometer();
	}
	else
		if ( HMC6352CompassActive() )
		{
			CompassType = HMC6352Compass;
			InitHMC6352Compass();
		}
		else
			CompassType = UnknownCompass;
#else
	if ( HMC6352CompassActive() )
	{
		CompassType = HMC6352Compass;
		InitHMC6352Compass();
	}
	else
		if ( HMC5843MagnetometerActive() )
		{
			CompassType = HMC5843Magnetometer;
			InitHMC5843Magnetometer();
		}
		else
			CompassType = CompassUnknown;


#endif // PREFER_HMC5843
} // InitCompass

#ifdef TESTING 

void DoCompassTest(void)
{
	if ( CompassType == HMC6352Compass )
		DoTestHMC6352Compass();
	else
		if ( CompassType == HMC5843Magnetometer )
			DoTestHMC5843Magnetometer();
} // DoCompassTest

void CalibrateCompass(void)
{
	if ( CompassType == HMC6352Compass )
		CalibrateHMC6352Compass();
	else
		if ( CompassType == HMC5843Magnetometer )
			CalibrateHMC5843Magnetometer();
} // CalibrateCompass

#endif // TESTING
//________________________________________________________________________________________

// HMC5843 3 Axis Magnetometer

int16 Mag[3];
uint8 HMC5843_ID;

int16 GetHMC5843Magnetometer(void) {

    static char b[6];
    static i16u X, Y, Z;
	static i32u Temp;
    static uint8 r;
    static int16 mx, my;
    static int16 CRoll, SRoll, CPitch, SPitch;
	static int16 CompassVal;

	F.CompassValid = ReadI2CString(HMC5843_ID, 0x03, b, 6);

	if( F.CompassValid ) 
	{
	    X.b1 = b[0]; X.b0 = b[1];
	    Y.b1 = b[2]; Y.b0 = b[3];
	    Z.b1 = b[4]; Z.b0 = b[5];
	
		if( P[SensorHint] == SFDOF9 )
		{
			// SparkFun 9DOF Sensor Stick
		    Mag[LR] = X.i16;     // Y axis (internal sensor x axis)
		    Mag[FB] = -Y.i16;    // X axis (internal sensor y axis)
		    Mag[DU] = -Z.i16;    // Z axis
		}
		else
		{
			// another alignment :).
		    Mag[LR] = X.i16;     // Y axis (internal sensor x axis)
		    Mag[FB] = -Y.i16;    // X axis (internal sensor y axis)
		    Mag[DU] = -Z.i16;    // Z axis
		}
	
		#ifdef HMC5843_FULL
	
			// UNFORTUNATELY THE ANGLE ESTIMATES ARE NOT VALID FOR THE PIC VERSION
			// AS THEY ARE NOT BOUNDED BY +/- Pi
		    CRoll = int16cos(Angle[Roll]);
		    SRoll = int16sin(Angle[Roll]);
		    CPitch = int16cos(Angle[Pitch]);
		    SPitch = int16sin(Angle[Pitch]);

			// Tilt compensated Magnetic field X:
			Temp.i32 = (int32)Mag[1] * SRoll * SPitch + (int32)Mag[2] * CRoll * SPitch;
			Temp.i32 = (int24)Mag[0] * CPitch + Temp.i3_1;
			mx = Temp.i3_1;
			
			// Tilt compensated Magnetic field Y:
			Temp.i32 =  (int24)Mag[1] * CRoll - (int24)Mag[2] * SRoll;
			my = Temp.i3_1;

		    // Magnetic Heading
		    CompassVal = int32atan2( -my, mx );
	
		#else
	
	    	CompassVal = int32atan2( -Mag[1], Mag[0] );
	
		#endif // HMC5843_FULL
	}
	else
		CompassVal = 0;

	return ( CompassVal );

} // GetHMC5843Magnetometer

#ifdef TESTING

void CalibrateHMC5843Magnetometer(void) {
	TxString("\r\nNo calibration for magnetometer YET\r\n");	
} // CalibrateHMC5843Magnetometer

void DoTestHMC5843Magnetometer(void) {
	int32 Temp;
	uint8 i;

    TxString("\r\nCompass test (HMC5843)\r\n\r\n");

    for ( i = 0; i < (uint8)100; i++)
		GetHeading();

    TxString("Mag:\t");
    TxVal32(Mag[LR], 0, HT);
    TxVal32(Mag[FB], 0, HT);
    TxVal32(Mag[DU], 0, HT);
    TxNextLine();
    TxNextLine();

    TxVal32(ConvertMPiToDDeg(MagHeading), 1, 0);
    TxString(" deg (Compass)\r\n");
    TxVal32(ConvertMPiToDDeg(Heading), 1, 0);
    TxString(" deg (True)\r\n");

} // DoHMC5843Test

#endif // TESTING

void InitHMC5843Magnetometer(void) 
{
    Delay1mS(COMPASS_TIME_MS);
	WriteI2CByteAtAddr(HMC5843_ID, 0x02, 0x00); // Set continuous mode (default to 10Hz)
	Delay1mS(COMPASS_TIME_MS);
} // InitHMC5843Magnetometer

boolean HMC5843MagnetometerActive(void) 
{
	HMC5843_ID = HMC5843_3DOF;

	F.CompassValid = I2CResponse(HMC5843_ID);
	if ( !F.CompassValid )
	{
		HMC5843_ID = HMC5843_9DOF;
	  	F.CompassValid = I2CResponse(HMC5843_ID);
	}

	return (F.CompassValid);
} //  HMC5843MagnetometerActive

//________________________________________________________________________________________

// HMC6352 Bosch Compass

void WriteHMC6352Command(uint8 c)
{
	I2CStart(); // Do Bridge Offset Set/Reset now
		WriteI2CByte(HMC6352_ID);
		WriteI2CByte(c);
	I2CStop();
} // WriteHMC6352Command

int16 GetHMC6352Compass(void)
{
	static i16u CompassVal;

	I2CStart();
		F.CompassMissRead = WriteI2CByte(HMC6352_ID+1) != I2C_ACK; 
		CompassVal.b1 = ReadI2CByte(I2C_ACK);
		CompassVal.b0 = ReadI2CByte(I2C_NACK);
	I2CStop();

	return ( ConvertDDegToMPi( CompassVal.i16 ) );
} // GetHMC6352Compass

static uint8 CP[9];

#ifdef TESTING

#define TEST_COMP_OPMODE 0b01110000	// standby mode to reliably read EEPROM

void GetHMC6352Parameters(void)
{
	#ifdef FULL_TEST

	uint8 r;

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

	Delay1mS(7);

	#endif // FULL_TEST

} // GetHMC6352Parameters

void DoTestHMC6352Compass(void)
{
	uint16 v, prev;
	int16 Temp;
	uint8 i;
	boolean r;

	TxString("\r\nCompass test (HMC6352)\r\n");

	#ifdef FULL_TEST
	I2CStart();
		WriteI2CByte(HMC6352_ID);
		WriteI2CByte('G');
		WriteI2CByte(0x74);
		WriteI2CByte(TEST_COMP_OPMODE);
	I2CStop();

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

	for ( i = 0; i < (uint8)200; i++) // settle filter 
    	GetHeading();

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

	// Set device to Compass mode 
	I2CStart();
		WriteI2CByte(HMC6352_ID);
		WriteI2CByte('G');
		WriteI2CByte(0x74);
		WriteI2CByte(COMP_OPMODE);
	I2CStop();

	Delay1mS(1);

	WriteHMC6352Command('L'); // save operation mode in EEPROM
	Delay1mS(1); 
	WriteHMC6352Command('O'); // Do Bridge Offset Set/Reset now
	Delay1mS(COMPASS_TIME_MS);

	// use default heading mode (1/10th degrees)

} // InitHMC6352Compass

boolean HMC6352CompassActive(void) 
{
	F.CompassValid = I2CResponse(HMC6352_ID);
	return(F.CompassValid);
} // HMC6352CompassActive



