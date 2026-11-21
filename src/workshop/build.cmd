@echo off
setlocal
pushd "%~dp0"
if not defined WORKSHOP_MOD_ROOT (
 if exist "%~dp0..\d2rloader\plugins" (for %%I in ("%~dp0..") do set "WORKSHOP_MOD_ROOT=%%~fI") else (for %%I in ("%~dp0..\..") do set "WORKSHOP_MOD_ROOT=%%~fI")
)
if not exist "%WORKSHOP_MOD_ROOT%\d2rloader\plugins" mkdir "%WORKSHOP_MOD_ROOT%\d2rloader\plugins"
rc /nologo /I SDK\include passives.rc
if errorlevel 1 exit /b 1
cl /nologo /std:c++latest /O2 /EHsc /LD /MT /I SDK\include passives.cpp passives.res /Fe:"%WORKSHOP_MOD_ROOT%\d2rloader\plugins\d2rl-astral-workshop.dll"
exit /b %errorlevel%
