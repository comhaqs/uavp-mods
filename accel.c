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

void ShowAccType(void);
void ReadAccelerations(void);
void GetNeutralAccelerations(void);
void AccelerometerTest(void);
void InitAccelerometers(void);

void ReadADXL345Acc(void);
void InitADXL345Acc(void);
boolean ADXL345AccActive(void);

void SendCommand(int8);
uint8 ReadLISL(uint8);
uint8 ReadLISLNext(void);
void WriteLISL(uint8, uint8);
void InitLISLAcc(void);
boolean LISLAccActive(void);
void ReadLISLAcc(void);

#pragma udata accs
i16u	Ax, Ay, Az;
int16	IntCorr[3];
int8	AccNeutral[3];
int16	Acc[3];
int8 	AccType;
int32	AccDUF;
int16   ADXL345Mag;
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
	case AccUnknown:
		TxString("Unknown");
		break;
	default:;
	} // switch
} // ShowAccType

void ReadAccelerations(void)
{
	if ( AccType == LISLAcc )
		ReadLISLAcc();
	else
	{
		ReadADXL345Acc();
		#ifdef FULL_MONTY
		// rescale to 1024 = 1G
		  	Ax.i16 = ((int24)Ax.i16*1024)/ADXL345Mag; // LR
		  	Ay.i16 = ((int24)Ay.i16*1024)/ADXL345Mag; // DU
		  	Az.i16 = ((int24)Ay.i16*1024)/ADXL345Mag; // FB
		#else
		  	Ax.i16 *= 5; // LR
		  	Ay.i16 *= 5; // DU
		  	Az.i16 *= 5; // FB
		#endif // FULL_MONTY
	}

} // ReadAccelerations

void GetNeutralAccelerations(void)
{
	// this routine is called ONLY ONCE while booting
	// and averages accelerations over 16 samples.
	// Puts values in Neutralxxx registers.
	static uint8 i;
	static int16 AccLR, AccFB, AccDU;

	// already done in caller program
	AccLR = AccFB = AccDU = 0;
	if ( F.AccelerationsValid )
	{
		for ( i = 16; i; i--)
		{
			ReadAccelerations();

			AccLR += Ax.i16;
			AccDU += Ay.i16;
			AccFB += Az.i16;
		}	
	
		AccLR = SRS16(AccLR, 4);
		AccFB = SRS16(AccFB, 4);
		AccDU = SRS16(AccDU, 4);
	
		AccNeutral[LR] = Limit1(AccLR, 99);
		AccNeutral[FB] = Limit1(AccFB, 99);
		AccNeutral[DU] = Limit1(AccDU - 1024, 99); // -1g
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
		TxVal32((int32)Ax.i16, 0, 0);
		if ( Abs((Ax.i16)) > 128 )
			TxString(" fault?");
		TxNextLine();

		TxString("\tF->B: \t");	
		TxVal32((int32)Az.i16, 0, 0);
		if ( Abs((Az.i16)) > 128 )
			TxString(" fault?");	
		TxNextLine();

		TxString("\tD->U:    \t");
	
		TxVal32((int32)Ay.i16, 0, 0);
		if ( ( Ay.i16 < 896 ) || ( Ay.i16 > 1152 ) )
			TxString(" fault?");	
		TxNextLine();

		TxString("\tMag:    \t");
		Mag = int32sqrt(Sqr((int24)Ax.i16)+Sqr((int24)Ay.i16)+Sqr((int24)Az.i16));
		TxVal32((int32)Mag, 0, 0);
		TxNextLine();
	}
	else
		TxString("\r\n(Acc. not present)\r\n");
} // AccelerometerTest

#endif // TESTING

void InitAccelerometers(void)
{
	static int8 i;

	AccNeutral[LR] = AccNeutral[FB] = AccNeutral[DU] = Ax.i16 = Ay.i16 = Az.i16 = 0;

	if ( ADXL345AccActive() )
	{
		AccType = ADXL345Acc;
		InitADXL345Acc();
	}		
	else
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

	if( F.AccelerationsValid )
	{
		LEDYellow_ON;
		GetNeutralAccelerations();
	}
	else
		F.AccFailure = true;
} // InitAccelerometers

//________________________________________________________________________________________________

boolean ADXL345AccActive(void);

#define ADXL345_ID          0xA6
#define ADXL345_W           ADXL345_ID
#define ADXL345_R           (ADXL345_ID+1)

void ReadADXL345Acc(void) {
    static uint8 a;
	static int8 r;
    static char b[6];

    I2CStart();
    r = WriteI2CByte(ADXL345_ID);
    r = WriteI2CByte(0x32); // point to acc data
    I2CStop();

	I2CStart();	
	if( WriteI2CByte(ADXL345_R) != I2C_ACK ) goto SGerror;
	r = ReadI2CString(b, 6);
	I2CStop();

	// Ax LR, Ay DU, Az FB

	if ( P[DesGyroType] == ITG3200DOF9)
	{
		// SparkFun 9DOF breakouts pins forward components up

		// Ax LR
		Ax.b1 = b[1]; Ax.b0 = b[0]; 
		Ax.i16 = -Ax.i16;	
	    
		// Ay DU	
	    Ay.b1 = b[5]; Ay.b0 = b[4];

		// Az FB	
	    Az.b1 = b[3]; Az.b0 = b[2];
	}
	else
	{
		// SparkFun 6DOF & ITG3200 breakouts pins forward components up

		// Ax LR	    	
	    Ax.b1 = b[3]; Ax.b0 = b[2];
	
		// Ay DU
	    Ay.b1 = b[5]; Ay.b0 = b[4];

		// Az FB
		Az.b1 = b[1]; Az.b0 = b[0];	

	}

	return;

SGerror:
	Ax.i16 = Ay.i16 = 0; Az.i16 = 1024;
	if ( State == InFlight )
	{
		Stats[AccFailS]++;	// data over run - acc out of range
		// use neutral values!!!!
		F.AccFailure = true;
	}

} // ReadADXL345Acc

void InitADXL345Acc() {

	uint8 i;
	int16 AccLR, AccDU, AccFB;

    I2CStart();
    WriteI2CByte(ADXL345_W);
    WriteI2CByte(0x2D);  // power register
    WriteI2CByte(0x08);  // measurement mode
    I2CStop();

    Delay1mS(5);

    I2CStart();
    WriteI2CByte(ADXL345_W);
    WriteI2CByte(0x31);  // format
    WriteI2CByte(0x08);  // full resolution, 2g
    I2CStop();

    Delay1mS(5);

    I2CStart();
    WriteI2CByte(ADXL345_W);
    WriteI2CByte(0x2C);  // Rate
    WriteI2CByte(0x0C);  // 400Hz
    I2CStop();

    Delay1mS(5);

	#ifdef FULL_MONTY
	ReadADXL345Acc();
	for ( i = 16; i; i--)
	{
		ReadAccelerations();

		AccLR += Ax.i16;
		AccDU += Ay.i16;
		AccFB += Az.i16;

		Delay1mS(5);
	}

	ADXL345Mag = SRS32(int32sqrt(Sqr(AccLR) + Sqr(AccDU) + Sqr(AccFB)),4);

	#endif // FULL_MONTY

} // InitADXL345Acc

boolean ADXL345AccActive(void) {

    I2CStart();
    F.AccelerationsValid = WriteI2CByte(ADXL345_ID) == I2C_ACK;
    I2CStop();

    return( F.AccelerationsValid );

} // ADXL345AccActive

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
	d=ReadLISLNext();
	
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

//	while( (ReadLISL(LISL_STATUS + LISL_READ) & 0x08) == (uint8)0 );

	F.AccelerationsValid = ReadLISL(LISL_WHOAMI + LISL_READ) == (uint8)0x3a; // Acc still there?
	if ( F.AccelerationsValid ) 
	{
		Ax.b0 = ReadLISL(LISL_OUTX_L + LISL_INCR_ADDR + LISL_READ);
		Ax.b1 = ReadLISLNext();
		Ay.b0 = ReadLISLNext();
		Ay.b1 = ReadLISLNext();
		Az.b0 = ReadLISLNext();
		Az.b1 = ReadLISLNext();
		SPI_CS = DSEL_LISL;	// end transmission
	}
	else
	{
		Ax.i16 = Ay.i16 = Az.i16 = 0;
		if ( State == InFlight )
		{
			Stats[AccFailS]++;	// data over run - acc out of range
			// use neutral values!!!!
			F.AccFailure = true;
		}
	}

} // ReadLISLAcc


