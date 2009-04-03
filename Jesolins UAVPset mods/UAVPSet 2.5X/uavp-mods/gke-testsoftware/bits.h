// ==============================================
// =      U.A.V.P Brushless UFO Controller      =
// =           Professional Version             =
// = Copyright (c) 2007 Ing. Wolfgang Mahringer =
// ==============================================
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License along
//  with this program; if not, write to the Free Software Foundation, Inc.,
//  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//
// ==============================================
// =  please visit http://www.uavp.de           =
// =               http://www.mahringer.co.at   =
// ==============================================

// Bit definitions

#define	NULL	0

// when changing, see OutSignal() in utils.c
#ifdef ESC_PPM
bit	PulseVorne		@PORTB.0;
bit	PulseLinks		@PORTB.1;
bit	PulseRechts		@PORTB.2;
bit	PulseHinten		@PORTB.3;
#endif
#if defined ESC_X3D || defined ESC_HOLGER || defined ESC_YGEI2C
bit ESC_SDA			@PORTB.1;
bit ESC_SCL			@PORTB.2;
bit ESC_DIO			@TRISB.1;
bit ESC_CIO			@TRISB.2;
#endif

// the sensor bus lines
bit I2C_SDA			@PORTB.6;
bit I2C_DIO			@TRISB.6;
bit I2C_SCL			@PORTB.7;
bit	I2C_CIO			@TRISB.7;

#define	I2C_ACK		0
#define	I2C_NACK	1


bit PulseCamRoll	@PORTB.4;
bit PulseCamNick	@PORTB.5;

#define ALL_PULSE_ON	PORTB |= 0b.0000.1111
#define ALL_OUTPUTS_OFF	PORTB &= 0b.1111.0000
#define ALL_OUTPUTS		(PORTB & 0b.0000.1111)
#define CAM_PULSE_ON	PORTB |= 0b.0011.0000;

bit	Switch		@PORTA.4;

bit	LISL_CS		@PORTC.5;
bit LISL_SDA	@PORTC.4;
bit LISL_SCL	@PORTC.3;
bit	LISL_IO		@TRISC.4;



// The LEDs and the beeper
#define ON	1
#define OFF	0

#define MODELLOSTTIMER		20;	// in 0,2sec units
				// time until first beep after lost xmit signal
#define MODELLOSTTIMERINT	2;	// in 0,2 sec units
				// interval beep when active


#define LedYellow	LED6
#define LedGreen	LED4
#define	LedBlue		LED2
#define LedRed		LED3

#define LED1	0x01	/* Aux2 */
#define LED2	0x02	/* blue */
#define LED3	0x04	/* red */ 
#define LED4	0x08	/* green */
#define LED5	0x10	/* Aux1 */
#define LED6	0x20	/* yellow */
#define LED7	0x40	/* Aux3 */
#define Beeper	0x80

#define ALL_LEDS_ON		SwitchLedsOn(LedBlue|LedRed|LedGreen|LedYellow)

#define ALL_LEDS_OFF	SwitchLedsOff(LedBlue|LedRed|LedGreen|LedYellow)

#define ARE_ALL_LEDS_OFF if((LedShadow&(LedBlue|LedRed|LedGreen|LedYellow))==0)

#define LedRed_ON		SwitchLedsOn(LedRed);
#define LedBlue_ON		SwitchLedsOn(LedBlue);
#define LedGreen_ON		SwitchLedsOn(LedGreen);
#define LedYellow_ON	SwitchLedsOn(LedYellow);
#define LedRed_OFF		SwitchLedsOff(LedRed);
#define LedBlue_OFF		SwitchLedsOff(LedBlue);
#define LedGreen_OFF	SwitchLedsOff(LedGreen);
#define LedYellow_OFF	SwitchLedsOff(LedYellow);
#define LedRed_TOG		if( (LedShadow&LedRed) == 0 ) SwitchLedsOn(LedRed); else SwitchLedsOff(LedRed);
#define LedBlue_TOG		if( (LedShadow&LedBlue) == 0 ) SwitchLedsOn(LedBlue); else SwitchLedsOff(LedBlue);
#define Beeper_OFF		SwitchLedsOff(Beeper);
#define Beeper_ON		SwitchLedsOn(Beeper);
#define Beeper_TOG		if( (LedShadow&Beeper) == 0 ) SwitchLedsOn(Beeper); else SwitchLedsOff(Beeper);

bit	_NoSignal		@Flags.0;	// if no valid signal is received
bit	_Flying			@Flags.1;	// UFO is flying
bit _NewValues		@Flags.2;	// new RX channel values sampled
bit _FirstTimeout	@Flags.3;	// is 1 after first 9ms TO expired
bit _NegIn			@Flags.4;	// negative signed input (serial.c)
bit _LowBatt		@Flags.5;	// if Batt voltage is low
bit	_UseLISL		@Flags.6;	// 1 if LISL Sensor is used
bit _SerEnabled		@Flags.7;	// 1 if RS232 is enabled

#define COMPASS_I2C_ID	0x42	/* I2C slave address */

// baro (altimeter) sensor
#define BARO_I2C_ID			0xee
#define BaroTemp_BMP085	0x2e
#define BaroTemp_SMD500	0x6e
#define BARO_PRESS			0xf4
#define BARO_CTL			0xf4
#define BARO_ADC_MSB		0xf6
#define BARO_ADC_LSB		0xf7
#define BARO_TYPE			0xd0
//#define BARO_ID_SMD500		??
#define BARO_ID_BMP085		0x55


// LISL-Register mapping
#define	LISL_WHOAMI		(0x0f)
#define	LISL_OFFSET_X	(0x16)
#define	LISL_OFFSET_Y	(0x17)
#define	LISL_OFFSET_Z	(0x18)
#define	LISL_GAIN_X		(0x19)
#define	LISL_GAIN_Y		(0x1A)
#define	LISL_GAIN_Z		(0x1B)
#define	LISL_CTRLREG_1	(0x20)
#define	LISL_CTRLREG_2	(0x21)
#define	LISL_CTRLREG_3	(0x22)
#define	LISL_STATUS		(0x27)
#define LISL_OUTX_L		(0x28)
#define LISL_OUTX_H		(0x29)
#define LISL_OUTY_L		(0x2A)
#define LISL_OUTY_H		(0x2B)
#define LISL_OUTZ_L		(0x2C)
#define LISL_OUTZ_H		(0x2D)
#define LISL_FF_CFG		(0x30)
#define LISL_FF_SRC		(0x31)
#define LISL_FF_ACK		(0x32)
#define LISL_FF_THS_L	(0x34)
#define LISL_FF_THS_H	(0x35)
#define LISL_FF_DUR		(0x36)
#define LISL_DD_CFG		(0x38)
#define LISL_READ		(0x80)
