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

// Temperature Burr-Brown TMP101

#include "uavx.h"

void GetTemperature(void);
void InitTemperature(void);

#define TMP100_MAX_ADC 	4095 		// 12 bits
		
#define TMP100_WR		0x96 		// Write
#define TMP100_RD		0x97 		// Read	
#define TMP100_TMP		0x00		// Temperature
#define TMP100_CMD		0x01 	
#define TMP100_LOW		0x02 		// Alarm low limit
#define TMP100_HI		0x03 		// Alarm high limit
#define TMP100_CFG		0b00000000	// 0.5 deg resolution continuous

i16u AmbientTemperature;

void GetTemperature(void)
{
	I2CStart();
	if( WriteI2CByte(TMP100_RD) != I2C_ACK ) goto Terror;
	AmbientTemperature.b1 = ReadI2CByte(I2C_ACK);
	AmbientTemperature.b0 = ReadI2CByte(I2C_NACK);
	I2CStop();

	// Top 9 bits 0.5C res. scale to 0.1C
	AmbientTemperature.i16 = SRS16(	AmbientTemperature.i16, 7) * 5;	 
	if ( AmbientTemperature.i16 > Stats[MaxTempS])
		Stats[MaxTempS] = AmbientTemperature.i16;
	else
		if ( AmbientTemperature.i16 < Stats[MinTempS] )
			Stats[MinTempS] = AmbientTemperature.i16;
	return;

Terror:
	I2CStop();
	AmbientTemperature.i16 = 0;

	return;
} // GetTemperature

void InitTemperature(void)
{
	static uint8 r;

	I2CStart();
	r = WriteI2CByte(TMP100_WR);
	r = WriteI2CByte(TMP100_CMD);
	r = WriteI2CByte(TMP100_CFG);
	I2CStop();

	I2CStart();
	r = WriteI2CByte(TMP100_WR);	
	r = WriteI2CByte(TMP100_TMP);  // Select temperature
	I2CStop();

	GetTemperature();

} // InitTemperature
