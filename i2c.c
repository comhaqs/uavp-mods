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

// I2C Routines - Compass is the limiting device at 100KHz!

#include "uavx.h"

void InitI2C(void);
void I2CStart(void);
void I2CStop(void);
boolean I2CWaitClkHi(void); // has timeout
uint8 ReadI2CByte(uint8);
uint8 WriteI2CByte(uint8);
void ShowI2CDeviceName(uint8);
uint8 ScanI2CBus(void);
uint8 ReadI2CByteAtAddr(uint8, uint8);
void WriteI2CByteAtAddr(uint8, uint8, uint8);
boolean ReadI2Ci16v(uint8, uint8, int16 *, uint8);
boolean I2CResponse(uint8);

#define I2C_IN			1
#define I2C_OUT			0

// These routine need much better tailoring to the I2C bus spec.
// 16MHz 0.25uS/Cycle
// 40MHz 0.1uS/Cycle

#ifdef CLOCK_16MHZ
	#define	I2C_DELAY_5US		Delay10TCY();Delay10TCY()	
#else // CLOCK_40MHZ
	#define	I2C_DELAY_5US		Delay10TCYx(5)	
#endif // CLOCK_16MHZ

#ifdef UAVX_HW
	#define I2C_SDA_SW			PORTCbits.RC4
	#define I2C_DIO_SW			TRISCbits.TRISC4
	#define I2C_SCL_SW			PORTCbits.RC3
	#define I2C_CIO_SW			TRISCbits.TRISC3
#else
	#define I2C_SDA_SW			PORTBbits.RB6
	#define I2C_DIO_SW			TRISBbits.TRISB6
	#define I2C_SCL_SW			PORTBbits.RB7
	#define I2C_CIO_SW			TRISBbits.TRISB7
#endif // UAVX_HW

#define I2C_DATA_LOW	{I2C_SDA_SW=0;I2C_DIO_SW=I2C_OUT;} 
#define I2C_DATA_FLOAT	{I2C_DIO_SW=I2C_IN;}
#define I2C_CLK_LOW		{I2C_SCL_SW=0;I2C_CIO_SW=I2C_OUT;}
#define I2C_CLK_FLOAT	{I2C_CIO_SW=I2C_IN;}

void InitI2C(void)
{
	// null for SW I2C
} // InitI2C

boolean I2CWaitClkHi(void)
{
	static uint8 s;

	Delay1TCY();Delay1TCY();Delay1TCY();Delay1TCY(); // tSU:DAT + call
	I2C_CLK_FLOAT;		// set SCL to input, output a high
	s = 0;
	while( !I2C_SCL_SW )	// timeout wraparound through 255 to 0 0.5mS @ 16MHz
		if( ++s == (uint8)0 )
		{
			Stats[I2CFailS]++;
			return (false);
		}
	return( true );
} // I2CWaitClkHi

void I2CStart(void)
{
	static boolean r;
	
	I2C_DATA_FLOAT; // should be floating after previous TBuf
	r = I2CWaitClkHi();
	I2C_DATA_LOW;
	Delay1TCY();Delay1TCY();Delay1TCY();Delay1TCY();Delay1TCY();Delay1TCY(); // tHD:STA
	I2C_CLK_LOW;
} // I2CStart

void I2CStop(void)
{
	static boolean r;

	I2C_DATA_LOW;
	r = I2CWaitClkHi();
	Delay1TCY();Delay1TCY();Delay1TCY(); // tsu:STO
	I2C_DATA_FLOAT;

	Delay10TCYx(13); // TBuf

} // I2CStop 

uint8 ReadI2CByte(uint8 r)
{
	static uint8 s, d;

	I2C_DATA_FLOAT;
	d = 0;
	s = 8;
	do {
		if( I2CWaitClkHi() )
		{ 
			d <<= 1;
			if( I2C_SDA_SW ) d |= 1;
			I2C_CLK_LOW;
 		}
		else
		{
			Stats[I2CFailS]++;
			return(false);
		}
	} while ( --s );

	I2C_SDA_SW = r;
	I2C_DIO_SW = I2C_OUT;
										
	if( I2CWaitClkHi() )
	{
		I2C_CLK_LOW;
		return(d);
	}
	else
	{
		Stats[I2CFailS]++;
		return(false);
	}
	
} // ReadI2CByte

uint8 WriteI2CByte(uint8 d)
{
	static uint8 s, dd;

	dd = d;  // a little faster
	s = 8;
	do {
		if( dd & 0x80 )
			I2C_DATA_FLOAT
		else
			I2C_DATA_LOW
	
		if( I2CWaitClkHi() )
		{ 	
			I2C_CLK_LOW;
			dd <<= 1;
		}
		else
		{
			Stats[I2CFailS]++;
			return(I2C_NACK);
		}
	} while ( --s );

	I2C_DATA_FLOAT;
	if( I2CWaitClkHi() )
		s = I2C_SDA_SW;
	else
	{
		Stats[I2CFailS]++;
		return(I2C_NACK);
	}	

	I2C_CLK_LOW;

	return(s);
} // WriteI2CByte

uint8 ReadI2CByteAtAddr(uint8 d, uint8 address)
{
	static uint8 data;
		
	I2CStart();
		if( WriteI2CByte(d) != I2C_ACK ) goto IRerror;
		if( WriteI2CByte(address) != I2C_ACK ) goto IRerror;
	I2CStart();
		if( WriteI2CByte(d | 1) != I2C_ACK ) goto IRerror;	
		data = ReadI2CByte(I2C_NACK);
	I2CStop();
	
	return ( data );

IRerror:
	I2CStop();
	Stats[I2CFailS]++;
	return (0);
} // ReadI2CByteAtAddr

void WriteI2CByteAtAddr(uint8 d, uint8 address, uint8 v)
{
	I2CStart();	// restart
		if( WriteI2CByte(d) != I2C_ACK ) goto IWerror;
		if( WriteI2CByte(address) != I2C_ACK ) goto IWerror;
		if(WriteI2CByte(v) != I2C_ACK ) goto IWerror;
	I2CStop();
	return;

IWerror:
	I2CStop();
	Stats[I2CFailS]++;
	return;
} // WriteI2CByteAtAddr

boolean ReadI2Ci16v(uint8 d, uint8 cmd, int16 *v, uint8 l)
{
	static uint8 b, c;
	static uint8 S[16];
	static i16u t;

	I2CStart();
	if( WriteI2CByte(d) != I2C_ACK ) goto IRSerror;

	if( WriteI2CByte(cmd) != I2C_ACK ) goto IRSerror;

	I2CStart();	
		if( WriteI2CByte(d | 1) != I2C_ACK ) goto IRSerror;
		for (b = 0; b < l*2; b++)
			if ( b < (l-1) )
				S[b] = ReadI2CByte(I2C_ACK);
			else
				S[b] = ReadI2CByte(I2C_NACK);
	I2CStop();

	// fix endian!
	c = 0;
	for ( b = 0; b < l; b++)
	{
		t.b1 = S[c++];
		t.b0 = S[c++];
		v[b] = t.i16;
	}
	
	return( true );

IRSerror:
	I2CStop();

	for (b = 0; b < l; b++)
		v[b] = 0;

	return(false);

} // ReadI2Ci16v

boolean I2CResponse(uint8 d)
{
	static boolean r;

    I2CStart();
 	   r = WriteI2CByte(d) == I2C_ACK;
    I2CStop();

	return (r);
} // I2CResponse

// -----------------------------------------------------------

// SW I2C Routines for ESCs

boolean ESCWaitClkHi(void);
void ESCI2CStart(void);
void ESCI2CStop(void);

uint8 WriteESCI2CByte(uint8);
void ProgramSlaveAddress(uint8);
void ConfigureESCs(void);

// Constants

#define	 ESC_SDA		PORTBbits.RB1
#define	 ESC_SCL		PORTBbits.RB2
#define	 ESC_DIO		TRISBbits.TRISB1
#define	 ESC_CIO		TRISBbits.TRISB2

#define ESC_DATA_LOW	{ESC_SDA=0;ESC_DIO=I2C_OUT;}
#define ESC_DATA_FLOAT	{ESC_DIO=I2C_IN;}
#define ESC_CLK_LOW		{ESC_SCL=0;ESC_CIO=I2C_OUT;}
#define ESC_CLK_FLOAT	{ESC_CIO=I2C_IN;}

boolean ESCWaitClkHi(void)
{
	static uint8 s;

	ESC_CLK_FLOAT;
	s = 1;						
	while( !ESC_SCL )
		if( ++s == (uint8)0 ) return (false);					

	return ( true );
} // ESCWaitClkHi

void ESCI2CStart(void)
{
	static uint8 r;

	ESC_DATA_FLOAT;
	r = ESCWaitClkHi();
	ESC_DATA_LOW;
	I2C_DELAY_5US;		
	ESC_CLK_LOW;				
} // ESCI2CStart

void ESCI2CStop(void)
{
	ESC_DATA_LOW;
	ESCWaitClkHi();
	ESC_DATA_FLOAT;

    I2C_DELAY_5US;

} // ESCI2CStop

uint8 WriteESCI2CByte(uint8 d)
{ // ~320KHz @ 40MHz
	static uint8 s, t, dd;

	dd = d; // a little faster
	s = 8;
	do {
		if( dd & 0x80 )
			ESC_DATA_FLOAT
		else
			ESC_DATA_LOW
	
		if( ESCWaitClkHi() )
		{ 	
			ESC_CLK_LOW;
			dd <<= 1;
		}
		else
			return(I2C_NACK);	
	} while ( --s );

	ESC_DATA_FLOAT;
	if( ESCWaitClkHi() )
		s = ESC_SDA;
	else
		return(I2C_NACK);	
	ESC_CLK_LOW;

	return( s);
} // WriteESCI2CByte

#ifdef TESTING

void ShowI2CDeviceName(uint8 d) {

	// could be a full table lookup?

    TxChar(' ');
    switch ( d  ) {
        case ADXL345_ID:
            TxString("ADXL345 (SF-6&9DOF)");
            break;
        case BMA180_ID:
            TxString("BMA180 Acc");
            break;
        case INV_ID_3DOF:
            TxString("ITG3200 Gyro (SF-3DOF/MPU6050)");
            break;
        case INV_ID_6DOF:
            TxString("ITG3200 Gyro (SF-6&9DOF)");
            break;
        case HMC5843_3DOF:
            TxString("HMC5843 Mag");
            break;
        case HMC5843_9DOF:
            TxString("HMC5843 Mag (SF-9DOF)");
            break;
    //    case MPU6050_ID:
    //        TxString("MPU6050");
    //        break;
        case HMC6352_ID:
            TxString("HMC6352 Compass");
            break;
        case ADS7823_ID:
            TxString("ADS7823 ADC");
            break;
        case MCP4725_ID:
            TxString("MCP4725 DAC");
            break;
        case BOSCH_ID:
            TxString("Bosch Baro");
            break;
        case TMP100_ID:
            TxString("TMP100 Temp");
            break;
        default:
            break;
    } // switch
    TxChar(' ');

} // ShowI2CDeviceName

uint8 ScanI2CBus(void)
{
	uint8 s;
	uint8 d;

	d = 0;

	TxString("Sensor Bus\r\n");
	for ( s = 0x10 ; s <= 0xf6 ; s += 2 )
	{
		if( I2CResponse(s) )
		{
			d++;
			TxString("\t0x");
			TxValH(s);
			ShowI2CDeviceName(s);
			TxNextLine();
		}
		Delay1mS(2);
	}

/*

	TxString("\r\nESC Bus\r\n");

	d = 0;

	if ( (P[ESCType] == ESCHolger)||(P[ESCType] == ESCX3D)||(P[ESCType] == ESCYGEI2C)||(P[ESCType] == ESCLRCI2C))
		for ( s = 0x10 ; s <= 0xf6 ; s += 2 )
		{
			ESCI2CStart();
			if( WriteESCI2CByte(s) == I2C_ACK )
			{
				d++;
				TxString("\t0x");
				TxValH(s);
				TxNextLine();
			}
			ESCI2CStop();
	
			Delay1mS(2);
		}
	else
		TxString("\tinactive - I2C ESCs not selected..\r\n");
	TxNextLine();
*/

	return(d);
} // ScanI2CBus

void ProgramSlaveAddress(uint8 addr)
{
	static uint8 s;

	for (s = 0x10 ; s < 0xf0 ; s += 2 )
	{
		ESCI2CStart();
		if( WriteESCI2CByte(s) == I2C_ACK )
			if( s == addr )
			{	// ESC is already programmed OK
				ESCI2CStop();
				TxString("\tESC at SLA 0x");
				TxValH(addr);
				TxString(" is already programmed OK\r\n");
				return;
			}
			else
				if( WriteESCI2CByte(0x87) == I2C_ACK ) // select register 0x07
					if( WriteESCI2CByte( addr ) == I2C_ACK ) // new slave address
					{
						ESCI2CStop();
						TxString("\tESC at SLA 0x");
						TxValH(s);
						TxString(" reprogrammed to SLA 0x");
						TxValH(addr);
						TxNextLine();
						return;
					}
		ESCI2CStop();
	}
	TxString("\tESC at SLA 0x");
	TxValH(addr);
	TxString(" no response - check cabling and pullup resistors!\r\n");
} // ProgramSlaveAddress

void ConfigureESCs(void)
{
	int8 m;

	if ( (int8)P[ESCType] == ESCYGEI2C )		
	{
		TxString("\r\nProgram YGE ESCs\r\n");
		for ( m = 0 ; m < NO_OF_I2C_ESCS ; m++ )
		{
			TxString("Connect ONLY ");
			switch( m ) {
			#ifdef Y6COPTER
				case 0 : TxString("FrontT"); break;
				case 1 : TxString("LeftT");  break;
				case 2 : TxString("RightT"); break;
				case 3 : TxString("FrontB"); break;
				case 4 : TxString("LeftB"); break;
				case 5 : TxString("RightB"); break;
			#else
				#ifdef HEXACOPTER
					case 0 : TxString("Front"); break;
					case 1 : TxString("LeftFront");  break;
					case 2 : TxString("RightFront"); break;
					case 3 : TxString("LeftBack"); break;
					case 4 : TxString("RightBack"); break;
					case 5 : TxString("Back"); break;
				#else
					#ifdef TRICOPTER
						case 1 : TxString("Left");  break;
						case 2 : TxString("Right"); break;
					#else
						case 0 : TxString("Front"); break;
						case 1 : TxString("Left");  break;
						case 2 : TxString("Right"); break;
						case 3 : TxString("Back"); break;
					#endif // TRICOPTER
				#endif // HEXACOPTER
			#endif // Y6COPTER
			}
			TxString(" ESC, then the CONTINUE button \r\n");
			while( PollRxChar() != 'x' ); // UAVPSet uses 'x' for CONTINUE button
		//	TxString("\r\n");
			ProgramSlaveAddress( 0x62 + ( m*2 ));
		}
		TxString("\r\nConnect ALL ESCs and power-cycle the Quadrocopter\r\n");
	}
	else
		TxString("\r\nYGEI2C not selected as ESC?\r\n");
} // ConfigureESCs

#endif // TESTING


