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

// depending on orientation of chip
#define ITG_ROLL_H	ITG_GX_H
#define ITG_ROLL_L	ITG_GX_L

#define ITG_PITCH_H	ITG_GY_H
#define ITG_PITCH_L	ITG_GY_L

#define ITG_YAW_H	ITG_GZ_H
#define ITG_YAW_L	ITG_GZ_L

uint8 ITG_ID, ITG_R, ITG_W;

void ITG3200ViewRegisters(void);
void BlockReadITG3200(void);
uint8 ReadByteITG3200(uint8);
void WriteByteITG3200(uint8, uint8);
void InitITG3200(void);
boolean ITG3200GyroActive(void);

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

	// Roll Right +, Pitch Up +, Yaw ACW +

	if ( P[DesGyroType] == ITG3200DOF9 )
	{

		// Roll
		GX.b0 = G[1]; GX.b1 = G[0]; GX.i16 = -GX.i16;

		// Pitch
		GY.b0 = G[3]; GY.b1 = G[2]; 

		// Yaw
		GZ.b0 = G[5]; GZ.b1 = G[4];
		GZ.i16 = - GZ.i16;

	}
	else
	{

		// Roll
		GX.b0 = G[1]; GX.b1 = G[0];
		GX.i16 = - GX.i16;

		// Pitch
		GY.b0 = G[3]; GY.b1 = G[2]; 

		// Yaw
		GZ.b0 = G[5]; GZ.b1 = G[4]; 
		GZ.i16 = - GZ.i16;

	}

	GyroADC[Roll] = GX.i16;
	GyroADC[Pitch] = GY.i16;
	GyroADC[Yaw] = GZ.i16;

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

	Delay1mS(50);

} // InitITG3200

boolean ITG3200GyroActive(void) 
{
	F.GyroFailure = true;
	ITG_ID = ITG_ID_3DOF;

    I2CStart();
    F.GyroFailure = WriteI2CByte(ITG_ID) != I2C_ACK;
    I2CStop();

	if ( F.GyroFailure )
	{
		ITG_ID = ITG_ID_6DOF;
	    I2CStart();
	    F.GyroFailure = WriteI2CByte(ITG_ID) != I2C_ACK;
	    I2CStop();
	}

	ITG_R = ITG_ID+1;	
	ITG_W =	ITG_ID;	

  return ( !F.GyroFailure );
} // ITG3200GyroActive

#ifdef TESTING

void GyroITG3200Test(void)
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

} // GyroITG3200Test

#endif // TESTING







