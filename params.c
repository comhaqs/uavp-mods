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

#include "uavx.h"

void ReadParametersEE(void);
void WriteParametersEE(uint8);
void UseDefaultParameters(void);
void UpdateWhichParamSet(void);
boolean ParameterSanityCheck(void);
void InitParameters(void);

const uint8 ESCLimits [] = { OUT_MAXIMUM, OUT_HOLGER_MAXIMUM, OUT_X3D_MAXIMUM, OUT_YGEI2C_MAXIMUM, OUT_LRC_MAXIMUM };

#ifdef MULTICOPTER
	#include "uavx_multicopter.h"
#else
	#ifdef HELICOPTER
		#include "uavx_helicopter.h"	
	#else
		#ifdef ELEVONS
			#include "uavx_elevon.h"
		#else
			#include "uavx_aileron.h"
		#endif
	#endif
#endif 


uint8	ParamSet;
boolean ParametersChanged, SaveAllowTurnToWP;

#pragma udata params
int8 P[MAX_PARAMETERS];
#pragma udata

#pragma udata orient
int16 OSin[48], OCos[48];
#pragma udata
int8 Orientation;
uint8 UAVXAirframe;

void ReadParametersEE(void)
{
	static int8 i,b;
	static uint16 a;

	if ( ParametersChanged )
	{   // overkill if only a single parameter has changed but is not in flight loop

		A[Roll].Kp = P[RollKp];
		A[Roll].Ki = P[RollKi];
		A[Roll].Kd = P[RollKd];
		A[Roll].IntLimit = P[RollIntLimit];
		A[Roll].AccOffset = (int16)P[MiddleLR];
		
		A[Pitch].Kp = P[PitchKp];
		A[Pitch].Ki = P[PitchKi];
		A[Pitch].Kd = P[PitchKd];
		A[Pitch].IntLimit = P[PitchIntLimit];
		A[Pitch].AccOffset = (int16)P[MiddleFB];
		
		A[Yaw].Kp = P[YawKp];
		A[Yaw].Ki = P[YawKi];
		//A[Yaw].Kd = P[YawKd];
		A[Yaw].Limiter = P[YawLimit];
		A[Yaw].IntLimit = P[YawIntLimit] * 256;
		A[Yaw].AccOffset = (int16)P[MiddleDU];

		#ifdef CLOCK_40MHZ
			#ifdef MULTICOPTER
				F.NormalFlightMode = ParamSet == (uint8)1;
			#else
				F.NormalFlightMode = true;
			#endif
		#else
			F.NormalFlightMode = true;
		#endif // CLOCK_40MHZ

		a = (ParamSet - 1)* MAX_PARAMETERS;	
		for ( i = 0; i < MAX_PARAMETERS; i++)
			P[i] = ReadEE(a + i);

		ESCMax = ESCLimits[P[ESCType]];
		if ( P[ESCType] == ESCPPM )
			TRISB = 0b00000000; // make outputs
		else
			for ( i = 0; i < NO_OF_I2C_ESCS; i++ )
				ESCI2CFail[i] = 0;

		InitGyros();
		InitAccelerometers();

		b = P[ServoSense];
		for ( i = 0; i < 6; i++ )
		{
			if ( b & 1 )
				PWMSense[i] = -1;
			else
				PWMSense[i] = 1;
			b >>=1;
		}

		F.UsingPositionHoldLock = ( (P[ConfigBits] & UsePositionHoldLockMask ) != 0);
		F.UsingAltControl = ( (P[ConfigBits] & UseAltControlMask ) != 0);

		#ifdef SIMULATE
			P[PercentCruiseThr] = 35;
		#endif // SIMULATE
		CruiseThrottle = NewCruiseThrottle = FromPercent((int16)P[PercentCruiseThr], RC_MAXIMUM);

		IdleThrottle = Limit((int16)P[PercentIdleThr], 10, 20); // 10-25%
		IdleThrottle = FromPercent(IdleThrottle, RC_MAXIMUM);
	 
		NavSlewLimit = Limit(P[NavSlew], 1, 4); 
		NavSlewLimit = ConvertMToGPS(NavSlewLimit); 

		NavNeutralRadius = Limit((int16)P[NeutralRadius], 0, 5);
		NavNeutralRadius = ConvertMToGPS(NavNeutralRadius);

		MinROCCmpS = (int16)P[MaxDescentRateDmpS] * 10;

		TauCF = (int16)P[BaroFilt];
		TauCF = Limit(TauCF, 3, 40);

		CompassOffset = ((((int16)P[CompassOffsetQtr] * 90L - (int16)P[NavMagVar])*MILLIPI)/180L); // changed sign of MagVar AGAIN!

		#ifdef MULTICOPTER
			Orientation = P[Orient];
			if (Orientation == -1 ) // uninitialised
				Orientation = 0;
			else
				if (Orientation < 0 )
					Orientation += 48;
		#else
			Orientation = 0;
		#endif // MULTICOPTER

		PPMPosPolarity = (P[ServoSense] & PPMPolarityMask) == 0;	
		F.UsingSerialPPM = ( P[ConfigBits] & RxSerialPPMMask ) != 0;
		PIE1bits.CCP1IE = false;
		DoRxPolarity();
		PPM_Index = PrevEdge = 0;
		PIE1bits.CCP1IE = true;

		NoOfControls = P[RxChannels];
		if ( (( NoOfControls&1 ) != 1 ) && !F.UsingSerialPPM )
			NoOfControls--;

		if ( NoOfControls < 7 )
		{
			NavSensitivity = FromPercent((int16)P[PercentNavSens6Ch], RC_MAXIMUM);
			NavSensitivity = Limit(NavSensitivity, 0, RC_MAXIMUM);
		}

		Map[ThrottleRC] = P[RxThrottleCh]-1;
		Map[RollRC] = P[RxRollCh]-1;
		Map[PitchRC] = P[RxPitchCh]-1;
		Map[YawRC] = P[RxYawCh]-1;
		Map[RTHRC] = P[RxGearCh]-1;
		Map[CamPitchRC] = P[RxAux1Ch]-1;
		Map[NavGainRC] = P[RxAux2Ch]-1;
		Map[Ch8RC]= P[RxAux3Ch]-1;
		Map[Ch9RC] = P[RxAux4Ch]-1;

		for ( i = 0; i < NoOfControls; i++) // make reverse map
			RMap[Map[i]] = i;

		F.RFInInches = ((P[ConfigBits] & RFInchesMask) != 0);

		F.FailsafesEnabled = ((P[ConfigBits] & UseFailsafeMask) == 0);

		F.UsingTxMode2 = ((P[ConfigBits] & TxMode2Mask) != 0);
		F.UsingRTHAutoDescend = ((P[ConfigBits] & UseRTHDescendMask) != 0);
		NavRTHTimeoutmS = (uint24)P[DescentDelayS]*1000L;

		BatteryVoltsLimitADC = BatteryVoltsADC = ((int24)P[LowVoltThres] * 1024 + 70L) / 139L; // UAVPSet 0.2V units
		BatteryCurrentADC = 0;
		
		F.ParametersValid = ParameterSanityCheck();

		ParametersChanged = false;
	}
	
} // ReadParametersEE

void WriteParametersEE(uint8 s)
{
	static int8 p;
	static uint16 addr;
	
	addr = (s - 1)* MAX_PARAMETERS;

	for ( p = 0; p < MAX_PARAMETERS; p++)
		WriteEE( addr + p,  P[p]);
} // WriteParametersEE

void UseDefaultParameters(void)
{ // loads a representative set of initial parameters as a base for tuning
	static uint16 p;

	for ( p = 0; p < (uint16)MAX_EEPROM; p++ )
		WriteEE( p,  0xff);

	for ( p = 0; p < (uint16)MAX_PARAMETERS; p++ )
		P[p] = DefaultParams[p][0];

	WriteParametersEE(1);
	WriteParametersEE(2);
	ParamSet = 1;

	WriteEE(NAV_NO_WP, 0); // set NoOfWaypoints to zero

	TxString("\r\nDefault Parameters Loaded\r\n");
	TxString("Do a READ CONFIG to refresh the UAVPSet parameter display\r\n");	
} // UseDefaultParameters

void UpdateParamSetChoice(void)
{
	#define STICK_WINDOW 30

	static uint8 NewParamSet, NewAllowNavAltitudeHold, NewAllowTurnToWP;
	static int8 Selector;

	NewParamSet = ParamSet;
	NewAllowNavAltitudeHold = F.AllowNavAltitudeHold;
	NewAllowTurnToWP = F.AllowTurnToWP;

	if ( F.UsingTxMode2 )
		Selector = A[Roll].Desired;
	else
		Selector = -A[Yaw].Desired;

	if ( (Abs(A[Pitch].Desired) > STICK_WINDOW) && (Abs(Selector) > STICK_WINDOW) )
	{
		if ( A[Pitch].Desired > STICK_WINDOW ) // bottom
		{
			if ( Selector < -STICK_WINDOW ) // left
			{ // bottom left
				NewParamSet = 1;
				NewAllowNavAltitudeHold = true;
			}
			else
				if ( Selector > STICK_WINDOW ) // right
				{ // bottom right
					NewParamSet = 2;
					NewAllowNavAltitudeHold = true;
				}
		}		
		else
			if ( A[Pitch].Desired < -STICK_WINDOW ) // top
			{		
				if ( Selector < -STICK_WINDOW ) // left
				{
					NewAllowNavAltitudeHold = false;
					NewParamSet = 1;
				}
				else 
					if ( Selector > STICK_WINDOW ) // right
					{
						NewAllowNavAltitudeHold = false;
						NewParamSet = 2;
					}
			}

		if ( ( NewParamSet != ParamSet ) || ( NewAllowNavAltitudeHold != F.AllowNavAltitudeHold ) )
		{	
			ParamSet = NewParamSet;
			F.AllowNavAltitudeHold = NewAllowNavAltitudeHold;
			LEDBlue_ON;
			DoBeep100mSWithOutput(2, 2);
			if ( ParamSet == (uint8)2 )
				DoBeep100mSWithOutput(2, 2);
			if ( F.AllowNavAltitudeHold )
				DoBeep100mSWithOutput(4, 4);
			ParametersChanged |= true;
			Beeper_OFF;
			LEDBlue_OFF;
		}
	}

	if ( F.UsingTxMode2 )
		Selector = -A[Yaw].Desired;
	else
		Selector = A[Roll].Desired;

	if ( (Abs(RC[ThrottleRC]) < STICK_WINDOW) && (Abs(Selector) > STICK_WINDOW ) )
	{
		if ( Selector < -STICK_WINDOW ) // left
			NewAllowTurnToWP = false;
		else
			if ( Selector > STICK_WINDOW ) // left
				NewAllowTurnToWP = true; // right
			
		if ( NewAllowTurnToWP != F.AllowTurnToWP )
		{		
			F.AllowTurnToWP = NewAllowTurnToWP;
			LEDBlue_ON;
		//	if ( F.AllowTurnToWP )
				DoBeep100mSWithOutput(4, 2);

			LEDBlue_OFF;
		}
	}
	
	SaveAllowTurnToWP = F.AllowTurnToWP;

} // UpdateParamSetChoice

boolean ParameterSanityCheck(void)
{
	return ((P[RollKp] != 0) &&
			(P[PitchKp]!= 0) &&
			(P[YawKp] != 0) );
} // ParameterSanityCheck

void InitParameters(void)
{
	static int8 i;
	static int16 A;

	UAVXAirframe = AF_TYPE;

	for (i = 0; i < 48; i++)
	{
		A = (int16)(((int32)i * MILLIPI)/24L);
		OSin[i] = int16sin(A);
		OCos[i] = int16cos(A);
	}
	Orientation = 0;

	ALL_LEDS_ON;
	ParamSet = 1;

	if ( ( ReadEE((uint16)RxChannels) == -1 ) || (ReadEE(MAX_PARAMETERS + (uint16)RxChannels) == -1 ) )
		UseDefaultParameters();

	ParamSet = 1;
	ReadParametersEE();
	ParametersChanged = true;

	ALL_LEDS_OFF;  
} // InitParameters


