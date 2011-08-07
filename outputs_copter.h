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

void WriteT580ESC(uint8, uint8, uint8);
void WriteT580ESCs(int8,  uint8, uint8, uint8, uint8);
void T580ESCs(uint8, uint8, uint8, uint8);
void OutSignals(void);

void OutSignals(void)
{	// The PWM pulses are in two parts these being a 1mS preamble followed by a 0-1mS part. 
	// Interrupts are enabled during the first part which uses TMR0.  TMR0 is monitored until 
	// there is just sufficient time for one remaining interrupt latency before disabling 
	// interrupts.  We do this because there appears to be no atomic method of detecting the 
	// remaining time AND conditionally disabling the interrupt. 
	static int8 m;
	static uint8 s, r, d;
	static i16u SaveTimer0;
	static uint24 SaveClockmS;

	if ( !F.MotorsArmed )
		StopMotors();

	#if !( defined SIMULATE | defined TESTING )

	PWM0 = PWMLimit(PWM[FrontC]);
	PWM1 = PWMLimit(PWM[LeftC]);
	PWM2 = PWMLimit(PWM[RightC]);
	PWM3 = PWMLimit(PWM[BackC]);
	PWM4 = PWMLimit(PWM[CamRollC]);
	PWM5 = PWMLimit(PWM[CamPitchC]);

	// Save TMR0 and reset
	DisableInterrupts;
	INTCONbits.TMR0IE = false;
	SaveClockmS = MilliSec;
	GetTimer0;
	SaveTimer0.u16 = Timer0.u16;
	FastWriteTimer0(TMR0_1MS);
	INTCONbits.TMR0IF = false;

	// dead timing code - caution
	#ifdef  CLOCK_16MHZ
		d = 7;
		do
			Delay10TCY(); 
		while ( --d > (uint8)0 );
	#else
		d = 16;
		while ( --d > (uint8)0 ) 
			Delay10TCY();
	#endif // CLOCK_16MHZ

	EnableInterrupts;

	if ( P[ESCType] == ESCPPM )
	{
		#ifdef TRICOPTER
			PORTB |= 0x07;
		#else
			PORTB |= 0x0f;
		#endif // TRICOPTER

		_asm
		MOVLB	0							// select Bank0
		#ifdef TRICOPTER
			MOVLW	0x07					// turn on 3 motors
		#else
			MOVLW	0x0f					// turn on all motors
		#endif // TRICOPTER
		MOVWF	SHADOWB,1
		_endasm

		SyncToTimer0AndDisableInterrupts();

		if( ServoToggle == 0 )	// driver cam servos only every 2nd pulse
		{
			PORTB |= 0x3f;
			_asm
			MOVLB	0						// select Bank0
			MOVLW	0x3f					// turn on all motors
			MOVWF	SHADOWB,1
			_endasm	
		}
		else	
		{
			Delay1TCY(); 
			Delay1TCY(); 
			Delay1TCY(); 
			Delay1TCY(); 
			Delay1TCY(); 
			Delay1TCY();
		}

		_asm
		MOVLB	0							// select Bank0
OS005:
		MOVF	SHADOWB,0,1	
		MOVWF	PORTB,0
		#ifdef TRICOPTER
			ANDLW	0x07		
		#else
			ANDLW	0x0f
		#endif //TRICOPTER
		BZ		OS006
			
		DECFSZ	PWM0,1,1				
		GOTO	OS007
					
		BCF		SHADOWB,0,1
OS007:
		DECFSZ	PWM1,1,1		
		GOTO	OS008
					
		BCF		SHADOWB,1,1
OS008:
		DECFSZ	PWM2,1,1
		GOTO	OS009
					
		BCF		SHADOWB,2,1

OS009:
		#ifndef TRICOPTER
		DECFSZ	PWM3,1,1	
		GOTO	OS010
						
		BCF		SHADOWB,3,1	
		#endif // !TRICOPTER		

OS010:
		_endasm

		#ifdef TRICOPTER
			Delay1TCY(); 
			Delay1TCY(); 
			Delay1TCY();  
		#endif // TRICOPTER 

		#ifdef CLOCK_40MHZ
			Delay10TCYx(2); 
			Delay1TCY(); 
			Delay1TCY(); 
			Delay1TCY(); 
			Delay1TCY(); 
			Delay1TCY(); 
			Delay1TCY(); 
			Delay1TCY(); 
		#endif // CLOCK_40MHZ
		_asm				
		GOTO	OS005
OS006:
		_endasm
		
		EnableInterrupts;
		SyncToTimer0AndDisableInterrupts();	

	} 
	else
	{ // I2C ESCs

		if( ServoToggle == 0 )	// driver cam servos only every 2nd pulse
		{
			#ifdef TRICOPTER
				PORTB |= 0x38;
			#else
				PORTB |= 0x30;
			#endif // TRICOPTER

			_asm
			MOVLB	0					// select Bank0
			#ifdef TRICOPTER
				MOVLW	0x38			// turn on 3 servoes
			#else
				MOVLW	0x30			// turn on 2 servoes
			#endif // TRICOPTER
			MOVWF	SHADOWB,1
			_endasm	
		}
		else
		{
			Delay1TCY(); 
			Delay1TCY(); 
			Delay1TCY(); 
			Delay1TCY(); 
			Delay1TCY(); 
			Delay1TCY();
		}

		#ifdef MULTICOPTER
		// in X3D and Holger-Mode, K2 (left motor) is SDA, K3 (right) is SCL.
		// ACK (r) not checked as no recovery is possible. 
		// Octocopters may have ESCs paired with common address so ACK is meaningless.
		// All motors driven with fourth motor ignored for Tricopter.
	
		switch ( P[ESCType] ) {
		case ESCX3D:
			ESCI2CStart();
			r = WriteESCI2CByte(0x10); // one command, 4 data bytes
			r += WriteESCI2CByte( I2CESCLimit(PWM[FrontC]) ); 
			r += WriteESCI2CByte( I2CESCLimit(PWM[LeftC]) );
			r += WriteESCI2CByte( I2CESCLimit(PWM[RightC]) );
			r += WriteESCI2CByte( I2CESCLimit(PWM[BackC]) );
			ESCI2CFail[0] += r;
			ESCI2CStop();
			break;
		case ESCLRCI2C:
			T580ESCs(I2CESCLimit(PWM[FrontC]), I2CESCLimit(PWM[LeftC]), I2CESCLimit(PWM[RightC]), I2CESCLimit(PWM[BackC]));
			break;
		case ESCYGEI2C:
			for ( m = 0 ; m < NoOfI2CESCOutputs ; m++ )
			{
				ESCI2CStart();
				r = WriteESCI2CByte(0x62 + ( m*2) ); // one cmd, one data byte per motor
				r += WriteESCI2CByte( I2CESCLimit(PWM[m])>>1 );
				ESCI2CFail[m] += r;
				ESCI2CStop();
			}
			break;
 		case ESCHolger:
			for ( m = 0 ; m < NoOfI2CESCOutputs ; m++ )
			{
				ESCI2CStart();
				r = WriteESCI2CByte(0x52 + ( m*2 )); // one command, one data byte per motor
				r += WriteESCI2CByte( I2CESCLimit(PWM[m]) );
				ESCI2CFail[m] += r;
				ESCI2CStop();
			}
			break;
		default:
			break;
		} // switch

		#endif //  MULTICOPTER
	}

	if ( ServoToggle == 0 )
	{
		_asm
		MOVLB	0
OS001:
		MOVF	SHADOWB,0,1	
		MOVWF	PORTB,0
		#ifdef TRICOPTER
			ANDLW	0x38
		#else
			ANDLW	0x30
		#endif  // TRICOPTER		
		BZ		OS002	
	
		DECFSZ	PWM4,1,1
		GOTO	OS003

		BCF		SHADOWB,4,1
	
OS003:
		DECFSZ	PWM5,1,1
		GOTO	OS004
	
		BCF		SHADOWB,5,1

OS004:
		#ifdef TRICOPTER
			DECFSZ	PWM3,1,1
			GOTO	OS0011
	
			BCF		SHADOWB,3,1
		#endif // TRICOPTER
OS0011:
		_endasm
	
		#ifndef TRICOPTER
			Delay1TCY(); 
			Delay1TCY(); 
			Delay1TCY(); 
		#endif // !TRICOPTER
 
		Delay1TCY();	
		Delay1TCY();

		#ifdef CLOCK_40MHZ
			Delay10TCYx(2); 
			Delay1TCY(); 
			Delay1TCY(); 
			Delay1TCY(); 
			Delay1TCY(); 
			Delay1TCY(); 
			Delay1TCY(); 
			Delay1TCY(); 
		#endif // CLOCK_40MHZ
	
		_asm
	
		GOTO	OS001
OS002:
		_endasm

		EnableInterrupts;
		SyncToTimer0AndDisableInterrupts();
	}

	FastWriteTimer0(SaveTimer0.u16);

	// add in period mS Clock was disabled - this is tending to advance the clock too far!
	if ( P[ESCType] == ESCPPM )
		if ( ServoToggle == 0 )
			MilliSec = SaveClockmS + 3;
		else
			MilliSec = SaveClockmS + 2;
	else
		if ( ServoToggle == 0 )
			MilliSec = SaveClockmS + 2;
		else
			MilliSec = SaveClockmS;

	if ( ++ServoToggle == ServoInterval )
		ServoToggle = 0;

	INTCONbits.TMR0IE = true;
	EnableInterrupts;
	
	#endif // !(SIMULATE | TESTING)

} // OutSignals

//________________________________________________________________

// LotusRC T580 I2C ESCs

enum T580States { T580Starting = 0, T580Stopping = 1, T580Constant = 2 };

boolean T580Running = false;
uint32 T580Available = 0;

void WriteT580ESC(uint8 a, uint8 s, uint8 d2) {

	ESCI2CStart();
	WriteESCI2CByte(a);
	WriteESCI2CByte(0xa0 | s );
	WriteESCI2CByte(d2);
	ESCI2CStop();

} // WriteT580ESC

void WriteT580ESCs(int8 s, uint8 f, uint8 l, uint8 r, uint8 b) {

    if ( ( s == T580Starting ) || ( s == T580Stopping ) ) {
        WriteT580ESC(0xd0, s, 0);
        WriteT580ESC(0xd2, s, 0);
        WriteT580ESC(0xd4, s, 0);
        WriteT580ESC(0xd6, s, 0);
    } else {
        WriteT580ESC(0xd0, s, l);
        WriteT580ESC(0xd2, s, f);
        WriteT580ESC(0xd4, s, b);
        WriteT580ESC(0xd6, s, r);
    }

} // WriteT580ESCs

void T580ESCs(uint8 f, uint8 l, uint8 r, uint8 b) {

    static boolean Run;
    static uint8 i;

    Run = ((int16)f+b+r+l) > 0;

    if ( T580Running )
        if ( Run )
            WriteT580ESCs(T580Constant, f, l, r, b);
        else 
		{
            WriteT580ESCs(T580Stopping, 0, 0, 0, 0);
			Delay1mS(2);
            for ( i = 0; i < (uint8)50; i++ ) 
			{
				WriteT580ESCs(T580Constant, 0, 0, 0, 0);
				Delay1mS(2);
			}
        }
    else
        if ( Run ) 
		{
        	for ( i = 0; i < (uint8)50; i++ )
			{ 
				WriteT580ESCs(T580Constant, 0, 0, 0, 0);
				Delay1mS(2);
			}
            WriteT580ESCs(T580Starting, 0, 0, 0, 0);
        }
        else
            WriteT580ESCs(T580Constant, f, l, r, b);

    T580Running = Run;

} // T580ESCs





