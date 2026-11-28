@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64 >nul
cd /d "%~dp0workshop"
cl /nologo /std:c++latest /EHsc /MT /I SDK\include set_reforge_test.cpp /Fe:set_reforge_test.exe
if not "%errorlevel%"=="0" exit /b 1
set_reforge_test.exe
if not "%errorlevel%"=="0" exit /b 1
cd combat
cl /nologo /std:c++latest /EHsc /O2 /MT /I ..\SDK\include aura_features_test.cpp /Fe:aura_features_test.exe
if not "%errorlevel%"=="0" exit /b 1
aura_features_test.exe
if not "%errorlevel%"=="0" exit /b 1

cl /nologo /std:c++latest /O2 /EHsc /MT "%~dp0workshop\leagues\stash_visibility_test.cpp" /Fe:"%~dp0workshop\leagues\stash_visibility_test.exe"
if errorlevel 1 exit /b 1
"%~dp0workshop\leagues\stash_visibility_test.exe"
if errorlevel 1 exit /b 1
