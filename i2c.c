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

#define I2C_IN			1
#define I2C_OUT			0

// These routine need much better tailoring to the I2C bus spec.
// 16MHz 0.25uS/Cycle
// 40MHz 0.1uS/Cycle

#ifdef CLOCK_16MHZ
	#define	I2C_DELAY_5US		Delay10TCY();Delay10TCY()
	#define	I2C_DELAY_2US		Delay10TCY()
	#define I2C_DELAY2			Delay1TCY()		
#else // CLOCK_40MHZ
	#define	I2C_DELAY_5US		Delay10TCYx(5)
	#define	I2C_DELAY_2US		Delay10TCYx(2)	
	// ~102KHz (Compass limit!)
	#define I2C_DELAY2		Delay10TCYx(1);Delay1TCY();Delay1TCY();Delay1TCY();Delay1TCY();Delay1TCY()
	// ~210KHz
	//#define I2C_DELAY2	
#endif // CLOCK_16MHZ

void InitI2C(uint8, uint8);
void I2CStart(void);
void I2CStop(void);
boolean I2CWaitClkHi(void); // has timeout
uint8 ReadI2CByte(uint8);
uint8 WriteI2CByte(uint8);
uint8 ScanI2CBus(void);
uint8 ReadI2CString(uint8 *, uint8);

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

#define I2C_DATA_LOW	{I2C_SDA_SW=0;I2C_DELAY2;I2C_DIO_SW=I2C_OUT;I2C_DELAY2;}
#define I2C_DATA_FLOAT	{I2C_DIO_SW=I2C_IN;}
#define I2C_CLK_LOW		{I2C_SCL_SW=0;I2C_DELAY2;I2C_CIO_SW=I2C_OUT;I2C_DELAY2;}
#define I2C_CLK_FLOAT	{I2C_CIO_SW=I2C_IN;I2C_DELAY2;} 

void InitI2C(uint8 sync, uint8 slew)
{
	// null for SW I2C
} // InitI2C

boolean I2CWaitClkHi(void)
{
	static uint8 s;

	I2C_CLK_FLOAT;		// set SCL to input, output a high
	s = 1;
	while( !I2C_SCL_SW )	// timeout wraparound through 255 to 0 0.5mS @ 40MHz
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
	
	I2C_DATA_FLOAT;
	r = I2CWaitClkHi();
	I2C_DATA_LOW;
	I2C_DELAY_5US;
	I2C_CLK_LOW;
} // I2CStart

void I2CStop(void)
{
	static boolean r;

	I2C_DATA_LOW;
	r = I2CWaitClkHi();
	I2C_DATA_FLOAT;

	I2C_DELAY_5US;

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
			I2C_DELAY_2US;
 		}
		else
		{
			Stats[I2CFailS]++;
			return(false);
		}
	} while ( --s );

	I2C_SDA_SW = r;
	I2C_DELAY2;
	I2C_DIO_SW = I2C_OUT;
	I2C_DELAY2;
										
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

uint8 ReadI2CString(uint8 *S, uint8 l)
{
	static uint8 d, b;

	for (b = 0; b < l; b++)
	{
		if ( b < (l-1) )
			d = ReadI2CByte(I2C_ACK);
		else
			d = ReadI2CByte(I2C_NACK);
		S[b] = d;
	}

	return( 0 );
} // ReadI2CString

#ifdef TESTING

uint8 ScanI2CBus(void)
{
	uint8 s;
	uint8 d;

	d = 0;

	TxString("Sensor Bus\r\n");
	for ( s = 0x10 ; s <= 0xf6 ; s += 2 )
	{
		I2CStart();
		if( WriteI2CByte(s) == I2C_ACK )
		{
			d++;
			TxString("\t0x");
			TxValH(s);
			TxNextLine();
		}
		I2CStop();

		Delay1mS(2);
	}

	TxString("\r\nESC Bus\r\n");

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

	return(d);
} // ScanI2CBus

#endif // TESTING

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
			{
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
			}
		ESCI2CStop();
	}
	TxString("\tESC at SLA 0x");
	TxValH(addr);
	TxString(" no response - check cabling and pullup resistors!\r\n");
} // ProgramSlaveAddress

boolean CheckESCBus(void)
{
	return ( true );
} // CheckESCBus

void ConfigureESCs(void)
{
	int8 m;

	if ( (int8)P[ESCType] == ESCYGEI2C )		
	{
		TxString("\r\nProgram YGE ESCs\r\n");
		for ( m = 0 ; m < NoOfI2CESCOutputs ; m++ )
		{
			TxString("Connect ONLY ");
			switch( m )
			{
				#ifdef VTCOPTER
					case 0 : TxString("VLeft"); break;
					case 1 : TxString("VRight");  break;
					case 2 : TxString("Right"); break;
					case 3 : TxString("Left");  break;
				#else
					case 0 : TxString("Front"); break;
					case 1 : TxString("Back");  break;
					case 2 : TxString("Right"); break;
					case 3 : TxString("Left");  break;
				#endif // HEXACOPTER
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




