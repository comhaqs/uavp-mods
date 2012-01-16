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

// Various controller forms ;)

#define D_LIMIT 32L

void Do_Wolf_Rate(AxisStruct *C)
{
	static i24u Temp24;
	static int32 Temp, r;

	r =  -SRS32((int32)C->Rate * C->Kp, 5);

	Temp = -SRS32((int32)C->Angle * C->Ki, 9);
	r += Limit1(Temp, C->IntLimit);
			
	Temp = SRS32((int32)(C->Rate - C->Ratep) * C->Kd, 3 + PIDCycleShift );
	Temp = Limit1(Temp, D_LIMIT);
	r += Temp;

	Temp24.i24 = r * GS;
	r = Temp24.i2_1;

	C->Out = r - SRS16(C->Control, 2 - PIDCycleShift);

	C->Ratep = C->Rate;
	
} // Do_Wolf_Rate

#if defined MING_P_RATE

void Do_Ming_P_Rate(AxisStruct *C)
{
	static i24u Temp24;
	static int32 r;

	r = SRS32((int32)C->Rate * C->Kp, 5);			
	Temp24.i24 = r * GS;
	r = Temp24.i2_1;

	// My P rate control testing if there is an OL pole s = 0 in OLTF 8/7/2011
	// The idea is keep Greg's code only changing ControlRoll divided by 16 then multiplied Kp
	C->Out = r - SRS32(C->Control * C->Kp, 4); 

} // Do_Ming_P_Rate

#elif defined MING_PI_RATE

void Do_Ming_PI_Rate(AxisStruct *C)
{
	static int32 Temp, RateE, r;
	static i24u Temp24;

	// My PI rate control 11/7/2011	
	RateE  = SRS32(C->Rate * GS + C->Control * RC_STICK_ANGLE_SCALE, 5);
	Temp24.i24 = RateE;
	RateE = Temp24.i2_1;
 
	C->RateIntE += RateE;
	C->RateIntE = Limit1(C->RateIntE, (int16)C->IntLimit);
 	r = SRS32(C->RateIntE * C->Ki, 2 + PIDCycleShiftScale);
	
	C->Out = (RateE * C->Kp) + r;

	C->RateEp = RateE;

} // Do_Ming_PI_Rate

#elif defined PI_P_ANGLE

void Do_PI_P_Angle(AxisStruct *C)
{
	static int32 p, d, DesRate, AngleE, AngleEp, AngleIntE, RateE;

	AngleEp = C->AngleE;
	AngleIntE = C->AngleIntE;

	AngleE = C->Control * RC_STICK_ANGLE_SCALE - C->Angle;
	AngleE = Limit1(AngleE, MAX_BANK_ANGLE_DEG * DEG_TO_ANGLE_UNITS); // limit maximum demanded angle

	p = -SRS32(AngleE * C->Kp, 10);

	d = SRS32((AngleE - AngleEp) * C->Kd, 9);
	d = Limit1(d, D_LIMIT);

	DesRate = p + d;

	RateE = DesRate - C->Rate; 	
	C->Out = SRS32(RateE * C->Kp2, 4 + PIDCycleShift);

	C->AngleE = AngleE;
	C->AngleIntE = AngleIntE;

} // Do_PI_D_Angle

#endif 




