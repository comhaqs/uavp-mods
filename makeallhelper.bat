@echo off

SETLOCAL ENABLEDELAYEDEXPANSION

rem Helper script for makeall.bat
rem =======================================================
rem parameters passed are:

set CLOCK=%1
set PROC=%2
set DBG=%3
set RX=%4
set CFG=%5
set EXP=%6
set BRD=%7

for /f "tokens=2-4 delims=/ " %%a in ('date /T') do set year=%%c
for /f "tokens=2-4 delims=/ " %%a in ('date /T') do set month=%%a
for /f "tokens=2-4 delims=/ " %%a in ('date /T') do set day=%%b
set TODAY=%year%%month%%day%

for /f "tokens=1 delims=: " %%h in ('time /T') do set hour=%%h
for /f "tokens=2 delims=: " %%m in ('time /T') do set minutes=%%m
for /f "tokens=3 delims=: " %%a in ('time /T') do set ampm=%%a
set NOW=%hour%%minutes%%ampm%

set CSRC=leds stats eeprom math params accel adc uavx irq menu control compass baro gyro tests serial rc utils gps rangefinder telemetry temperature autonomous i2c outputs
set ASRC=bootl18f

rem Set all the name tokens for the HEX files
set G=
set E=
set D=
set T=
set R=
set B=
set L=

if "%DBG%" == "TESTING"     		set D=-TEST
if "%DBG%" == "SIMULATE"     		set D=-SIMULATOR
if "%CFG%" == "QUADROCOPTER"        set C=-QUAD
if "%CFG%" == "TRICOPTER"           set C=-TRI
if "%CFG%" == "HELICOPTER"			set C=-HELI
if "%CFG%" == "HEXACOPTER"			set C=-HEX
if "%CFG%" == "AILERON"				set C=-AILERON
if "%CFG%" == "ELEVON"				set C=-ELEVON

if "%BRD%" == "UAVXLIGHT"				set L=Light

if "%DBG%" == "TESTING"				set C=

if "%RX%" == "RX6CH"				set R=-6CH
if "%CLOCK%" == "CLOCK_16MHZ"    	set X=-16
if "%CLOCK%" == "CLOCK_40MHZ"     	set X=-40


if "%EXP%" == "EXPERIMENTAL"     		set E=EXP-

set CC="C:\MCC18\bin\mcc18" 
rem removed integer promotions set CCMD=  -Oi -w1 -Opa- -DBATCHMODE
set CCMD=  -w3 -Opa- -DBATCHMODE

set ACMD=/q /d%CLOCK% /p%PROC% %%i.asm /l%%i.lst /e%%i.err /o%%i.o
set AEXE="C:\MCC18\mpasm\mpasmwin.exe"

set LCMD=/p%PROC% /l"C:\MCC18\lib" /k"C:\MCC18\lkr"
set LEXE="C:\MCC18\bin\mplink.exe"

rem Build the list of expected object files
set F=
for %%i in ( %CSRC% ) do set F=!F! %%i.o
for %%i in ( %ASRC% ) do set F=!F! %%i.o

for %%i in ( %CSRC% ) do %CC% -p=%PROC% /i"C:\MCC18\h" %%i.c -fo=%%i.o %CCMD%  -D%CLOCK% -D%DBG% -D%RX% -D%CFG% -D%EXP% -D%BRD% >> log.lst

for %%i in ( %ASRC% ) do %AEXE%  %ACMD% >> log.lst

%LEXE% %LCMD% %F% /u_CRUNTIME /z__MPLAB_BUILD=1 /W /o UAVX%L%-V1.1192gke-%E%%PROC%%X%%R%%C%%D%%T%.hex >> log.lst 


if %ERRORLEVEL% == 1 goto FAILED

echo compiled - UAVX%L%-V1.1192gke-%E%%PROC%%X%%R%%C%%D%%T%.hex
echo compiled - UAVX%L%-V1.1192gke-%E%%X%%R%%C%%D%%T%.hex >> gen.lst
call makeclean.bat
goto FINISH

:FAILED
echo failed - UAVX%L%-V1.1192gke-%E%%PROC%%X%%R%%C%%D%%T%.hex
echo failed - UAVX%L%-V1.1192gke-%E%%PROC%%X%%R%%C%%D%%T%.hex >> gen.lst
rem don't delete working files

:FINISH















