// =================================================================================================
// =                                  UAVX Quadrocopter Controller                                 =
// =                             Copyright (c) 2008 by Prof. Greg Egan                             =
// =                   Original V3.15 Copyright (c) 2007 Ing. Wolfgang Mahringer                   =
// =                       http://code.google.com/p/uavp-mods/ http://uavp.ch                      =
// =================================================================================================

//    This is part of UAVX.

//    UAVX is free software: you can redistribute it and/or modify it under the terms of the GNU 
//    General Public License as published by the Free Software Foundation, either version 3 of the 
//    License, or (at your option) any later version.

//    UAVX is distributed in the hope that it will be useful,but WITHOUT ANY WARRANTY; without even 
//    the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
//    General Public License for more details.

//    You should have received a copy of the GNU General Public License along with this program.  
//    If not, see http://www.gnu.org/licenses/

#include "uavx.h"

void MapRC(void);
void ReadParametersEE(void);
void WriteParametersEE(uint8);
void UseDefaultParameters(void);
void UpdateWhichParamSet(void);
void InitParameters(void);

const rom uint8 ESCLimits [] = { OUT_MAXIMUM, OUT_HOLGER_MAXIMUM, OUT_X3D_MAXIMUM, OUT_YGEI2C_MAXIMUM };
const rom int8	ComParms[]={ // mask giving common variables across parameter sets
	0,0,0,1,0,0,0,0,0,0,
	0,0,0,0,0,1,1,1,0,1,
	1,1,1,1,1,0,0,0,0,0,
	0,0,0,1,1,1,1,0,0,1,
	0,1,1,1,1,0,1,0,0,1,
	1,0,0,0,0,0,0,0,0,0,
	0,0,0,0
	};

const rom int8 DefaultParams[] = {
	-24, 			// RollKp, 			01
	-14, 			// RollKi,			02
	75, 			// RollKd,			03
	0, 				// HorizDampKp,		04c 
	3, 				// RollIntLimit,	05
	-24, 			// PitchKp,			06
	-14, 			// PitchKi,			07
	75, 			// PitchKd,			08
	4, 				// AltKp,			09
	3, 				// PitchIntLimit,	10
	
	-30, 			// YawKp, 			11
	-20, 			// YawKi,			12
	0, 				// YawKd,			13
	30, 			// YawLimit,		14
	2, 				// YawIntLimit,		15
	0, 				// ConfigBits,		16c
	4, 				// TimeSlots,		17c
	48, 			// LowVoltThres,	18c
	24, 			// CamRollKp,		19
	45, 			// PercentHoverThr,	20c 
	
	-1, 			// VertDampKp,		21c
	0, 				// MiddleDU,		22c
	10, 			// PercentIdleThr,	23c
	0, 				// MiddleLR,		24c
	0, 				// MiddleFB,		25c
	24, 			// CamPitchKp,		26
	24, 			// CompassKp,		27
	10, 			// AltKi,			28
	90, 			// NavRadius,		29
	8, 				// NavKi,			30 

	0, 				// unused1,			31
	0, 				// unused2,			32
	20, 			// NavRTHAlt,		33
	0, 				// NavMagVar,		34c
	ADXRS300, 		// GyroType,		35c
	ESCPPM, 		// ESCType,			36c
	DX7AR7000, 		// TxRxType			37c
	2,				// NeutralRadius	38
	30,				// PercentNavSens6Ch	39
	0,				// CamRollTrim,		40c

	-16,			// NavKd			41
	1,				// VertDampDecay    42c
	1,				// HorizDampDecay	43c
	70,				// BaroScale		44c
	0,				// TelemetryType	45c
	-10,			// MaxDescentRateDmpS 	46
	30,				// DescentDelayS	47c
	12,				// NavIntLimit		48
	3,				// AltIntLimit		49
	11,				// GravComp			50c
	1,				// CompSteps		51c			

	0,				// 52 - 64 unused currently	

	0,
	0,
	0,
	0,
	0,	
	0,
	0,
	0,

	0,
	0,
	0,
	0					
	};

const rom uint8 Map[CustomTxRx+1][CONTROLS] = {
	{ 3,1,2,4,5,6,7 }, 	// Futaba Thr 3 Throttle
	{ 2,1,4,3,5,6,7 },	// Futaba Thr 2 Throttle
	{ 5,3,2,1,6,4,7 },	// Futaba 9C Spektrum DM8/AR7000
	{ 1,2,3,4,5,6,7 },	// JR XP8103/PPM
	{ 7,1,4,6,3,5,2 },	// JR 9XII Spektrum DM9 ?

	{ 6,1,4,7,3,2,5 },	// JR DXS12 
	{ 6,1,4,7,3,2,5 },	// Spektrum DX7/AR7000
	{ 5,1,4,6,3,2,7 },	// Spektrum DX7/AR6200

	{ 3,1,2,4,5,7,6 }, 	// Futaba Thr 3 Sw 6/7
	{ 1,2,3,4,5,6,7 },	// Spektrum DX7/AR6000
	{ 1,2,3,4,5,6,7 },	// Graupner MX16S

	{ 1,2,3,4,5,6,7 }	// Custom
	};

// Rx signalling polarity used only for serial PPM frames usually
// by tapping internal Rx circuitry.
const rom boolean PPMPosPolarity[CustomTxRx+1] =
	{
		false, 	// Futaba Ch3 Throttle
		false,	// Futaba Ch2 Throttle
		true,	// Futaba 9C Spektrum DM8/AR7000
		true,	// JR XP8103/PPM
		true,	// JR 9XII Spektrum DM9/AR7000

		true,	// JR DXS12
		true,	// Spektrum DX7/AR7000
		true,	// Spektrum DX7/AR6200
		false,	// Futaba Thr 3 Sw 6/7
		true,	// Spektrum DX7/AR6000
		true,	// Graupner MX16S
		true	// custom Tx/Rx combination
	};

// Reference Internal Quadrocopter Channel Order
// 1 Throttle
// 2 Aileron
// 3 Elevator
// 4 Rudder
// 5 Gear
// 6 Aux1
// 7 Aux2

uint8	ParamSet;
boolean ParametersChanged;
int8 RMap[CONTROLS];
#pragma udata params
int8 P[MAX_PARAMETERS];
#pragma udata

void MapRC(void)
{  // re-maps captured PPM to Rx channel sequence
	static uint8 c;
	static int16 LastThrottle, Temp, i; 

	LastThrottle = RC[ThrottleC];

	for (c = 0 ; c < RC_CONTROLS ; c++)
	{
		i = Map[P[TxRxType]][c]-1;
		#ifdef CLOCK_16MHZ
		Temp = PPM[i].b0; // clip to bottom byte 0..255
		#else // CLOCK_40MHZ
		Temp = ( (int32)PPM[i].i16 * RC_MAXIMUM + 625L )/1250L; // scale to 4uS res. for now
		#endif // CLOCK_16MHZ
		RC[c] = RxFilter(RC[c], Temp);		
	}

	if ( THROTTLE_SLEW_LIMIT > 0 )
		RC[ThrottleC] = SlewLimit(LastThrottle, RC[ThrottleC], THROTTLE_SLEW_LIMIT);

} // MapRC

void ReadParametersEE(void)
{
	uint8 i;
	static uint16 a;

	if ( ParametersChanged )
	{   // overkill if only a single parameter has changed but is not in flight loop
		a = (ParamSet - 1)* MAX_PARAMETERS;	
		for ( i = 0; i < MAX_PARAMETERS; i++)
			P[i] = ReadEE(a + i);

		ESCMax = ESCLimits[P[ESCType]];
		if ( P[ESCType] == ESCPPM )
			ESCMin = 1;
		else
		{
			ESCMin = 0;
			for ( i = 0; i < NoOfMotors; i++ )
				ESCI2CFail[i] = false;
			InitI2CESCs();
		}

		#ifdef RX6CH
		NavSensitivity = ((int16)P[PercentNavSens6Ch] * RC_MAXIMUM)/100L;
		#endif // RX6CH

		for ( i = 0; i < CONTROLS; i++) // make reverse map
			RMap[Map[P[TxRxType]][i]-1] = i+1;
	
		IdleThrottle = ((int16)P[PercentIdleThr] * OUT_MAXIMUM )/100L;
		IdleThrottle = Max( IdleThrottle, RC_THRES_STOP );
		HoverThrottle = ((int16)P[PercentHoverThr] * OUT_MAXIMUM )/100L;
	
		RollIntLimit256 = (int16)P[RollIntLimit] * 256L;
		PitchIntLimit256 = (int16)P[PitchIntLimit] * 256L;
		YawIntLimit256 = (int16)P[YawIntLimit] * 256L;
	
		NavNeutralRadius = Limit((int16)P[NeutralRadius], 0, NAV_MAX_NEUTRAL_RADIUS);
		NavClosingRadius = Limit((int16)P[NavRadius], NAV_MAX_NEUTRAL_RADIUS+1, NAV_MAX_RADIUS);

		NavNeutralRadius = ConvertMToGPS(NavNeutralRadius); 
		NavClosingRadius = ConvertMToGPS(NavClosingRadius);
		NavCloseToNeutralRadius = NavClosingRadius - NavNeutralRadius;

		CompassOffset = (((COMPASS_OFFSET_DEG - (int16)P[NavMagVar])*MILLIPI)/180L);
		F.UsingXMode = ( (P[ConfigBits] & FlyXModeMask) != 0);
		if ( F.UsingXMode )
			CompassOffset -= QUARTERMILLIPI;
	
		F.UsingSerialPPM = ((P[ConfigBits] & RxSerialPPMMask) != 0);
		PIE1bits.CCP1IE = false;
		DoRxPolarity();
		PPM_Index = PrevEdge = 0;
		PIE1bits.CCP1IE = true;

		F.UsingTxMode2 = ((P[ConfigBits] & TxMode2Mask) != 0);
		F.UsingGPSAlt = ((P[ConfigBits] & UseGPSAltMask) != 0);
		F.UsingRTHAutoDescend = ((P[ConfigBits] & UseRTHDescendMask) != 0);
		NavRTHTimeoutmS = (uint24)P[DescentDelayS]*1000L;
		DescentCmpS = (int16)P[MaxDescentRateDmpS] * 10L; // cm/S

		BatteryVoltsADC = (int16)P[LowVoltThres];
		
		ParametersChanged = false;
	}
	
} // ReadParametersEE

void WriteParametersEE(uint8 s)
{
	uint8 p;
	uint8 b;
	uint16 addr;
	
	addr = (s - 1)* MAX_PARAMETERS;
	for ( p = 0; p < MAX_PARAMETERS; p++)
		WriteEE( addr + p,  P[p]);
} // WriteParametersEE

void UseDefaultParameters(void)
{ // loads a representative set of initial parameters as a base for tuning
	uint8 p;

	for ( p = 0; p < MAX_PARAMETERS; p++ )
		P[p] = DefaultParams[p];

	WriteParametersEE(1);
	WriteParametersEE(2);
	ParamSet = 1;
	TxString("\r\nDefault Parameters Loaded\r\n");
	TxString("Do a READ CONFIG to refresh the UAVPSet parameter display\r\n");	
} // UseDefaultParameters

void UpdateParamSetChoice(void)
{
	#define STICK_WINDOW 30

	int8 NewParamSet, NewNavAltitudeHold, NewTurnToWP, Selector;

	NewParamSet = ParamSet;
	NewNavAltitudeHold = F.NavAltitudeHold;
	NewTurnToWP = F.TurnToWP;

	if ( F.UsingTxMode2 )
		Selector = DesiredRoll;
	else
		Selector = -DesiredYaw;

	if ( (Abs(DesiredPitch) > STICK_WINDOW) && (Abs(Selector) > STICK_WINDOW) )
	{
		if ( DesiredPitch > STICK_WINDOW ) // bottom
		{
			if ( Selector < -STICK_WINDOW ) // left
			{ // bottom left
				NewParamSet = 1;
				NewNavAltitudeHold = true;
			}
			else
				if ( Selector > STICK_WINDOW ) // right
				{ // bottom right
					NewParamSet = 2;
					NewNavAltitudeHold = true;
				}
		}		
		else
			if ( DesiredPitch < -STICK_WINDOW ) // top
			{		
				if ( Selector < -STICK_WINDOW ) // left
				{
					NewNavAltitudeHold = false;
					NewParamSet = 1;
				}
				else 
					if ( Selector > STICK_WINDOW ) // right
					{
						NewNavAltitudeHold = false;
						NewParamSet = 2;
					}
			}

		if ( ( NewParamSet != ParamSet ) || ( NewNavAltitudeHold != F.NavAltitudeHold ) )
		{	
			ParamSet = NewParamSet;
			F.NavAltitudeHold = NewNavAltitudeHold;
			LEDBlue_ON;
			DoBeep100mSWithOutput(2, 2);
			if ( ParamSet == 2 )
				DoBeep100mSWithOutput(2, 2);
			if ( F.NavAltitudeHold )
				DoBeep100mSWithOutput(4, 4);
			ParametersChanged |= true;
			Beeper_OFF;
			LEDBlue_OFF;
		}
	}

	if ( F.UsingTxMode2 )
		Selector = -DesiredYaw;
	else
		Selector = DesiredRoll;

	if ( (Abs(RC[ThrottleC]) < STICK_WINDOW) && (Abs(Selector) > STICK_WINDOW ) )
	{
		if ( Selector < -STICK_WINDOW ) // left
			NewTurnToWP = false;
		else
			if ( Selector > STICK_WINDOW ) // left
				NewTurnToWP = true; // right
			
		if ( NewTurnToWP != F.TurnToWP )
		{		
			F.TurnToWP = NewTurnToWP;
			LEDBlue_ON;
			if ( F.TurnToWP )
				DoBeep100mSWithOutput(4, 2);

			LEDBlue_OFF;
		}
	}
} // UpdateParamSetChoice

void InitParameters(void)
{
	ALL_LEDS_ON;
	ParamSet = 1;

	if ( ReadEE((uint16)TxRxType) == -1 )
		UseDefaultParameters();
	
	ParamSet = 1;
	ParametersChanged = true;
	ReadParametersEE();
	ALL_LEDS_OFF;
} // InitParameters


