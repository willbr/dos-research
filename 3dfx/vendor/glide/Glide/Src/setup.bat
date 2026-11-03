@ECHO OFF
REM ------------------------------------------
REM CSETUP.BAT SET UP 3DFX BUILD ENVIRONMENT
REM ------------------------------------------

IF "%1"=="MSVC"   SET CTARGET=MSVC
IF "%1"=="msvc"   SET CTARGET=MSVC
IF "%1"=="WATWIN" SET CTARGET=WATWIN
IF "%1"=="watwin" SET CTARGET=WATWIN
IF "%1"=="WATDOS" SET CTARGET=WATDOS
IF "%1"=="watdos" SET CTARGET=WATDOS
IF "%CTARGET%"=="" GOTO USAGE

REM ------------------------------------------
REM SETUP YOUR ENVIRONMENT VARIBALES HERE!!!
REM ------------------------------------------
rem SET BUILD_DRIVE=C:
rem SET BUILD_ROOT=C:\3dfx
rem SET MASM=C:\MASM611
rem SET DIRECTXSDK=C:\DX5SDK
rem SET MSVC=C:\msdev
rem SET NMAKE_DIR=C:\msdev\bin
rem SET WATCOM=d:\WATCOM


REM ------------------------------------------
REM STORE THE PATH
REM ------------------------------------------
IF NOT "%OLDPATH%"=="" GOTO DONTSTORE
SET OLDPATH=%PATH%
:DONTSTORE

echo.
echo NOTE: YOU MUST USE NMAKE 1.61.0000 OR HIGHER!!!
echo       You Are Using The Following Version Of Nmake
echo.
%NMAKE_DIR%\nmake /?

REM ------------------------------------------
REM CHECK CRITICAL VARS
REM ------------------------------------------
IF NOT "%BUILD_DRIVE%"=="" GOTO DRIVEDEFD
ECHO You must set the environment variable BUILD_DRIVE
ECHO   to the driver letter of the source tree.
ECHO eg. SET BUILD_DRIVE=C:
GOTO END
:DRIVEDEFD
IF NOT "%BUILD_ROOT%"=="" GOTO ROOTDEFD
ECHO You must set the environment variable BUILD_ROOT
ECHO    to the root directory of the source tree.
ECHO eg. SET BUILD_ROOT=\3dfx\devel 
ECHO NOTE: BUILD_ROOT doesn't need a drive letter.
GOTO END
:ROOTDEFD
IF NOT "%MASM%=="" GOTO MASMDEFD
ECHO You must set the environment variable MASM
ECHO    To the install directory for 
ECHO    Microsoft Assember VERSION 6.11 ( or greater )
ECHO eg. SET MASM=C:\DEVEL\MASM611
GOTO END
:MASMDEFD
IF NOT "%DIRECTXSDK%=="" GOTO DXSDKDEFD
ECHO You must set the environment variable DIRECTXSDK
ECHO    to the DirectX SDK directory.
ECHO eg. SET DIRECTXSDK=C:\DEVEL\DXSDK\SDK
GOTO END
:DXSDKDEFD


REM ------------------------------------------
REM SET COMMON VARS
REM ------------------------------------------
SET BUILD_ROOT_SST1=%BUILD_ROOT%\sst1
SET BUILD_ROOT_SWLIBS=%BUILD_ROOT%\swlibs
SET BUILD_ROOT_GCI=%BUILD_ROOT%\gci
SET BUILD_ROOT_ARCADE=%BUILD_ROOT_SWLIBS%\arcade
SET BUILD_ROOT_DEMOS=%BUILD_ROOT%\demos

REM ------------------------------------------
REM SWITCH TO COMPILER
REM ------------------------------------------
GOTO %CTARGET%

REM ------------------------------------------
REM MSVC Targetting Win32
REM ------------------------------------------
:MSVC
IF NOT "%MSVC%"=="" GOTO MSVCDEFD
ECHO You must set the environment variable MSVC
ECHO   to the install directory for Microsoft 
ECHO   visual C/C++ 4.2 or greater
ECHO eg. SET MSVC=C:\devel\msdev
GOTO END
:MSVCDEFD
SET PATH=%MSVC%\bin;%OLDPATH%
SET INCLUDE=%MSVC%\INCLUDE;%MSVC%\MFC\INCLUDE
SET LIB=%MSVC%\LIB
SET AT_LOADER_MG=1
SET AT_DRIVER_D3D=1
COPY %BUILD_ROOT%\glide\lib\win32\glide2x.lib %BUILD_ROOT%\glide\src\sst1\lib
COPY %BUILD_ROOT%\glide\lib\win32\fxmisc.lib %BUILD_ROOT%\glide\src\swlibs\lib
COPY %BUILD_ROOT%\glide\lib\win32\fxpci.lib %BUILD_ROOT%\glide\src\swlibs\lib

SET FX_COMPILER=MICROSOFT
SET FX_TARGET=WIN32
SET FX_HOST=DOS
GOTO FINISH


REM ------------------------------------------
REM Watcom Targetting Win32
REM ------------------------------------------
:WATWIN
IF NOT "%WATCOM%"=="" GOTO WATDEFD0
ECHO You must set the environment variable WATCOM
ECHO   to the install directory for Watcom
ECHO   C/C++ 10.6 or greater
ECHO eg. SET WATCOM=C:\devel\WATCOM
GOTO END
:WATDEFD0
SET PATH=%WATCOM%\binnt;%WATCOM%\binw;%OLDPATH%
SET INCLUDE=%WATCOM%\H;%WATCOM%\H\NT
SET LIB=%WATCOM%\LIB386\NT
SET AT_LOADER_MG=1
SET AT_DRIVER_D3D=1
COPY %BUILD_ROOT%\glide\lib\win32\glide2x.lib %BUILD_ROOT%\glide\src\sst1\lib
COPY %BUILD_ROOT%\glide\lib\win32\fxmisc.lib %BUILD_ROOT%\glide\src\swlibs\lib
COPY %BUILD_ROOT%\glide\lib\win32\fxpci.lib %BUILD_ROOT%\glide\src\swlibs\lib
SET FX_COMPILER=WATCOM
SET FX_TARGET=WIN32
SET FX_HOST=DOS
GOTO FINISH


REM ------------------------------------------
REM Watcom Targetting Dos
REM ------------------------------------------
:WATDOS
IF NOT "%WATCOM%"=="" GOTO WATDEFD1
ECHO You must set the environment variable WATCOM
ECHO   to the install directory for Watcom
ECHO   C/C++ 10.6 or greater
ECHO eg. SET WATCOM=C:\devel\WATCOM
GOTO END
:WATDEFD1
SET PATH=%WATCOM%\binnt;%WATCOM%\binw;%OLDPATH%
SET INCLUDE=%WATCOM%\H
SET LIB=%WATCOM%\LIB386\DOS
COPY %BUILD_ROOT%\glide\lib\dos\stack\glide2x.lib %BUILD_ROOT%\glide\src\sst1\lib
COPY %BUILD_ROOT%\glide\lib\dos\stack\fxmisc.lib %BUILD_ROOT%\glide\src\swlibs\lib
COPY %BUILD_ROOT%\glide\lib\dos\stack\fxdpmi.lib %BUILD_ROOT%\glide\src\swlibs\lib
COPY %BUILD_ROOT%\glide\lib\dos\stack\fxpci.lib %BUILD_ROOT%\glide\src\swlibs\lib
SET FX_COMPILER=WATCOM
SET FX_TARGET=DOS
SET FX_HOST=DOS
GOTO FINISH

REM ------------------------------------------
REM MISC VARIABLES GO HERE
REM ------------------------------------------
:FINISH
SET AT_DRIVER_GLIDE=1
SET FX_DLL_BUILD=1
SET FX_NO_GEN_BOF=1
GOTO END

GOTO END
:USAGE
ECHO CSETUP.BAT - Set up 3Dfx Software Build Environment
ECHO USAGE: CSETUP buildspec
ECHO where buildspec is:
ECHO         msvc   - Microsoft Visual C Targetting Win32
ECHO         watwin - Watcom C/C++ 10.6 Targetting Win32
ECHO         watdos - Watcom C/C++ 10.6 Targetting Dos
GOTO END

:END
set CTARGET=

