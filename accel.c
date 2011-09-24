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

// Accelerator 400KHz I2C or SPI

#include "uavx.h"

#include "MPU6050.h"

void ShowAccType(void);
void ReadAccelerations(void);
void GetNeutralAccelerations(void);
void AccelerometerTest(void);
void InitAccelerometers(void);

void ReadADXL345Acc(void);
void InitADXL345Acc(void);
boolean ADXL345AccActive(void);

void ReadMPU6050Acc(void);
void InitMPU6050Acc(void);
boolean MPU6050AccActive(void);

void ReadBMA180Acc(void);
void InitBMA180Acc(void);
boolean BMA180AccActive(void);

void SendCommand(int8);
uint8 ReadLISL(uint8);
uint8 ReadLISLNext(void);
void WriteLISL(uint8, uint8);
void InitLISLAcc(void);
boolean LISLAccActive(void);
void ReadLISLAcc(void);

#pragma udata accs
int16	AccADC[3];
int16	IntCorr[3];
int8	AccNeutral[3];
int16	Acc[3];
int8 	AccType;
#pragma udata

void ShowAccType(void)
{
    switch ( AccType ) {
	case LISLAcc:
		TxString("LIS3L");
		break;
	case ADXL345Acc:
		TxString("ADXL345");
		break;
	case MPU6050Acc:
		TxString("MPU6050");
		break;
	case BMA180Acc:
		TxString("BMA180");
		break;
	case AccUnknown:
		TxString("Unknown");
		break;
	default:;
	} // switch
} // ShowAccType

void ReadAccelerations(void)
{
	switch ( AccType ) {
	case LISLAcc:
		ReadLISLAcc();
		break;
	#ifdef INC_ADXL345
	case ADXL345Acc:
		ReadADXL345Acc();
		break;
	#endif // INC_ADXL345
	#ifdef INC_BMA180
	case BMA180Acc:
		ReadBMA180Acc();
		break;
	#endif // INC_BMA180
	#ifdef INC_MPU6050
	case MPU6050Acc:
		ReadMPU6050Acc();
		break;
	#endif // INC_MPU6050
	default:
		break;
	} // Switch

} // ReadAccelerations

void GetNeutralAccelerations(void)
{
	// this routine is called ONLY ONCE while booting
	// and averages accelerations over 16 samples.
	// Puts values in Neutralxxx registers.
	static uint8 i;
	static int16 Temp[3];

	// already done in caller program
	Temp[LR] = Temp[FB] = Temp[DU] = 0;
	if ( F.AccelerationsValid )
	{
		for ( i = 16; i; i--)
		{
			ReadAccelerations();

			Temp[LR] += AccADC[LR];
			Temp[DU] += AccADC[DU];
			Temp[FB] += AccADC[FB];
		}	
	
		Temp[LR] = SRS16(Temp[LR], 4);
		Temp[FB] = SRS16(Temp[FB], 4);
		Temp[DU] = SRS16(Temp[DU], 4);
	
		AccNeutral[LR] = Limit1(Temp[LR], 99);
		AccNeutral[FB] = Limit1(Temp[FB], 99);
		AccNeutral[DU] = Limit1(Temp[DU] - 1024, 99); // -1g
	}
	else
		AccNeutral[LR] = AccNeutral[FB] = AccNeutral[DU] = 0;

} // GetNeutralAccelerations

#ifdef TESTING

void AccelerometerTest(void)
{
	int16 Mag;

	TxString("\r\nAccelerometer test:\r\n");
	TxString("Read once - no averaging (1024 = 1G)\r\n");

	InitAccelerometers();
	if( F.AccelerationsValid )
	{
		ReadAccelerations();
	
		TxString("\tL->R: \t");
		TxVal32((int32)AccADC[LR], 0, 0);
		if ( Abs((AccADC[LR])) > 128 )
			TxString(" fault?");
		TxNextLine();

		TxString("\tF->B: \t");	
		TxVal32((int32)AccADC[FB], 0, 0);
		if ( Abs((AccADC[FB])) > 128 )
			TxString(" fault?");	
		TxNextLine();

		TxString("\tD->U:    \t");
	
		TxVal32((int32)AccADC[DU], 0, 0);
		if ( ( AccADC[DU] < 896 ) || ( AccADC[DU] > 1152 ) )
			TxString(" fault?");	
		TxNextLine();

		TxString("\tMag:    \t");
		Mag = int32sqrt(Sqr((int24)AccADC[LR])+Sqr((int24)AccADC[FB])+Sqr((int24)AccADC[DU]));
		TxVal32((int32)Mag, 0, 0);
		TxNextLine();
	}
	else
		TxString("\r\n(Acc. not present)\r\n");
} // AccelerometerTest

#endif // TESTING

void InitAccelerometers(void)
{
	uint8 a;

	for ( a = 0; a<(uint8)3; a++)
		AccNeutral[a] = AccADC[a] = 0;
	AccADC[DU] = GRAVITY;

	#ifdef SUPPRESS_ACC

		AccType = AccUnknown;
		F.AccelerationsValid = false;

	#else

		#ifdef PREFER_LISL
	
		if ( LISLAccActive() )
		{
			AccType = LISLAcc;
			InitLISLAcc();
		}
		else
			#ifdef INC_ADXL345
			if ( ADXL345AccActive() )
			{
				AccType = ADXL345Acc;
				InitADXL345Acc();
			}		
			else
			#endif // INC_ADXL345
				#ifdef INC_BMA180
				if ( BMA180AccActive() )
				{
					AccType = BMA180Acc;
					InitBMA180Acc();
				}
				else
				#endif // INC_BMA180
					#ifdef INC_MPU6050
					if ( MPU6050AccActive() )
					{
						AccType = MPU6050Acc;
						InitMPU6050Acc();
					}
					else
					#endif // INC_MPU6050
					{
						AccType = AccUnknown;
						F.AccelerationsValid = false;
					}
		
		#else
	
		#ifdef INC_ADXL345
		if ( ADXL345AccActive() )
		{
			AccType = ADXL345Acc;
			InitADXL345Acc();
		}		
		else
		#endif // INC_ADXL345
			#ifdef INC_BMA180
			if ( BMA180AccActive() )
			{
				AccType = BMA180Acc;
				InitBMA180Acc();
			}
			else
			#endif // INC_BMA180
				#ifdef INC_MPU6050
				if ( MPU6050AccActive() )
				{
					AccType = MPU6050Acc;
					InitMPU6050Acc();
				}
				else
				#endif // INC_MPU6050
					if ( LISLAccActive() )
					{
						AccType = LISLAcc;
						InitLISLAcc();
					}
					else
					{
						AccType = AccUnknown;
						F.AccelerationsValid = false;
					}
	
		#endif // PREFER_LISL

	#endif // SUPPRESS_ACC

	if( F.AccelerationsValid )
	{
		LEDYellow_ON;
		GetNeutralAccelerations();
	}
	else
	{
		LEDYellow_OFF;
		F.AccFailure = true;
	}
} // InitAccelerometers

//________________________________________________________________________________________________

#ifdef INC_ADXL345

boolean ADXL345AccActive(void);

void ReadADXL345Acc(void) 
{
	static charint16x4u A;

	if ( ReadI2CString(ADXL345_ID, 0x32, A.c, 6) ) 
	{
		if ( P[SensorHint] == ITG3200DOF9)
		{
			// SparkFun 9DOF breakouts pins forward components up
			AccADC[LR] = -A.i16[X]; 		
		    AccADC[DU] = A.i16[Z]; 	
		    AccADC[FB] = A.i16[Y]; 
		}
		else
		{
			// SparkFun 6DOF & ITG3200 breakouts pins forward components up    	
			AccADC[LR] = A.i16[Y]; 		
		    AccADC[DU] = A.i16[Z]; 	
		    AccADC[FB] = A.i16[X]; 
		}
		
		AccADC[LR] *= 5; 		
		AccADC[DU] *= 5; 	
		AccADC[FB] *= 5;
	}
	else
	{
		AccADC[LR] = AccADC[FB] = 0; AccADC[DU] = GRAVITY;
		if ( State == InFlight )
		{
			Stats[AccFailS]++;	// data over run - acc out of range
			// use neutral values!!!!
			F.AccFailure = true;
		}
	}

} // ReadADXL345Acc

void InitADXL345Acc() {

	uint8 i;
	int16 AccLR, AccDU, AccFB;

	WriteI2CByteAtAddr(ADXL345_ID, 0x2D, 0x08);  // measurement mode
    Delay1mS(5);
	WriteI2CByteAtAddr(ADXL345_ID, 0x31, 0x08);  // full resolution, 2g
    Delay1mS(5);
	WriteI2CByteAtAddr(ADXL345_ID, 0x2C,
	    //0x0C);  	// 400Hz
		//0x0b);	// 200Hz 
	  	0x0a); 	// 100Hz 
	  	//WriteI2CByte(0x09); 	// 50Hz
    Delay1mS(5);

} // InitADXL345Acc

boolean ADXL345AccActive(void) 
{
    F.AccelerationsValid = I2CResponse(ADXL345_ID);
    return( F.AccelerationsValid );
} // ADXL345AccActive

#endif // INC_ADXL345

//________________________________________________________________________________________________

#ifdef INC_MPU6050

boolean MPU6050AccActive(void);

void ReadMPU6050Acc(void) 
{
	static charint16x4u A;

	if ( ReadI2CString(MPU6050_ID, MPU6050_ACC_XOUT_H, A.c, 6) ) 
	{
		// QuadroUFO 	
		AccADC[LR] = A.i16[Y]; 
		AccADC[DU] = A.i16[Z];
		AccADC[FB] = A.i16[X];
	}
	else
	{
		AccADC[LR] = AccADC[FB] = 0; AccADC[DU] = GRAVITY_MPU6050;
		if ( State == InFlight )
		{
			Stats[AccFailS]++;	// data over run - acc out of range
			// use neutral values!!!!
			F.AccFailure = true;
		}
	}

} // ReadMPU6050Acc

void InitMPU6050Acc() {

	InitInvenSenseGyro();

	WriteI2CByteAtAddr(MPU6050_ID, MPU6050_ACC_CONFIG, 
			0 // 2G
			//1 << 3 // 4G
			//2 << 3 // 8G
			//3 << 3 // 16G 
			//| 1 // 2.5Hz
			//| 2 // 2.5Hz
			| 3 // 1.25Hz
			//| 4 // 0.63Hz
			//| 7 // 0.63Hz
			);

    Delay1mS(5);

} // InitMPU6050Acc

boolean MPU6050AccActive(void) 
{
    F.AccelerationsValid = I2CResponse(MPU6050_ID);
    return( F.AccelerationsValid );
} // MPU6050AccActive

#endif // INC_MPU6050

//________________________________________________________________________________________________

#ifdef INC_BMA180

// Bosch BMA180 Acc

// 0 1g, 1 1.5g, 2 2g, 3 3g, 4 4g, 5 8g, 6 16g
// 0 19Hz, 1 20, 2 40, 3 75, 4 150, 5 300, 6 600, 7 1200Hz 

#define BMA180_RANGE	5
#define BMA180_BW 		4

#define BMA180_Version 0x01
#define BMA180_ACCXLSB 0x02
#define BMA180_TEMPERATURE 0x08
#define BMA180_STATREG1 0x09
#define BMA180_STATREG2 0x0A
#define BMA180_STATREG3 0x0B
#define BMA180_STATREG4 0x0C
#define BMA180_CTRLREG0 0x0D
#define BMA180_CTRLREG1 0x0E
#define BMA180_CTRLREG2 0x0F

#define BMA180_BWTCS 0x20
#define BMA180_CTRLREG3 0x21

#define BMA180_HILOWNFO 0x25
#define BMA180_LOWDUR 0x26
#define BMA180_LOWTH 0x29

#define BMA180_tco_y 0x2F
#define BMA180_tco_z 0x30

#define BMA180_OLSB1 0x35

boolean BMA180AccActive(void);

void ReadBMA180Acc(void) 
{
	static charint16x4u A;

	if ( ReadI2CString(BMA180_ID, BMA180_ACCXLSB, A.c, 6) )
	{
		AccADC[LR] = A.i16[0]; 
		AccADC[DU] = A.i16[2]; 
		AccADC[FB] = A.i16[1];
	}
	else
	{
		AccADC[FB] = AccADC[LR] = 0; AccADC[DU] = GRAVITY;
		if ( State == InFlight )
		{
			Stats[AccFailS]++;	// data over run - acc out of range
			// use neutral values!!!!
			F.AccFailure = true;
		}
	}

} // ReadBMA180Acc

void InitBMA180Acc() {
	
	uint8 i, bw, range, ee_w;
	int16 AccLR, AccDU, AccFB;

	// if connected correctly, ID register should be 3
//	if(read(ID) != 3)
//		return -1;

	ee_w = ReadI2CByteAtAddr(BMA180_ID, BMA180_CTRLREG0);
	ee_w |= 0x10;
	WriteI2CByteAtAddr(BMA180_ID, BMA180_CTRLREG0, ee_w); // Have to set ee_w to write any other registers

	bw = ReadI2CByteAtAddr(BMA180_ID, BMA180_BWTCS);
	bw |= (BMA180_BW << 4) &~0xF0;
	WriteI2CByteAtAddr(BMA180_ID, BMA180_BWTCS, bw); // Keep tcs<3:0> in BWTCS, but write new BW	
		
	range = ReadI2CByteAtAddr(BMA180_ID, BMA180_OLSB1);
	range |= (BMA180_RANGE<<1) & ~0x0E;
	WriteI2CByteAtAddr(BMA180_ID, BMA180_OLSB1, range); //Write new range data, keep other bits the same

} // InitBMA180Acc

boolean BMA180AccActive(void) {

    I2CStart();
		WriteI2CByte(BMA180_ID);
	    F.AccelerationsValid =  ReadI2CByte(I2C_NACK) == BMA180_Version; 
    I2CStop();

    return( F.AccelerationsValid );

} // BMA180AccActive

#endif // INC_BMA180

//________________________________________________________________________________________________

// LISL Acc

#ifdef CLOCK_16MHZ
	#define SPI_HI_DELAY Delay10TCY()
	#define SPI_LO_DELAY Delay10TCY()
#else // CLOCK_40MHZ
	#define SPI_HI_DELAY Delay10TCYx(2)
	#define SPI_LO_DELAY Delay10TCYx(2)
#endif // CLOCK_16MHZ

// LISL-Register mapping

#define	LISL_WHOAMI		(uint8)(0x0f)
#define	LISL_OFFSET_X	(uint8)(0x16)
#define	LISL_OFFSET_Y	(uint8)(0x17)
#define	LISL_OFFSET_Z	(uint8)(0x18)
#define	LISL_GAIN_X		(uint8)(0x19)
#define	LISL_GAIN_Y		(uint8)(0x1A)
#define	LISL_GAIN_Z		(uint8)(0x1B)
#define	LISL_CTRLREG_1	(uint8)(0x20)
#define	LISL_CTRLREG_2	(uint8)(0x21)
#define	LISL_CTRLREG_3	(uint8)(0x22)
#define	LISL_STATUS		(uint8)(0x27)
#define LISL_OUTX_L		(uint8)(0x28)
#define LISL_OUTX_H		(uint8)(0x29)
#define LISL_OUTY_L		(uint8)(0x2A)
#define LISL_OUTY_H		(uint8)(0x2B)
#define LISL_OUTZ_L		(uint8)(0x2C)
#define LISL_OUTZ_H		(uint8)(0x2D)
#define LISL_FF_CFG		(uint8)(0x30)
#define LISL_FF_SRC		(uint8)(0x31)
#define LISL_FF_ACK		(uint8)(0x32)
#define LISL_FF_THS_L	(uint8)(0x34)
#define LISL_FF_THS_H	(uint8)(0x35)
#define LISL_FF_DUR		(uint8)(0x36)
#define LISL_DD_CFG		(uint8)(0x38)
#define LISL_INCR_ADDR	(uint8)(0x40)
#define LISL_READ		(uint8)(0x80)

void SendCommand(int8 c)
{
	static int8 s;

	SPI_IO = WR_SPI;	
	SPI_CS = SEL_LISL;	
	for( s = 8; s; s-- )
	{
		SPI_SCL = 0;
		if( c & 0x80 )
			SPI_SDA = 1;
		else
			SPI_SDA = 0;
		c <<= 1;
		SPI_LO_DELAY;
		SPI_SCL = 1;
		SPI_HI_DELAY;
	}
} // SendCommand

uint8 ReadLISL(uint8 c)
{
	static uint8 d;

	SPI_SDA = 1;	//zzz // very important!! really!! LIS3L likes it
	SendCommand(c);
	SPI_IO = RD_SPI;	// SDA is input
	d = ReadLISLNext();
	
	if( (c & LISL_INCR_ADDR) == (uint8)0 )
		SPI_CS = DSEL_LISL;
	return(d);
} // ReadLISL

uint8 ReadLISLNext(void)
{
	static int8 s;
	static uint8 d;

	for( s = 8; s; s-- )
	{
		SPI_SCL = 0;
		SPI_LO_DELAY;
		d <<= 1;
		if( SPI_SDA == (uint8)1 )
			d |= 1;	
		SPI_SCL = 1;
		SPI_HI_DELAY;
	}
	return(d);
} // ReadLISLNext

void WriteLISL(uint8 d, uint8 c)
{
	static int8 s;

	SendCommand(c);
	for( s = 8; s; s-- )
	{
		SPI_SCL = 0;
		if( d & 0x80 )
			SPI_SDA = 1;
		else
			SPI_SDA = 0;
		d <<= 1;
		SPI_LO_DELAY;
		SPI_SCL = 1;
		SPI_HI_DELAY;
	}
	SPI_CS = DSEL_LISL;
	SPI_IO = RD_SPI;	// IO is input (to allow RS232 reception)
} // WriteLISL

void InitLISLAcc(void)
{
	static int8 r;

	F.AccelerationsValid = false;

	r = ReadLISL(LISL_WHOAMI + LISL_READ);
	if( r == 0x3A )	// a LIS03L sensor is there!
	{
		WriteLISL(0b11000111, LISL_CTRLREG_1); // on always, 40Hz sampling rate,  10Hz LP cutoff, enable all axes
		WriteLISL(0b00000000, LISL_CTRLREG_3);
		WriteLISL(0b01000000, LISL_FF_CFG); // latch, no interrupts; 
		WriteLISL(0b00000000, LISL_FF_THS_L);
		WriteLISL(0b11111100, LISL_FF_THS_H); // -0,5g threshold
		WriteLISL(255, LISL_FF_DUR);
		WriteLISL(0b00000000, LISL_DD_CFG);
		F.AccelerationsValid = true;
	}
	else
		F.AccFailure = true;
} // InitLISLAcc

boolean LISLAccActive(void)
{
	SPI_CS = DSEL_LISL;
	WriteLISL(0b01001010, LISL_CTRLREG_2); // enable 3-wire, BDU=1, +/-2g

	F.AccelerationsValid = ReadLISL(LISL_WHOAMI + LISL_READ) == (uint8)0x3a;

	return ( F.AccelerationsValid );
} // LISLAccActive

void ReadLISLAcc()
{
	static charint16x4u A;

//	while( (ReadLISL(LISL_STATUS + LISL_READ) & 0x08) == (uint8)0 );

	F.AccelerationsValid = ReadLISL(LISL_WHOAMI + LISL_READ) == (uint8)0x3a; // Acc still there?
	if ( F.AccelerationsValid ) 
	{
		A.c[0] = ReadLISL(LISL_OUTX_L + LISL_INCR_ADDR + LISL_READ);
		A.c[1] = ReadLISLNext();
		A.c[2] = ReadLISLNext();
		A.c[3] = ReadLISLNext();
		A.c[4] = ReadLISLNext();
		A.c[5] = ReadLISLNext();
		SPI_CS = DSEL_LISL;	// end transmission
		AccADC[LR] = A.i16[X];
		AccADC[DU] = A.i16[Y];
		AccADC[FB] = A.i16[Z];
	}
	else
	{
		AccADC[LR] = AccADC[FB] = 0; AccADC[DU] = GRAVITY_LISL;
		if ( State == InFlight )
		{
			Stats[AccFailS]++;	// data over run - acc out of range
			// use neutral values!!!!
			F.AccFailure = true;
		}
	}

} // ReadLISLAcc


