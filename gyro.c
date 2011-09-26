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

void AdaptiveYawFilterA(void);
void ShowGyroType(void);
void CompensateRollPitchGyros(void);
void GetGyroValues(void);
void CalculateGyroRates(void);
void CheckGyroFault(uint8, uint8, uint8);
void ErectGyros(void);
void GyroTest(void);
void InitGyros(void);

int16 Rate[3], Ratep[3], GyroNeutral[3], FirstGyroADC[3], GyroADC[3];
i32u YawRateF;
int16 YawFilterA;
int8 GyroType;
charint16x4u G;

#include "MPU6050.h"

#include "gyro_i2c.h"
#include "gyro_analog.h"

const rom char * GyroName[GyroUnknown+1] ={
		"MLX90609","ADXRS613/150","IDG300","ST-AY530","ADXRS610/300",
		"ITG3200 SF-3DOF","ITG3200 SF-9DOF","MPU6050","ITG3200 SF-DOF6","IR Sensors",
		"Unknown"
		};

void ShowGyroType(void)
{
	TxString(GyroName[P[SensorHint]]);
} // ShowGyroType

void AdaptiveYawFilterA(void)
{ 
	YawFilterA = 5 + Abs(DesiredYaw);
} // AdaptiveYawFilterA

void GetGyroValues(void)
{
	if ((GyroType == ITG3200Gyro) || (GyroType == SFDOF6) || (GyroType == SFDOF9))
		BlockReadInvenSenseGyro();
	else
		GetAnalogGyroValues();

} // GetGyroValues

void CalculateGyroRates(void)
{
	static i24u RollT, PitchT, YawT;

	Rate[Roll] = GyroADC[Roll] - GyroNeutral[Roll];
	Rate[Pitch] = GyroADC[Pitch] - GyroNeutral[Pitch];
	Rate[Yaw] = (int16)GyroADC[Yaw] - GyroNeutral[Yaw];

	switch ( GyroType ) {
	case IDG300Gyro:// 500 Deg/Sec 
		RollT.i24 = -(int24)Rate[Roll] * 422; // reversed roll gyro sense
		PitchT.i24 = (int24)Rate[Pitch] * 422;
		YawT.i24 = (int24)Rate[Yaw] * 34; // ADXRS150 assumed
		break;
 	case LY530Gyro:// REFERENCE generically 300deg/S 3.3V
		RollT.i24 = (int24)Rate[Roll] * 256;
		PitchT.i24 = (int24)Rate[Pitch] * 256;
		YawT.i24 = (int24)Rate[Yaw] * 128;
		break;
	case MLX90609Gyro:// generically 300deg/S 5V
		RollT.i24 = (int24)Rate[Roll] * 127;
		PitchT.i24 = (int24)Rate[Pitch] * 127;
		YawT.i24 = (int24)Rate[Yaw] * 63;
		break;
	case ADXRS300Gyro:// ADXRS610/300 300deg/S 5V
		RollT.i24 = (int24)Rate[Roll] * 169;
		PitchT.i24 = (int24)Rate[Pitch] * 169;
		YawT.i24 = (int24)Rate[Yaw] * 84;
		break;
	case SFDOF6:
	case SFDOF9:
	case ITG3200Gyro: // Gyro alone or 6&9DOF SF Sensor Stick 73/45
		RollT.i24 = (int24)Rate[Roll] * 11; // 18
		PitchT.i24 = (int24)Rate[Pitch] * 11;
		YawT.i24 = (int24)Rate[Yaw] * 5;
		break;
	case IRSensors:// IR Sensors - NOT IMPLEMENTED IN PIC VERSION
		RollT.i24 = PitchT.i24 = YawT.i24 = 0;
		break;
	case ADXRS150Gyro:// ADXRS613/150 or generically 150deg/S 5V
		RollT.i24 = (int24)Rate[Roll] * 68;
		PitchT.i24 = (int24)Rate[Pitch] * 68;
		YawT.i24 = (int24)Rate[Yaw] * 34;
		break;
	default:;
	} // GyroType

	Rate[Roll] = RollT.i2_1;
	Rate[Pitch] = PitchT.i2_1;
	Rate[Yaw] = YawT.i2_1;

    LPFilter16(&Rate[Yaw], &YawRateF, YawFilterA);

} // CalculateGyroRates

void ErectGyros(void)
{
	int8 i, g;
	int32 Av[3];

	for ( g = 0; g < (int8)3; g++ )	
		Av[g] = 0;

    for ( i = 32; i ; i-- )
	{
		LEDRed_TOG;
		Delay100mSWithOutput(1);

		GetGyroValues();

		Av[Roll] += GyroADC[Roll];
		Av[Pitch] += GyroADC[Pitch];	
		Av[Yaw] += GyroADC[Yaw];
	}
	
	for ( g = 0; g < (int8)3; g++ )
	{
		GyroNeutral[g] = (int16)SRS32( Av[g], 5); // InvenSense is signed
		Rate[g] =  Ratep[g] = Angle[g] = RawAngle[g] = 0;
	}
 
	LEDRed_OFF;

} // ErectGyros

void InitGyros(void)
{
	GyroType = GyroUnknown;

	switch (P[SensorHint]){
	case ITG3200Gyro:
		INV_ID = INV_ID_3DOF;
		INVGyroAddress = INV_GX_H;
		if (InvenSenseGyroActive())
		{
			InitInvenSenseGyro();
			GyroType = ITG3200Gyro;
		}
		else
		{
			INV_ID = INV_ID_6DOF;
			INVGyroAddress = INV_GX_H;
			if (InvenSenseGyroActive())
			{
				InitInvenSenseGyro();
				GyroType = SFDOF6;
			}
		}
		break;
	case SFDOF6: // ITG3200
		INV_ID = INV_ID_6DOF;
		INVGyroAddress = INV_GX_H;
		if (InvenSenseGyroActive())
		{
			InitInvenSenseGyro();
			GyroType = SFDOF6;
		}
		break;
	case SFDOF9: // ITG3200
		INV_ID = INV_ID_6DOF;
		INVGyroAddress = INV_GX_H;
		if (InvenSenseGyroActive())
		{
			InitInvenSenseGyro();
			GyroType = SFDOF9;
		}
		break;
	case MPU6050:
		INV_ID = INV_ID_MPU6050;
		INVGyroAddress = MPU6050_GYRO_XOUT_H;
		if (InvenSenseGyroActive())
		{
			InitInvenSenseGyro();
			GyroType = MPU6050;
		}
		break;	
	default:
		GyroType = P[SensorHint];
		InitAnalogGyros();
		break;
	}

} // InitGyros

#ifdef TESTING
void GyroTest(void)
{
	if (GyroType == ITG3200Gyro)
		GyroInvenSenseTest();
	else
		GyroAnalogTest();
} // GyroTest
#endif // TESTING

 int16 Grav[2], Dyn[2]; // zzz

void CompensateRollPitchGyros(void)
{
	// RESCALE_TO_ACC is dependent on cycle time and is defined in uavx.h
	#define ANGLE_COMP_STEP 6 //25

	#define AccFilter HardFilter // NoFilter

	static int16 NewAcc[3];
	static i32u Temp;

	#ifndef SUPPRESS_ACC

	if( F.AccelerationsValid ) 
	{
		ReadAccelerations();
	
		NewAcc[LR] = AccADC[LR]; // X
		NewAcc[FB] = AccADC[FB]; // Z
		NewAcc[DU] = AccADC[DU]; // Y

		// NeutralLR, NeutralFB, NeutralDU pass through UAVPSet 
		// and come back as MiddleLR etc.

		NewAcc[LR] -= (int16)P[MiddleLR];
		NewAcc[FB] -= (int16)P[MiddleFB];
		NewAcc[DU] -= (int16)P[MiddleDU];

		Acc[LR] = AccFilter(Acc[LR], NewAcc[LR]);
		Acc[FB] = AccFilter(Acc[FB], NewAcc[FB]);
		Acc[DU] = AccFilter(Acc[DU], NewAcc[DU]);
		
		// Roll
	
		Temp.i32 = -(int32)Angle[Roll] * RESCALE_TO_ACC; // avoid shift  32/256 = 0.125 @ 16MHz
		Grav[LR] = Temp.i3_1;
		Dyn[LR] = 0; //Rate[Roll];
	
		IntCorr[LR] = SRS32(Acc[LR] + Grav[LR] + Dyn[LR], 3); 
		IntCorr[LR] = Limit1(IntCorr[LR], ANGLE_COMP_STEP); 
		
		// Pitch
	
		Temp.i32 = -(int32)Angle[Pitch] * RESCALE_TO_ACC; // avoid shift
		Grav[FB] = Temp.i3_1;
		Dyn[FB] = 0; // Rate[Pitch];

		IntCorr[FB] = SRS16(Acc[FB] + Grav[FB] + Dyn[FB], 3); 
		IntCorr[FB] = Limit1(IntCorr[FB], ANGLE_COMP_STEP);
	}	
	else
	#endif // !SUPPRESS_ACC
	{
		IntCorr[LR] = IntCorr[FB] = Acc[LR] = Acc[FB] = 0; Acc[DU] = GRAVITY;
		F.UsingAccComp = false;
	}

} // CompensateRollPitchGyros




