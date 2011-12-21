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

// Gyros

#include "uavx.h"

void ShowGyroType(uint8);
void GetGyroValues(void);
void CalculateGyroRates(void);
void CheckGyroFault(uint8, uint8, uint8);
void ErectGyros(void);
void GyroTest(void);
void InitGyros(void);

int16 RawYawRateP;
uint8 GyroType;
int16 RawGyro[3];
int32 AccCorrAv, NoAccCorr;

#include "MPU6050.h"

#include "gyro_i2c.h"
#include "gyro_analog.h"

#pragma idata gyronames
const rom char * GyroName[GyroUnknown+1] ={
		"MLX90609","ADXRS613/150","IDG300","ST-AY530","ADXRS610/300",
		"ITG3200","SF-DOF6","SF-9DOF","MPU6050","FreeIMU","Drotek","IR Sensors",
		"Unknown"
		};
#pragma idata

void ShowGyroType(uint8 G)
{
	TxString(GyroName[G]);
} // ShowGyroType

void CalculateGyroRates(void)
{
	static i24u RollT, PitchT, YawT;

	A[Roll].Rate = A[Roll].GyroADC - A[Roll].GyroBias;
	A[Pitch].Rate = A[Pitch].GyroADC - A[Pitch].GyroBias;
	A[Yaw].Rate = A[Yaw].GyroADC - A[Yaw].GyroBias;

	switch ( GyroType ) {
	case IDG300Gyro:// 500 Deg/Sec 
		RollT.i24 = (int24)A[Roll].Rate * 422; // reversed roll gyro sense
		PitchT.i24 = (int24)A[Pitch].Rate * 422;
		YawT.i24 = (int24)A[Yaw].Rate * 34; // ADXRS150 assumed
		break;
 	case LY530Gyro:// REFERENCE generically 300deg/S 3.3V
		RollT.i24 = (int24)A[Roll].Rate * 256;
		PitchT.i24 = (int24)A[Pitch].Rate * 256;
		YawT.i24 = (int24)A[Yaw].Rate * 128;
		break;
	case MLX90609Gyro:// generically 300deg/S 5V
		RollT.i24 = (int24)A[Roll].Rate * 127;
		PitchT.i24 = (int24)A[Pitch].Rate * 127;
		YawT.i24 = (int24)A[Yaw].Rate * 63;
		break;
	case ADXRS300Gyro:// ADXRS610/300 300deg/S 5V
		RollT.i24 = (int24)A[Roll].Rate * 169;
		PitchT.i24 = (int24)A[Pitch].Rate * 169;
		YawT.i24 = (int24)A[Yaw].Rate * 84;
		break;
	case MPU6050:
	case ITG3200Gyro: // Gyro alone or 6&9DOF SF Sensor Stick 73/45
		RollT.i24 = (int24)A[Roll].Rate * 11; // 18
		PitchT.i24 = (int24)A[Pitch].Rate * 11;
		YawT.i24 = (int24)A[Yaw].Rate * 5;
		break;
	case IRSensors:// IR Sensors - NOT IMPLEMENTED IN PIC VERSION
		RollT.i24 = PitchT.i24 = YawT.i24 = 0;
		break;
	case ADXRS150Gyro:// ADXRS613/150 or generically 150deg/S 5V
		RollT.i24 = (int24)A[Roll].Rate * 68;
		PitchT.i24 = (int24)A[Pitch].Rate * 68;
		YawT.i24 = (int24)A[Yaw].Rate * 34;
		break;
	default:;
	} // GyroType

	A[Roll].Rate = RollT.i2_1;
	A[Pitch].Rate = PitchT.i2_1;
	A[Yaw].Rate = YawT.i2_1;

} // CalculateGyroRates

void ErectGyros(void)
{
	static uint8 i, g;
	static int32 Av[3];

	for ( g = Roll; g <=(uint8)Yaw; g++ )	
		Av[g] = 0;

    for ( i = 32; i ; i-- )
	{
		LEDRed_TOG;
		Delay100mSWithOutput(1);

		GetGyroValues();

		for ( g = Roll; g <= (uint8)Yaw; g++ )
			Av[g] += A[g].GyroADC;
	}
	
	for ( g = Roll; g <= (uint8)Yaw; g++ )
	{
		A[g].GyroBias = (int16)SRS32( Av[g], 5); // InvenSense is signed
		A[g].Rate = A[g].Ratep = A[g].Angle = 0;
	}

	RawYawRateP = 0;
 
	LEDRed_OFF;

} // ErectGyros

void CompensateRollPitchGyros(void)
{
	// RESCALE_TO_ACC is dependent on cycle time and is defined in uavx.h
	#define ANGLE_COMP_STEP 6 //25

	#define AccFilter MediumFilter32

	static int16 NewAcc;
	static int16 Grav, Dyn;
	static i32u Temp;
	static int16 NewCorr;
	static uint8 a;
	static AxisStruct *C;

	if ( F.AccelerationsValid && F.AccelerometersEnabled ) 
	{
		ReadAccelerations();

		A[Yaw].Acc = A[Yaw].AccADC - A[Yaw].AccOffset; // don't bother filtering vertical acc - not used

		for ( a = Roll; a<(uint8)Yaw; a++ )
		{
			C = &A[a];	
			NewAcc = C->AccADC - C->AccOffset; 
			C->Acc = AccFilter(C->Acc, NewAcc);

			Temp.i32 = (int32)C->Angle * P[AccTrack]; // avoid shift  32/256 = 0.125 @ 16MHz
			Grav = Temp.i3_1;
			Dyn = 0; //A[a].Rate;
	
			NewCorr = SRS32(C->Acc + Grav + Dyn, 3); 

			if ( (State == InFlight) && (Abs(C->Angle > 10 * DEG_TO_ANGLE_UNITS)) )
			{
				AccCorrAv += Abs(NewCorr);
				NoAccCorr++;
			}

			NewCorr = Limit1(NewCorr, ANGLE_COMP_STEP);
			C->AngleCorr = MediumFilter(C->AngleCorr,NewCorr); 
		}
	}	
	else
	{
		A[Roll].AngleCorr = A[Roll].Acc = A[Pitch].AngleCorr = A[Yaw].Acc = 0;
		A[Yaw].Acc = GRAVITY;
	}

} // CompensateRollPitchGyros

void GetGyroValues(void)
{
	switch ( P[SensorHint] ) {
	case ITG3200Gyro:
		BlockReadInvenSenseGyro();
		A[Roll].GyroADC = -RawGyro[X];
		A[Pitch].GyroADC = RawGyro[Y];
		A[Yaw].GyroADC = -RawGyro[Z];
		break;	
	case SFDOF6:
		BlockReadInvenSenseGyro();
		A[Roll].GyroADC = -RawGyro[X];
		A[Pitch].GyroADC = RawGyro[Y];
		A[Yaw].GyroADC = -RawGyro[Z];
		break;
	case SFDOF9: 
		BlockReadInvenSenseGyro();
		A[Roll].GyroADC = RawGyro[X];
		A[Pitch].GyroADC = -RawGyro[Y];
		A[Yaw].GyroADC = -RawGyro[Z];
		break;
	case FreeIMU:
		BlockReadInvenSenseGyro();
		A[Roll].GyroADC = -RawGyro[Y]; // not done yet
		A[Pitch].GyroADC = -RawGyro[X];
		A[Yaw].GyroADC = -RawGyro[Z];
		break;
	case Drotek:
		BlockReadInvenSenseGyro();
		A[Roll].GyroADC = -RawGyro[X];
		A[Pitch].GyroADC = RawGyro[Y];
		A[Yaw].GyroADC = -RawGyro[Z];
		break;
	#ifdef INC_MPU6050
	case MPU6050:
		BlockReadInvenSenseGyro();
		A[Roll].GyroADC = -RawGyro[Y];
		A[Pitch].GyroADC = -RawGyro[X];
		A[Yaw].GyroADC = RawGyro[Z];
		break;
	#endif // INC_MPU6050	
	default:
		GetAnalogGyroValues();
		break;
	} // switch
} // GetGyroValues

void InitGyros(void)
{
	switch ( P[SensorHint]){
	case ITG3200Gyro:
		INV_ID = INV_ID_3DOF;
		INVGyroAddress = INV_GX_H;
		if (InvenSenseGyroActive())
		{
			GyroType = ITG3200Gyro;
			InitInvenSenseGyro();
		}
		break;
	case SFDOF6: // ITG3200
	case SFDOF9:
	case FreeIMU:
	case Drotek:
		INV_ID = INV_ID_3DOF;
		INVGyroAddress = INV_GX_H;
		if (InvenSenseGyroActive())
		{
			GyroType = ITG3200Gyro;
			InitInvenSenseGyro();
		}
		break;
	#ifdef INC_MPU6050
	case MPU6050:
		INV_ID = INV_ID_MPU6050;
		INVGyroAddress = MPU6050_GYRO_XOUT_H;
		if (InvenSenseGyroActive())
		{
			GyroType = MPU6050;
			InitInvenSenseGyro();
		}
		break;
	#endif // INC_MPU6050
	default:
		InitAnalogGyros();
		GyroType = P[SensorHint];
		break;
	} // switch

} // InitGyros

#ifdef TESTING
void GyroTest(void)
{
	TxString("\r\n");
	ShowGyroType(GyroType);
	TxString(" - Gyro test:\r\n");

	GetGyroValues();

	if ( !F.GyroFailure )
	{
		TxString("\tRoll:     \t");TxVal32(A[Roll].GyroADC,0,0);
		TxString("\r\n\tPitch:\t");TxVal32(A[Pitch].GyroADC,0,0);
		TxString("\r\n\tYaw:  \t");TxVal32(A[Yaw].GyroADC,0,0);
		TxNextLine();
	}
	else
		TxString("\r\n(Gyro read FAIL)\r\n");	
} // GyroTest
#endif // TESTING





