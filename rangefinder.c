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
//    If not, see http://www.gnu.org/licenses/.

// Rangefinder (Maxbotix 5V Analog at either 1cm/click or 1"/click)

#include "uavx.h"

void GetRangefinderAltitude(void);
void InitRangefinder(void);

int16 RangefinderAltitude, RangefinderROC;

void GetRangefinderAltitude(void)
{
	static int16 Temp;

	if ( F.RangefinderAltitudeValid )
	{
		if ( F.RFInInches )
			Temp = (int16)(((int24)ADC(ADCAltChan) * 254L)/100L);
		else
			Temp = ADC(ADCAltChan); // Centimetres

		if ( mS[Clock] > mS[RangefinderROCUpdate] )
		{
			mS[RangefinderROCUpdate] = mS[Clock] + 1000; // 1 Sec.
			RangefinderROC = Temp - RangefinderAltitude;
		}
		RangefinderAltitude = Temp;
	}
	else
		RangefinderAltitude = RangefinderROC = 0;
} // GetRangefinderAltitude

void InitRangefinder(void)
{
	#ifndef UAVXLITE

	static int16 Temp;

	Temp = ADC(ADCAltChan);
	F.RangefinderAltitudeValid = !(Temp > 573) && (Temp < 778); // 2.8-3.8V => supply not RF
	GetRangefinderAltitude();

	#endif // !UAVXLITE

} // InitRangefinder
