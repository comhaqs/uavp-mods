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

// ITG3200 3-axis I2C Gyro

// ITG3200 Register Defines
#define ITG_WHO		0x00
#define	ITG_SMPL	0x15
#define ITG_DLPF	0x16
#define ITG_INT_C	0x17
#define ITG_INT_S	0x1A
#define	ITG_TMP_H	0x1B
#define	ITG_TMP_L	0x1C
#define	ITG_GX_H	0x1D
#define	ITG_GX_L	0x1E
#define	ITG_GY_H	0x1F
#define	ITG_GY_L	0x20
#define ITG_GZ_H	0x21
#define ITG_GZ_L	0x22
#define ITG_PWR_M	0x3E

#define ITG_I2C_ID 	0xD2
#define ITG_R 		(ITG_I2C_ID+1)	
#define ITG_W 		ITG_I2C_ID

// depending on orientation of chip
#define ITG_ROLL_H	ITG_GX_H
#define ITG_ROLL_L	ITG_GX_L

#define ITG_PITCH_H	ITG_GY_H
#define ITG_PITCH_L	ITG_GY_L

#define ITG_YAW_H	ITG_GZ_H
#define ITG_YAW_L	ITG_GZ_L

void ITG3200ViewRegisters(void);
void BlockReadITG3200(void);
uint8 ReadByteITG3200(uint8);
void WriteByteITG3200(uint8, uint8);
void InitITG3200(void);

void BlockReadITG3200(void)
{
	static uint8 G[6], r;
	static i16u GX, GY, GZ;

	I2CStart();
	if( WriteI2CByte(ITG_W) != I2C_ACK ) goto SGerror;

	if( WriteI2CByte(ITG_GX_H) != I2C_ACK ) goto SGerror;

	I2CStart();	
	if( WriteI2CByte(ITG_R) != I2C_ACK ) goto SGerror;
	r = ReadI2CString(G, 6);
	I2CStop();

	GX.b0 = G[1]; GX.b1 = G[0];
	GY.b0 = G[3]; GY.b1 = G[2];
	GZ.b0 = G[5]; GZ.b1 = G[4];

	RollRateADC = GX.i16;
	PitchRateADC = -GY.i16;
	YawRateADC = -GZ.i16;

	return;	

SGerror:
	I2CStop();
	// GYRO FAILURE - FATAL
	Stats[GyroFailS]++;
	F.GyroFailure = true;
	return;
} // BlockReadITG3200

uint8 ReadByteITG3200(uint8 address)
{
	uint8 data;
		
	I2CStart();
	if( WriteI2CByte(ITG_W) != I2C_ACK ) goto SGerror;
	if( WriteI2CByte(address) != I2C_ACK ) goto SGerror;

	I2CStart();
	if( WriteI2CByte(ITG_R) != I2C_ACK ) goto SGerror;	
	data = ReadI2CByte(I2C_NACK);
	I2CStop();
	
	return ( data );

SGerror:
	I2CStop();
	// GYRO FAILURE - FATAL
	Stats[GyroFailS]++;
	F.GyroFailure = true;
	return (0);
} // ReadByteITG3200

void WriteByteITG3200(uint8 address, uint8 data)
{
	I2CStart();	// restart
	if( WriteI2CByte(ITG_W) != I2C_ACK ) goto SGerror;
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
} // WriteByteITG3200

void InitITG3200(void)
{
	F.GyroFailure = false; // reset optimistically!

	WriteByteITG3200(ITG_PWR_M, 0x80);			// Reset to defaults
	WriteByteITG3200(ITG_SMPL, 0x00);			// continuous update
	WriteByteITG3200(ITG_DLPF, 0b00011001);		// 188Hz, 2000deg/S
	WriteByteITG3200(ITG_INT_C, 0b00000000);	// no interrupts
	WriteByteITG3200(ITG_PWR_M, 0b00000001);	// X Gyro as Clock Ref.
} // InitITG3200

void GetGyroValues(void)
{ 
	BlockReadITG3200();
} // GetGyroValues

void CalculateGyroRates(void)
{
	static int16 Temp;

	// ITG-3200 Gyro
	Rate[Roll] = SRS16( GyroADC[Roll] - GyroNeutral[Roll], 4);	
	Rate[Pitch] = SRS16( GyroADC[Pitch] - GyroNeutral[Pitch], 4);
	
	Rate[Yaw] = GyroADC[Yaw] - GyroNeutral[Yaw]; 
	Rate[Yaw] = SRS16(Rate[Yaw], 4);	

} // CalculateGyroRates

void ErectGyros(void)
{
	static int16 i;
	extern uint8 a;
	static int32 Av[3];

	LEDRed_ON;
	
	Av[Roll] = Av[Pitch] = Av[Yaw] =0;	
    for ( i = 0; i < 32 ; i++ )
	{
		BlockReadITG3200();
	
		Av[Roll] += GyroADC[Roll];
		Av[Pitch] += GyroADC[Pitch];	
		Av[Yaw] += GyroADC[Yaw];
	}
	
	for ( a = 0; a < (int8)3; a++ )
	{
		GyroNeutral[a] = SRS32(Av[a], 5);		
		Rate[a] = Angle[Roll] = 0;
	}

	REp = PEp = YEp = 0;

	LEDRed_OFF;

} // ErectGyros

void InitGyros(void)
{
	InitITG3200();
} // InitGyros

#ifdef TESTING

void CheckGyroFault(uint8 v, uint8 lv, uint8 hv)
{
	// not used for ITG-3000
} // CheckGyroFault

void GyroTest(void)
{
	TxString("\r\nITG3200 3 axis I2C Gyro Test\r\n");
	TxString("WHO_AM_I  \t"); TxValH(ReadByteITG3200(ITG_WHO)); TxNextLine();
//	Delay1mS(1000);
	TxString("SMPLRT_DIV\t"); TxValH(ReadByteITG3200(ITG_SMPL)); TxNextLine();
	TxString("DLPF_FS   \t"); TxValH(ReadByteITG3200(ITG_DLPF)); TxNextLine();
	TxString("INT_CFG   \t"); TxValH(ReadByteITG3200(ITG_INT_C)); TxNextLine();
	TxString("INT_STATUS\t"); TxValH(ReadByteITG3200(ITG_INT_S)); TxNextLine();
	TxString("TEMP      \t"); TxVal32( (ReadByteITG3200(ITG_TMP_H)<<8) | ReadByteITG3200(ITG_TMP_L),0,0); TxNextLine();
	TxString("GYRO_X    \t"); TxVal32( (ReadByteITG3200(ITG_GX_H)<<8) | ReadByteITG3200(ITG_GX_L),0,0); TxNextLine();
	TxString("GYRO_Y    \t"); TxVal32( (ReadByteITG3200(ITG_GY_H)<<8) | ReadByteITG3200(ITG_GY_L),0,0); TxNextLine();
	TxString("GYRO_Z    \t"); TxVal32( (ReadByteITG3200(ITG_GZ_H)<<8) | ReadByteITG3200(ITG_GZ_L),0,0); TxNextLine();
	TxString("PWR_MGM   \t"); TxValH(ReadByteITG3200(ITG_PWR_M)); TxNextLine();

	TxNextLine();
	if ( F.GyroFailure )
		TxString("Test FAILED\r\n");
	else
		TxString("Test OK\r\n");

} // GyroTest

#endif // TESTING







