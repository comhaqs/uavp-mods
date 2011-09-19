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
	
	const int8 DefaultParams[MAX_PARAMETERS][2] = {
	{-20,0},			// RollKp, 			01
	{-10,0},	 		// RollKi,			02
	{50, 0},			// RollKd,			03
	{2,0},				// NeutralRadius was HorizDampKp,	04
	{6,0},	 			// RollIntLimit,	05

	{-20,0},	 		// PitchKp,			06
	{-10,0},	 		// PitchKi,			07
	{50,0},	 			// PitchKd,			08
	{4,0},	 			// AltKp,			09 // was 8
	{6,0},	 			// PitchIntLimit,	10
	
	{-25,0},	 		// YawKp, 			11
	{-4,0},	 			// YawKi,			12 was 35
	{0,0},	 			// was YawKd,		13

	{20,0},	 			// YawLimit,		14 was 50
	{2,0},	 			// YawIntLimit,		15
	{2,true}, 			// ConfigBits,		16c
	{9,true},			// RxThrottleCh was TimeSlots,	17
	{51,true}, 			// LowVoltThres,	18c
	{20,true}, 			// CamRollKp,		19c
	{45,true}, 			// PercentCruiseThr,20c 
	
	{7,true}, 			// BaroFilt,		21c 
	{0,true}, 			// MiddleDU,		22c
	{20,true}, 			// PercentIdleThr,	23c
	{0,true}, 			// MiddleLR,		24c
	{0,true}, 			// MiddleFB,		25c
	{20,true}, 			// CamPitchKp,		26c
	{10,0}, 			// CompassKp,		27
	{6,0},				// AltKi,			28 // was 8
	{2,0}, 				// NavSlew was NavRadius,	29
	{8,0}, 				// NavKi,			30 

	{0,0}, 				// GSThrottle,	    31
	{0,0},				// Acro,	    	32
	{10,0}, 		    // NavRTHAlt,		33
	{0,true},			// NavMagVar,		34c
	{LY530Gyro,true}, 	// SensorHint,     35c
	{ESCPPM,true}, 		// ESCType,			36c
	{7,true}, 			// RxChannels was TxRxType		37c
	{9,true},			// RxRollCh was NeutralRadius	38
	{30,true},			// PercentNavSens6Ch	39c
	{1,true},			// CamRollTrim,		40c

	{-16,0},			// NavKd			41
	{9,true},			// RxPitchCh was VertDampDecay    42
	{9,true},			// RxYawCh was HorizDampDecay	43
	{56,true},			// BaroScale	    44c
	{UAVXTelemetry,true}, // TelemetryType	45c
	{-10,0},		    // MaxDescentRateDmpS 	46
	{30,0},				// DescentDelayS	47
	{4,0},				// NavIntLimit		48
	{1,0},				// AltIntLimit		49 was 8
	{9,true},			// RxGearCh was GravComp		50c

	{9,true},			// RxAux1Ch was CompSteps	51
	{0,true},			// ServoSense		52c	
	{3,true},			// CompassOffsetQtr 53c
	{49,true},			// BatteryCapacity	54c	
	{9,true},			// RxAux2Ch was GyroYawType	55c		
	{9,true},			// RxAux3Ch was AltKd		56
	#if (defined  TRICOPTER) | (defined VTCOPTER ) | (defined Y6COPTER )
	{24,true},			// Orient			57
	#else	
	{0,true},			// Orient			57
	#endif // TRICOPTER | VTCOPTER | Y6COPTER			
	
	{12,0},				// NavYawLimit		58
	{50,0},				// Balance			59
	{9,true},			// RxAux4Ch			60
	{0,0},				// 60 - 64 unused currently	

	{0,0},	
	{0,0},	
	{0,0}						
	};




