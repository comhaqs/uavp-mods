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

int16 GetCompass(void);
void GetHeading(void);
void GetCompassParameters(void);
void DoCompassTest(void);
void CalibrateCompass(void);
void InitHeading(void);
void InitCompass(void);

i16u Compass;
int16 	CompassFilterA;
i32u 	CompassValF;

int16 GetCompass(void)
{
	static i16u CompassVal;

	I2CStart();
	F.CompassMissRead = WriteI2CByte(COMPASS_I2C_ID+1) != I2C_ACK; 
	CompassVal.b1 = ReadI2CByte(I2C_ACK);
	CompassVal.b0 = ReadI2CByte(I2C_NACK);
	I2CStop();

	return ( CompassVal.i16 );
} // GetCompass

void GetHeading(void)
{
	static i24u CompassVal;
//	static i32u Temp;

	if( F.CompassValid ) // continuous mode but Compass only updates avery 50mS
	{
		CompassVal.i2_1 = GetCompass();
		CompassVal.b0 = 0;

		CompassVal.i2_1 = Make2Pi((int16) ConvertDDegToMPi(CompassVal.i2_1) - CompassOffset);
		CompassValF.i32 += ((int32)CompassVal.i24 - CompassValF.i3_1) * CompassFilterA;

		Heading = CompassValF.iw1; 
		if ( F.CompassMissRead && (State == InFlight) ) Stats[CompassFailS]++;	
	}
	else
		Heading = 0;

	#ifdef SIMULATE
		#if ( defined AILERON | defined ELEVON )
			if ( State == InFlight )
				FakeHeading -= FakeDesiredRoll/5 + FakeDesiredYaw/5;
		#else
			if ( State == InFlight )
			{
				if ( Abs(FakeDesiredYaw) > 5 )
					FakeHeading -= FakeDesiredYaw/5;
			}
	
		FakeHeading = Make2Pi((int16)FakeHeading);
		Heading = FakeHeading;
		#endif // AILERON | ELEVON

	#endif // SIMULATE

} // GetHeading

static uint8 CP[9];

#ifdef TESTING

#define TEST_COMP_OPMODE 0b01110000	// standby mode to reliably read EEPROM

void GetCompassParameters(void)
{
	#ifndef CLOCK_40MHZ

	uint8 r;

	I2CStart();
	if( WriteI2CByte(COMPASS_I2C_ID) != I2C_ACK ) goto CTerror;
	if( WriteI2CByte('G')  != I2C_ACK ) goto CTerror;
	if( WriteI2CByte(0x74) != I2C_ACK ) goto CTerror;
	if( WriteI2CByte(TEST_COMP_OPMODE) != I2C_ACK ) goto CTerror;
	I2CStop();

	Delay1mS(COMPASS_TIME_MS);

	for (r = 0; r <= (uint8)8; r++)
	{
		CP[r] = 0xff;

		Delay1mS(10);

		I2CStart();
		if ( WriteI2CByte(COMPASS_I2C_ID) != I2C_ACK ) goto CTerror;
		if ( WriteI2CByte('r')  != I2C_ACK ) goto CTerror;
		if ( WriteI2CByte(r)  != I2C_ACK ) goto CTerror;
		I2CStop();

		Delay1mS(10);

		I2CStart();
		if( WriteI2CByte(COMPASS_I2C_ID+1) != I2C_ACK ) goto CTerror;
		CP[r] = ReadI2CByte(I2C_NACK);
		I2CStop();

	}

	Delay1mS(7);

	return;

CTerror:
	I2CStop();
	TxString("FAIL\r\n");

	#endif // CLOCK_40MHZ
} // GetCompassParameters

void DoCompassTest(void)
{
	uint16 v, prev;
	int16 Temp;

	TxString("\r\nCompass test\r\n");

	I2CStart();
	if( WriteI2CByte(COMPASS_I2C_ID) != I2C_ACK ) goto CTerror;
	if( WriteI2CByte('G')  != I2C_ACK ) goto CTerror;
	if( WriteI2CByte(0x74) != I2C_ACK ) goto CTerror;
	if( WriteI2CByte(TEST_COMP_OPMODE) != I2C_ACK ) goto CTerror;
	I2CStop();

	Delay1mS(1);

	I2CStart(); // Do Set/Reset now		
	if( WriteI2CByte(COMPASS_I2C_ID) != I2C_ACK ) goto CTerror;
	if( WriteI2CByte('O')  != I2C_ACK ) goto CTerror;
	I2CStop();

	Delay1mS(7);

	#ifndef CLOCK_40MHZ

	GetCompassParameters();

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
	TxNextLine();

	#endif // CLOCK_40MHZ

	InitCompass();
	if ( !F.CompassValid ) goto CTerror;

	Delay1mS(COMPASS_TIME_MS);

	GetHeading();
	if ( F.CompassMissRead ) goto CTerror;

	TxVal32((int32)Compass.u16, 1, 0);
	TxString(" deg \tCorrected ");
	TxVal32(((int32)Heading * 180L)/CENTIPI, 1, 0);
	TxString(" deg\r\n");
	
	return;
CTerror:
	I2CStop();
	TxString("FAIL\r\n");
} // DoCompassTest

void CalibrateCompass(void)
{	// calibrate the compass by rotating the ufo through 720 deg smoothly
	TxString("\r\nCalib. compass - Press any key (x) to continue\r\n");	
	while( PollRxChar() != 'x' ); // UAVPSet uses 'x' for any key button

	I2CStart(); // Do Set/Reset now		
	if( WriteI2CByte(COMPASS_I2C_ID) != I2C_ACK ) goto CCerror;
	if( WriteI2CByte('O')  != I2C_ACK ) goto CCerror;
	I2CStop();

	Delay1mS(7);

	// set Compass device to Calibration mode 
	I2CStart();
	if( WriteI2CByte(COMPASS_I2C_ID) != I2C_ACK ) goto CCerror;
	if( WriteI2CByte('C')  != I2C_ACK ) goto CCerror;
	I2CStop();

	TxString("\r\nRotate horizontally 720 deg in ~30 sec. - Press any key (x) to FINISH\r\n");
	while( PollRxChar() != 'x' );

	// set Compass device to End-Calibration mode 
	I2CStart();
	if( WriteI2CByte(COMPASS_I2C_ID) != I2C_ACK ) goto CCerror;
	if( WriteI2CByte('E')  != I2C_ACK ) goto CCerror;
	I2CStop();

	TxString("\r\nCalibration complete\r\n");

	Delay1mS(COMPASS_TIME_MS);

	InitCompass();

	return;

CCerror:
	TxString("Calibration FAILED\r\n");
} // CalibrateCompass

#endif // TESTING

void InitCompass(void)
{

	CompassFilterA = ( (int24) COMPASS_TIME_MS * 256L) / ( 10000L / ( 6L * (int16) COMPASS_FREQ ) + (int16) COMPASS_TIME_MS );
	CompassValF.i32 = 0;

	// 20Hz continuous read with periodic reset.
	#ifdef SUPPRESS_COMPASS_SR
		#define COMP_OPMODE 0b01100010
	#else
		#define COMP_OPMODE 0b01110010
	#endif // SUPPRESS_COMPASS_SR

	// Set device to Compass mode 
	I2CStart();
	if( WriteI2CByte(COMPASS_I2C_ID) != I2C_ACK ) goto CTerror;
	if( WriteI2CByte('G')  != I2C_ACK ) goto CTerror;
	if( WriteI2CByte(0x74) != I2C_ACK ) goto CTerror;
	if( WriteI2CByte(COMP_OPMODE) != I2C_ACK ) goto CTerror;
	I2CStop();

	Delay1mS(1);

	I2CStart(); // save operation mode in EEPROM
	if( WriteI2CByte(COMPASS_I2C_ID) != I2C_ACK ) goto CTerror;
	if( WriteI2CByte('L')  != I2C_ACK ) goto CTerror;
	I2CStop();

	Delay1mS(1);

	I2CStart(); // Do Bridge Offset Set/Reset now
	if( WriteI2CByte(COMPASS_I2C_ID) != I2C_ACK ) goto CTerror;
	if( WriteI2CByte('O')  != I2C_ACK ) goto CTerror;
	I2CStop();

	Delay1mS(COMPASS_TIME_MS);

	// use default heading mode (1/10th degrees)

	F.CompassValid = true;
	return;
CTerror:
	F.CompassValid = false;
	Stats[CompassFailS]++;
	F.CompassFailure = true;
	
	I2CStop();
} // InitCompass

void InitHeading(void)
{
	static int16 CompassVal;

	CompassValF.i32 = 0;
	CompassVal = GetCompass();
	CompassValF.iw1 = Make2Pi((int16) ConvertDDegToMPi(CompassVal) - CompassOffset);

	#ifdef SIMULATE
		FakeHeading = Heading = 0;
	#endif // SIMULATE
	DesiredHeading = Heading;
	HEp = 0;
} // InitHeading


