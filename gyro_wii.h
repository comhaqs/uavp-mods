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

// WII 3-axis I2C Gyro

// WII Register Defines
#define WII_WHO		0x00
#define	WII_SMPL	0x15
#define WII_DLPF	0x16
#define WII_INT_C	0x17
#define WII_INT_S	0x1A
#define	WII_TMP_H	0x1B
#define	WII_TMP_L	0x1C
#define	WII_GX_H	0x1D
#define	WII_GX_L	0x1E
#define	WII_GY_H	0x1F
#define	WII_GY_L	0x20
#define WII_GZ_H	0x21
#define WII_GZ_L	0x22
#define WII_PWR_M	0x3E

#define WII_I2C_ID 	0xD2
#define WII_R 		(WII_I2C_ID+1)	
#define WII_W 		WII_I2C_ID

// depending on orientation of chip
#define WII_ROLL_H	WII_GX_H
#define WII_ROLL_L	WII_GX_L

#define WII_PITCH_H	WII_GY_H
#define WII_PITCH_L	WII_GY_L

#define WII_YAW_H	WII_GZ_H
#define WII_YAW_L	WII_GZ_L

void WIIViewRegisters(void);
void BlockReadWII(void);
uint8 ReadByteWII(uint8);
void WriteByteWII(uint8, uint8);
void InitWII(void);

void BlockReadWII(void)
{
	static uint8 G[6], r;
	static i16u GX, GY, GZ;

	I2CStart();
	if( WriteI2CByte(WII_W) != I2C_ACK ) goto SGerror;

	if( WriteI2CByte(WII_GX_H) != I2C_ACK ) goto SGerror;

	I2CStart();	
	if( WriteI2CByte(WII_R) != I2C_ACK ) goto SGerror;
	r = ReadI2CString(G, 6);
	I2CStop();

	GX.b0 = G[1]; GX.b1 = G[0];
	GY.b0 = G[3]; GY.b1 = G[2];
	GZ.b0 = G[5]; GZ.b1 = G[4];

	Gyro[Roll]ADC = GX.i16;
	Gyro[Pitch]ADC = -GY.i16;
	Gyro[Yaw]ADC = -GZ.i16;

	return;	

SGerror:
	I2CStop();
	// GYRO FAILURE - FATAL
	Stats[GyroFailS]++;
	F.GyroFailure = true;
	return;
} // BlockReadWII

uint8 ReadByteWII(uint8 address)
{
	uint8 data;
		
	I2CStart();
	if( WriteI2CByte(WII_W) != I2C_ACK ) goto SGerror;
	if( WriteI2CByte(address) != I2C_ACK ) goto SGerror;

	I2CStart();
	if( WriteI2CByte(WII_R) != I2C_ACK ) goto SGerror;	
	data = ReadI2CByte(I2C_NACK);
	I2CStop();
	
	return ( data );

SGerror:
	I2CStop();
	// GYRO FAILURE - FATAL
	Stats[GyroFailS]++;
	F.GyroFailure = true;
	return ( 0 );
} // ReadByteWII

void WriteByteWII(uint8 address, uint8 data)
{
	I2CStart();	// restart
	if( WriteI2CByte(WII_W) != I2C_ACK ) goto SGerror;
	if( WriteI2CByte(address) != I2C_ACK ) goto SGerror;
	if(WriteI2CByte(data) != I2C_ACK ) goto SGerror;
	I2CStop();
	return;

SGerror:
	I2CStop();
	// GYRO FAILURE - FATAL
	Stats[GyroFailS]++;
	F.GyroFailure = true;
	return;
} // WriteByteWII

void InitWII(void)
{
	F.GyroFailure = false; // reset optimistically!

	WriteByteWII(WII_PWR_M, 0x80);			// Reset to defaults
	WriteByteWII(WII_SMPL, 0x00);			// continuous update
	WriteByteWII(WII_DLPF, 0b00011001);		// 188Hz, 2000deg/S
	WriteByteWII(WII_INT_C, 0b00000000);	// no interrupts
	WriteByteWII(WII_PWR_M, 0b00000001);	// X Gyro as Clock Ref.
} // InitWII

void GetGyroValues(void)
{ // invoked TWICE per control cycle so roll/pitch "rates" are double actual

	BlockReadWII();

} // GetRollPitchGyroValues

void CalcGyroRates(void)
{
	static int16 Temp;

	// Gyro[Roll] & Gyro[Pitch] hold the sum of 2 consecutive conversions
	// 300 Deg/Sec is the "reference" gyro full scale rate
	// ITG-3200 Gyro rescaled 1/8
	Gyro[Roll] = SRS16( Gyro[Roll]ADC - GyroMidRoll, 3);	
	Gyro[Pitch] = SRS16( Gyro[Pitch]ADC - GyroMidPitch, 3);
	
	Gyro[Yaw] = Gyro[Yaw]ADC - GyroMidYaw; 
	Gyro[Yaw] = SRS16(Gyro[Yaw], 1);	

} // CalcGyroRates

void ErectGyros(void)
{
	static int16 i;
	static int32 RollAv, PitchAv, YawAv;

	LEDRed_ON;
	
	RollAv = PitchAv = YawAv = 0;	
    for ( i = 0; i < 32 ; i++ )
	{
		BlockReadWII();
	
		RollAv += Gyro[Roll]ADC;
		PitchAv += Gyro[Pitch]ADC;	
		YawAv += Gyro[Yaw]ADC;
	}
	
	GyroMidRoll = SRS32(RollAv, 5);	
	GyroMidPitch = SRS32(PitchAv, 5);
	GyroMidYaw = SRS32(YawAv, 5);
	
	for ( i = 0; i <3; i++ )
		Gyro[i] =  Angle[i] = 0;

	LEDRed_OFF;

} // ErectGyros

void InitGyros(void)
{
	InitWII();
} // InitGyros

#ifdef TESTING

void CheckGyroFault(uint8 v, uint8 lv, uint8 hv)
{
	// not used for ITG-3000
} // CheckGyroFault

void GyroTest(void)
{
	TxString("\r\nWII 3 axis I2C Gyro Test\r\n");
	TxString("WHO_AM_I  \t"); TxValH(ReadByteWII(WII_WHO)); TxNextLine();
//	Delay1mS(1000);
	TxString("SMPLRT_DIV\t"); TxValH(ReadByteWII(WII_SMPL)); TxNextLine();
	TxString("DLPF_FS   \t"); TxValH(ReadByteWII(WII_DLPF)); TxNextLine();
	TxString("INT_CFG   \t"); TxValH(ReadByteWII(WII_INT_C)); TxNextLine();
	TxString("INT_STATUS\t"); TxValH(ReadByteWII(WII_INT_S)); TxNextLine();
	TxString("TEMP      \t"); TxVal32( (ReadByteWII(WII_TMP_H)<<8) | ReadByteWII(WII_TMP_L),0,0); TxNextLine();
	TxString("GYRO_X    \t"); TxVal32( (ReadByteWII(WII_GX_H)<<8) | ReadByteWII(WII_GX_L),0,0); TxNextLine();
	TxString("GYRO_Y    \t"); TxVal32( (ReadByteWII(WII_GY_H)<<8) | ReadByteWII(WII_GY_L),0,0); TxNextLine();
	TxString("GYRO_Z    \t"); TxVal32( (ReadByteWII(WII_GZ_H)<<8) | ReadByteWII(WII_GZ_L),0,0); TxNextLine();
	TxString("PWR_MGM   \t"); TxValH(ReadByteWII(WII_PWR_M)); TxNextLine();

	TxNextLine();
	if ( F.GyroFailure )
		TxString("Test FAILED\r\n");
	else
		TxString("Test OK\r\n");

} // GyroTest

#endif // TESTING







