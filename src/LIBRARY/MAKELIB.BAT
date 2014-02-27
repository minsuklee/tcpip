@rem  IMIGE/TCP, Copyright (C) 1995, IMIGE Systems Inc.
@rem
@echo off

SET BEFORE=UNKNOWN
if exist SMALL   SET BEFORE=SMALL
if exist LARGE   SET BEFORE=LARGE
if exist COMPACT SET BEFORE=COMPACT
if exist MEDIUM  SET BEFORE=MEDIUM
if exist HUGE    SET BEFORE=HUGE

SET MODEL=
if not ""=="%1" goto CHECK_ARG
if "%BEFORE%"=="SMALL"   SET MODEL=s
if "%BEFORE%"=="LARGE"   SET MODEL=l
if "%BEFORE%"=="COMPACT" SET MODEL=c
if "%BEFORE%"=="MEDIUM"  SET MODEL=m
if "%BEFORE%"=="HUGE"    SET MODEL=h
goto CHECK_MODEL

:CHECK_ARG
if %1==clean   goto CLEAN
if %1==CLEAN   goto CLEAN
if %1==small   SET MODEL=s
if %1==SMALL   SET MODEL=s
if %1==LARGE   SET MODEL=l
if %1==large   SET MODEL=l
if %1==COMPACT SET MODEL=c
if %1==compact SET MODEL=c
if %1==MEDIUM  SET MODEL=m
if %1==medium  SET MODEL=m
if %1==HUGE    SET MODEL=h
if %1==huge    SET MODEL=h

:CHECK_MODEL
if not "%MODEL%"=="" goto START

:USAGE
echo usage: MAKE MODEL(small, large, compact, medium, huge)
goto END

:START
if %MODEL%==s SET NOW=SMALL
if %MODEL%==l SET NOW=LARGE
if %MODEL%==c SET NOW=COMPACT
if %MODEL%==m SET NOW=MEDIUM
if %MODEL%==h SET NOW=HUGE
echo Making -[%NOW%]- Model Library.

if "%BEFORE%"=="%NOW%" goto MAKE_LIB
if not exist *.obj goto NEW_MAKE
echo But OBJ files are in -[%BEFORE%]- model.

if not exist %NOW% echo So, Now erase *.OBJ to make new -[%NOW%]- model library ?
if not exist %NOW% echo Type CTRL-C to stop erasing and making...
if not exist %NOW% pause
if not exist %NOW% if exist *.obj erase *.obj

:NEW_MAKE
if exist %BEFORE% erase %BEFORE%
if not exist %NOW% echo "MODEL_FILE" > %NOW%
:MAKE_LIB
%TC%\bin\make -DMODEL=%MODEL%
goto END

:CLEAN
%TC%\bin\make clean
:END

set MODEL=
set BEFORE=
set NOW=
