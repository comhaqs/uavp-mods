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

// InvenSense ITG3200 3-axis I2C Gyro

// InvenSense ITG3200 Register Defines
#define INV_WHO		0x00
#define	INV_SMPL	0x15
#define INV_DLPF	0x16
#define INV_INT_C	0x17
#define INV_INT_S	0x1A
#define	INV_TMP_H	0x1B
#define	INV_TMP_L	0x1C
#define	INV_GX_H	0x1D
#define	INV_GX_L	0x1E
#define	INV_GY_H	0x1F
#define	INV_GY_L	0x20
#define INV_GZ_H	0x21
#define INV_GZ_L	0x22
#define INV_PWR_M	0x3E

uint8 INV_ID, INVGyroAddress, INVAccAddress;

void InvenSenseViewRegisters(void);
void BlockReadInvenSenseGyro(void);
void InitInvenSenseGyro(void);
boolean InvenSenseGyroActive(void);

void BlockReadInvenSenseGyro(void)
{	// Roll Right +, Pitch Up +, Yaw ACW +
	F.GyroFailure = !ReadI2CString(INV_ID, INVGyroAddress, G.c, 6);
	if ( F.GyroFailure ) 
		Stats[GyroFailS]++;
	else	
		if ( P[SensorHint] == ITG3200DOF9) 
		{
			GyroADC[Roll] = -G.i16[X];
			GyroADC[Pitch] = G.i16[Y];
			GyroADC[Yaw] = -G.i16[Z];
		}
		else
			if ( P[SensorHint] == ITG3200Gyro )
			{
				GyroADC[Roll] = -G.i16[X];
				GyroADC[Pitch] = G.i16[Y];
				GyroADC[Yaw] = -G.i16[Z];
			}
			else
			{ // MPU6050
				GyroADC[Roll] = -G.i16[X];
				GyroADC[Pitch] = G.i16[Y];
				GyroADC[Yaw] = -G.i16[Z];
			}

} // BlockReadInvenSenseGyro

void InitInvenSenseGyro(void)
{
	if ( INV_ID == MPU6050_ID )
	{


	}
	else
	{
		WriteI2CByteAtAddr(INV_ID,INV_PWR_M, 0x80);			// Reset to defaults
		WriteI2CByteAtAddr(INV_ID,INV_SMPL, 0x00);			// continuous update
		WriteI2CByteAtAddr(INV_ID,INV_DLPF, 0b00011001);	// 188Hz, 2000deg/S
		WriteI2CByteAtAddr(INV_ID,INV_INT_C, 0b00000000);	// no interrupts
		WriteI2CByteAtAddr(INV_ID,INV_PWR_M, 0b00000001);	// X Gyro as Clock Ref.
	}

	Delay1mS(50);

} // InitInvenSenseGyro

boolean InvenSenseGyroActive(void) 
{
	INV_ID = INV_ID_3DOF;
	INVGyroAddress = INV_GX_H;

	F.GyroFailure = !I2CResponse(INV_ID);
	if ( F.GyroFailure )
	{
		INV_ID = INV_ID_6DOF;
		INVGyroAddress = INV_GX_H;

	  	F.GyroFailure = !I2CResponse(INV_ID);
		if ( F.GyroFailure )
		{
			INV_ID = INV_ID_MPU6050;
			INVGyroAddress = MPU6050_GYRO_XOUT_H;

			F.GyroFailure = !I2CResponse(INV_ID);
		}
	}

  return ( !F.GyroFailure );
} // InvenSenseGyroActive

#ifdef TESTING

void GyroInvenSenseTest(void)
{
	TxString("\r\nInvenSense 3 axis I2C Gyro Test\r\n");

	if ( INV_ID == MPU6050_ID )
	{
		TxString("No MPU6050 Test yet\r\n");
	}
	else
	if ( I2CResponse(INV_ID))
	{
		TxString("WHO_AM_I  \t"); TxValH(ReadI2CByteAtAddr(INV_ID,INV_WHO)); TxNextLine();
	//	Delay1mS(1000);
		TxString("SMPLRT_DIV\t"); TxValH(ReadI2CByteAtAddr(INV_ID,INV_SMPL)); TxNextLine();
		TxString("DLPF_FS   \t"); TxValH(ReadI2CByteAtAddr(INV_ID,INV_DLPF)); TxNextLine();
		TxString("INT_CFG   \t"); TxValH(ReadI2CByteAtAddr(INV_ID,INV_INT_C)); TxNextLine();
		TxString("INT_STATUS\t"); TxValH(ReadI2CByteAtAddr(INV_ID,INV_INT_S)); TxNextLine();
		TxString("TEMP      \t"); TxVal32((ReadI2CByteAtAddr(INV_ID,INV_TMP_H)<<8) | ReadI2CByteAtAddr(INV_ID,INV_TMP_L),0,0); TxNextLine();
		TxString("GYRO_X    \t"); TxVal32((ReadI2CByteAtAddr(INV_ID,INV_GX_H)<<8) | ReadI2CByteAtAddr(INV_ID,INV_GX_L),0,0); TxNextLine();
		TxString("GYRO_Y    \t"); TxVal32((ReadI2CByteAtAddr(INV_ID,INV_GY_H)<<8) | ReadI2CByteAtAddr(INV_ID,INV_GY_L),0,0); TxNextLine();
		TxString("GYRO_Z    \t"); TxVal32((ReadI2CByteAtAddr(INV_ID,INV_GZ_H)<<8) | ReadI2CByteAtAddr(INV_ID,INV_GZ_L),0,0); TxNextLine();
		TxString("PWR_MGM   \t"); TxValH(ReadI2CByteAtAddr(INV_ID,INV_PWR_M)); TxNextLine();
		TxString("\r\nTest OK\r\n");
	}
	else
		TxString("\r\nTest FAILED\r\n");

} // GyroInvenSenseTest

#endif // TESTING







