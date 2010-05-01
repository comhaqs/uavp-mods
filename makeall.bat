@echo off
rem To see why we do the setlocal, see:
rem http://www.robvanderwoude.com/variableexpansion.html
rem http://www.robvanderwoude.com/ntset.html
SETLOCAL ENABLEDELAYEDEXPANSION

rem Batch compiles various possibilities of the UFO software for the Microchip 18F2xxx series
rem
rem Greg Egan 2008-2009
rem
rem Uses: makeallhelper.bat and makeclean.bat
rem
rem Clock rate CLOCK_16MHZ (Only 16MHZ available for UAVP version)
rem Type of PIC processor 18F2620 only
rem DEBUG_SENSORS to generate trace files of all main program and sensor values which can be plotted
rem SIMULATE to generate a simple flight simulator (no dynamics) for use with UAVXGS - no motors.
rem using UAVPSet (blank option in menu below testsoftware).
rem Configuration TRICOPTER for 3 motors and QUAD for 4.
rem Motors are disabled for DEBUG_SENSORS for safety reasons.
rem throttle shaping and X-mode to orient the camera forward set under UAVPSet.

rem Add/Delete required combinations to these sets
set CLOCK=CLOCK_16MHZ CLOCK_40MHZ
set PROC=18F2620
set DBG=NO_DEBUG DEBUG_SENSORS 
set RX=RX7CH RX6CH
set CFG=QUADROCOPTER TRICOPTER 

rem Personal choice
rem set CLOCK=CLOCK_16MHZ
rem set PROC=18F2620
rem set DBG=NO_DEBUG DEBUG_SENSORS
rem set RX=RX7CH RX6CH
rem set CFG=QUADROCOPTER TRICOPTER HELICOPTER FIXEDWING DELTAWING

rem Delete working files
call makeclean.bat

rem Requires Tortoise SVN 
call makerev.bat

del *.HEX

echo Starting makeall uavp > gen.lst
echo Starting makeall uavp > log.lst

for %%x in (%CLOCK%) do for %%p in (%PROC%) do for %%d in (%DBG%) do for %%r in (%RX%) do for %%c in (%CFG%) do call makeallhelper.bat %%x %%p %%d %%r %%c 

set CLOCK=CLOCK_16MHZ CLOCK_40MHZ
set PROC=18F2620
set DBG=SIMULATE 
set RX=RX7CH RX6CH
set CFG=QUADROCOPTER

for %%x in (%CLOCK%) do for %%p in (%PROC%) do for %%d in (%DBG%) do for %%r in (%RX%) do for %%c in (%CFG%) do call makeallhelper.bat %%x %%p %%d %%r %%c 
