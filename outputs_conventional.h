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


void OutSignals(void)
{	// The PWM pulses are in two parts these being a 1mS preamble followed by a 0-1mS part. 
	// Interrupts are enabled during the first part which uses TMR0.  TMR0 is monitored until 
	// there is just sufficient time for one remaining interrupt latency before disabling 
	// interrupts.  We do this because there appears to be no atomic method of detecting the 
	// remaining time AND conditionally disabling the interupt. 

	static i16u SaveTimer0;
	static uint24 SaveClockmS;
	static int8 ServoUpdate;

	if ( !F.MotorsArmed )
		StopMotors();	

	#if ( defined SIMULATE | defined TESTING )

	MixAndLimitMotors();
	MixAndLimitCam();

	PWM0 = PWMLimit(PWM[K1]);
	PWM1 = PWMLimit(PWM[K2]);
	PWM2 = PWMLimit(PWM[K3]);
	PWM3 = PWMLimit(PWM[K4]);	
	PWM4 = PWMLimit(PWM[K5]);
	PWM5 = PWMLimit(PWM[K6]);

	#else

	if ( --ServoUpdate <= 0 )
	{	
		ServoUpdate = SERVO_INTERVAL;						
		// Save TMR0 and reset
		DisableInterrupts;
		SaveClockmS = MilliSec;
		GetTimer0;
		SaveTimer0.u16 = Timer0.u16;
		FastWriteTimer0(TMR0_1MS);

		// dead timing code to reduce pre-pulse to 1mS - caution
		Delay10TCYx(12); // 2.5uS per click
	
		EnableInterrupts;
		
		PORTB |= 0x0f;
		
		_asm
		MOVLB	0					
		MOVLW	0x0f					
		MOVWF	SHADOWB,1
		_endasm

		MixAndLimitMotors();
		MixAndLimitCam();

		PWM0 = PWMLimit(PWM[K1]);
		PWM1 = PWMLimit(PWM[K2]);
		PWM2 = PWMLimit(PWM[K3]);
		PWM3 = PWMLimit(PWM[K4]);	
		PWM4 = PWMLimit(PWM[K5]);
		PWM5 = PWMLimit(PWM[K6]);

		SyncToTimer0AndDisableInterrupts();
		
		PORTB |= 0x3f;
		_asm
		MOVLB	0	
		MOVLW	0x3f				// turn on all motors
		MOVWF	SHADOWB,1
			
		MOVLB	0
OS005:
		MOVF	SHADOWB,0,1	
		MOVWF	PORTB,0
		ANDLW	0x0f
		
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
		DECFSZ	PWM3,1,1	
		GOTO	OS010
								
		BCF		SHADOWB,3,1					
OS010:				
		GOTO	OS005
OS006:
		_endasm
				
		EnableInterrupts;
		SyncToTimer0AndDisableInterrupts();	
			
		_asm
		MOVLB	0
OS001:
		MOVF	SHADOWB,0,1	
		MOVWF	PORTB,0
		ANDLW	0x30
			
		BZ		OS002	
			
		DECFSZ	PWM4,1,1
		GOTO	OS003
		
		BCF		SHADOWB,4,1			
OS003:
		DECFSZ	PWM5,1,1
		GOTO	OS004
			
		BCF		SHADOWB,5,1		
OS004:
		_endasm
			
		Delay1TCY(); 
		Delay1TCY(); 
		Delay1TCY(); 
		
		Delay1TCY();
		Delay1TCY(); 
		Delay1TCY();
			
		_asm
			
		GOTO	OS001
OS002:
		_endasm
		
		EnableInterrupts;
		SyncToTimer0AndDisableInterrupts();
		
		FastWriteTimer0(SaveTimer0.u16);
		// the 1mS clock seems to get in for 40MHz but not 16MHz so like this for now?
		
		MilliSec = SaveClockmS + 3;
		
//		INTCONbits.TMR0IE = true;
		EnableInterrupts;
	}
		
	#endif // !(SIMULATE | TESTING)
	
} // OutSignals




