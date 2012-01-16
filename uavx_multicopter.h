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
	
	#pragma idata paramdefaults
	const int8 DefaultParams[MAX_PARAMETERS][2] = {
	#ifdef CLOCK_16MHZ
		{-20,0},			// RollKp, 			01
		{-10,0},	 		// RollKi,			02
		{50, 0},			// RollKd,			03
		{2,true},			// NeutralRadius was HorizDampKp,	04
		{20,0},	 			// RollIntLimit,	05
	
		{-20,0},	 		// PitchKp,			06
		{-10,0},	 		// PitchKi,			07
		{50,0},	 			// PitchKd,			08
		{5,0},	 			// AltKp,			09 // was 8
		{20,0},	 			// PitchIntLimit,	10
		
		{-25,0},	 		// YawKp, 			11
		{-4,0},	 			// YawKi,			12 was 35
		{RESCALE_TO_ACC,true},// AccTrack was YawKd,		13
		{20,0},	 			// YawLimit,		14 was 50
		{2,0},	 			// YawIntLimit,		15
	#else // CLOCK_40MHZ
		{-20,0},			// RollKp, 			01
		{-15,0},	 		// RollKi,			02
		{50, 0},			// RollKd,			03
		{2,true},			// NeutralRadius was HorizDampKp,	04
		{50,0},	 			// RollIntLimit,	05
	
		{-20,0},	 		// PitchKp,			06
		{-15,0},	 		// PitchKi,			07
		{50,0},	 			// PitchKd,			08
		{5,0},	 			// AltKp,			09 // was 8
		{50,0},	 			// PitchIntLimit,	10
		
		{-25,0},	 		// YawKp, 			11
		{-4,0},	 			// YawKi,			12 was 35
		{RESCALE_TO_ACC,true},// AccTrack was YawKd,		13
		{20,0},	 			// YawLimit,		14 was 50
		{2,0},	 			// YawIntLimit,		15
	#endif // CLOCK_40MHZ

	{2,true}, 			// ConfigBits,		16c
	{1,true},			// RxThrottleCh was TimeSlots,	17
	{51,true}, 			// LowVoltThres,	18c
	{20,true}, 			// CamRollKp,		19c
	{45,true}, 			// PercentCruiseThr,20c 
	
	{7,true}, 			// BaroFilt,		21c 
	{0,true}, 			// MiddleDU,		22c
	{20,true}, 			// PercentIdleThr,	23c
	{0,true}, 			// MiddleLR,		24c
	{0,true}, 			// MiddleFB,		25c
	{20,true}, 			// CamPitchKp,		26c
	{10,true}, 			// CompassKp,		27
	{6,0},				// AltKi,			28 // was 8
	{2,true}, 			// NavSlew was NavRadius,	29
	{8,true}, 			// NavKi,			30 

	{0,0}, 				// GSThrottle,	    31
	{0,0},				// Acro,	    	32
	{10,true}, 		    // NavRTHAlt,		33
	{0,true},			// NavMagVar,		34c
	{LY530Gyro,true}, 	// SensorHint,     	35c
	{ESCPPM,true}, 		// ESCType,			36c
	{7,true}, 			// RxChannels was TxRxType		37c
	{2,true},			// RxRollCh was NeutralRadius	38
	{30,true},			// PercentNavSens6Ch	39c
	{1,true},			// CamRollTrim,		40c

	{-20,true},			// NavKd			41
	{3,true},			// RxPitchCh was VertDampDecay    42
	{4,true},			// RxYawCh was HorizDampDecay	43
	{56,true},			// BaroScale	    44c
	{UAVXTelemetry,true}, // TelemetryType	45c
	{-10,0},		    // MaxDescentRateDmpS 	46
	{30,0},				// DescentDelayS	47
	{6,true},			// NavIntLimit		48 
	{1,0},				// AltIntLimit		49 was 8
	{5,true},			// RxGearCh was GravComp		50c

	{6,true},			// RxAux1Ch was CompSteps	51
	{0,true},			// ServoSense		52c	
	{3,true},			// CompassOffsetQtr 53c
	{49,true},			// BatteryCapacity	54c	
	{7,true},			// RxAux2Ch was GyroYawType	55c		
	{8,true},			// RxAux3Ch was AltKd		56
	#if (defined  TRICOPTER) | (defined VTCOPTER ) | (defined Y6COPTER )
	{24,true},			// Orient			57
	#else	
	{0,true},			// Orient			57
	#endif // TRICOPTER | VTCOPTER | Y6COPTER			
	
	{12,true},			// NavYawLimit		58
	{50,0},				// Balance			59
	{9,true},			// RxAux4Ch			60
	{-15,0},			// RollKp2			61

	{-15,0},			// PitchKp2			62
	{0,0},	
	{0,0}						
	};
#pragma idata




