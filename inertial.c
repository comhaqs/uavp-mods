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

void CompensateRollPitchGyros(void);
void CompensateYawGyro(void);
void GetAttitude(void);
void DoAttitudeAngles(void);

int16 YawCorr;

void CompensateRollPitchGyros(void)
{
	// RESCALE_TO_ACC is dependent on cycle time and is defined in uavx.h
	#define ANGLE_COMP_STEP 6 //25

//	#define AccFilter MediumFilter32
	#define AccFilter NoFilter // using chip filters

	static int16 AbsAngle, Grav, Dyn, NewAcc, NewCorr;
	static i32u Temp;
	static uint8 a;
	static AxisStruct *C;

	if ( F.AccelerationsValid && F.AccelerometersEnabled ) 
	{
		ReadAccelerations();

		NewAcc = A[Yaw].AccADC - A[Yaw].AccOffset; 
		A[Yaw].Acc = AccFilter(A[Yaw].Acc, NewAcc);

		for ( a = Roll; a<(uint8)Yaw; a++ )
		{
			C = &A[a];
			AbsAngle = Abs(C->Angle);
			if ( AbsAngle < (30 * DEG_TO_ANGLE_UNITS) ) // ArcSin approximation holds
			{	
				NewAcc = C->AccADC - C->AccOffset; 
				C->Acc = AccFilter(C->Acc, NewAcc);
				Temp.i32 = (int32)C->Angle * P[AccTrack]; 
				Grav = Temp.i3_1;
				Dyn = 0; //A[a].Rate;
		
			    NewCorr = SRS32(C->Acc + Grav + Dyn, 3); 
	
				if ( (State == InFlight) && (AbsAngle > 10 * DEG_TO_ANGLE_UNITS) )
				{
					C->AccCorrAv += Abs(NewCorr);
					C->AccCorrMean += NewCorr;
					NoAccCorr++;
				}
	
				NewCorr = Limit1(NewCorr, ANGLE_COMP_STEP);
				C->DriftCorr = MediumFilter(C->DriftCorr, NewCorr);
			}
			else
				C->DriftCorr = 0;
				
		}
	}	
	else
	{
		A[Roll].Angle = Decay1(A[Roll].Angle);
		A[Pitch].Angle = Decay1(A[Pitch].Angle);
		A[Roll].DriftCorr = A[Roll].Acc = A[Pitch].DriftCorr = A[Pitch].Acc = 0;
		A[Yaw].Acc = GRAVITY;
	}

} // CompensateRollPitchGyros

void DoAttitudeAngles(void)
{	
	// Angles and rates are normal aircraft coordinate conventions
	// X/Forward axis reversed for Acc to simplify compensation
	
	static uint8 a;
	static int16 Temp;
	static AxisStruct *C;

	for ( a = Roll; a <= (uint8)Pitch; a++ )
	{
		C = &A[a];
	
		#ifdef INC_RAW_ANGLES
		C->RawAngle += C->Rate; // for Acc comp studies
		#endif
	
		Temp = C->Angle + C->Rate;	
		Temp = Limit1(Temp, ANGLE_LIMIT); // turn off comp above this angle?
		Temp -= C->DriftCorr;			// last for accelerometer compensation
		C->Angle = Temp;
	}

} // DoAttitudeAngles

void CompensateYawGyro(void)
{ 	// Yaw gyro compensation using compass
	static int16 HE;

	if ( F.CompassValid && F.NormalFlightMode )
	{
		// + CCW
		if ( A[Yaw].Hold > COMPASS_MIDDLE ) // acquire new heading
		{
			DesiredHeading = Heading;
			A[Yaw].Ratep = A[Yaw].Rate;
		}
		else
			if ( F.NewCompassValue )
			{
				F.NewCompassValue = false;
				HE = MinimumTurn(DesiredHeading - Heading);
				HE = Limit1(HE, SIXTHMILLIPI); // 30 deg limit
				A[Yaw].DriftCorr = SRS32((int24)HE * (int24)P[CompassKp], 7);
				A[Yaw].DriftCorr = Limit1(A[Yaw].DriftCorr, YAW_COMP_LIMIT); // yaw gyro drift compensation
			}		
	}

	A[Yaw].Rate -= A[Yaw].DriftCorr;

} // CompensateYawGyro

void GetAttitude(void)
{
	GetGyroValues();
	CalculateGyroRates();
	CompensateRollPitchGyros();
	CompensateYawGyro();
	DoAttitudeAngles();
} // GetAttitude







