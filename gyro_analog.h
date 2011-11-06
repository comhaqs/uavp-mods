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

// Analog Gyros

void GetAnalogGyroValues(void)
{ 
	if ( GyroType == IDG300Gyro ) // 500 Deg/Sec
	{
		GyroADC[Roll] = ADC(IDGADCRollChan);
		GyroADC[Pitch] = ADC(IDGADCPitchChan);
	}
	else
	{
		GyroADC[Roll] = ADC(NonIDGADCRollChan);
		GyroADC[Pitch] = ADC(NonIDGADCPitchChan);
	}

	GyroADC[Yaw] = ADC(ADCYawChan);

} // GetAnalogGyroValues

void InitAnalogGyros(void)
{
	// nothing to be done for analog gyros - could check nominal midpoints?
	YawRateF.i32 = 0;
	F.GyroFailure = false;
} // InitGyros

#ifdef TESTING

void CheckGyroFault(uint8 v, uint8 lv, uint8 hv)
{
	TxVal32(v, 1, 0);
	TxString(" (");
	TxVal32(lv,1,0);
	TxString(" >< ");
	TxVal32(hv,1,'V');
	TxString(")");
	if ( ( v < lv ) || ( v > hv ) )
		TxString(" Gyro NC or faulty?");
	TxNextLine();
} // CheckGyroFault

void GyroAnalogTest(void)
{
	int8 c, A[5], lv, hv, v;

	for ( c = 1; c <= 5; c++ )
		A[c] = ((int24)ADC(c) * 50L + 512L)/1024L;

	TxString("\r\n");
	ShowGyroType();
	TxString(" - Gyro Test\r\n");
	if ( (GyroType == IDG300Gyro ) || (GyroType == LY530Gyro ) ) // 3V gyros
		{ lv = 10; hv = 20;}
	else
		{ lv = 20; hv = 30;}

	// Roll
	if ( GyroType == IDG300Gyro )
		v = A[IDGADCRollChan];
	else
		v = A[NonIDGADCRollChan];

	TxString("Roll: \t"); 
	CheckGyroFault(v, lv, hv);

	// Pitch
	if ( GyroType == IDG300Gyro )
		v = A[IDGADCPitchChan]; 
	else 
		v = A[NonIDGADCPitchChan]; 

	TxString("Pitch:\t");		
	CheckGyroFault(v, lv, hv);	

	// Yaw
	if ( (GyroType == IDG300Gyro ) || ( GyroType == LY530Gyro ) ) // 3V gyros
		{ lv = 10; hv = 20;}
	else
		{ lv = 20; hv = 30;}
	
	v = ADC(ADCYawChan);
	TxString("Yaw:  \t");
	CheckGyroFault(v, lv, hv);	
	
} // GyroAnalogTest

#endif // TESTING








