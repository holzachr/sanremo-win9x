@echo off

REM call C:\DDK\Ddkenv.bat 32 net
set sdkroot=C:\MSTOOLS
set ddkroot=C:\DDK
set WIN16=
set include=%ddkroot%\%2\inc;%ddkroot%\inc32;%sdkroot%\include;%_include%;%ddkroot%\inc16;%sdkroot%\inc16
set lib=%ddkroot%\lib;%sdkroot%\lib;%_lib%
path=%ddkroot%\%2\bin;%ddkroot%\bin;%sdkroot%\bin;%_path%
REM prompt Windows 95 32-bit %2 build$_%_prompt%

REM call C:\Msvc20\BIN\Vcvars32.bat x86     
set PATH=C:\MSVC20\BIN;%PATH%
set INCLUDE=C:\MSVC20\INCLUDE;C:\MSVC20\MFC\INCLUDE;%INCLUDE%
set LIB=C:\MSVC20\LIB;C:\MSVC20\MFC\LIB;%LIB%

REM call C:\MASM611\BIN\New-vars.bat
SET PATH=C:\MASM611\BIN;%PATH%
SET LIB=C:\MASM611\LIB
SET INCLUDE=C:\MASM611\INCLUDE
SET INIT=C:\MASM611\INIT
SET HELPFILES=C:\MASM611\HELP\*.HLP
SET TMP=C:\MASM611\TMP


set MASTER_MAKE=1
set DDKROOT=c:\ddk

@echo on

nmake clean
nmake depend
nmake %1 > proj.err

REM type proj.err

REM pause
