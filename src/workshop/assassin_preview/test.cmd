@echo off
setlocal
call "C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64 >nul
if errorlevel 1 exit /b 1
pushd "%~dp0"
if not exist "..\..\..\build\assassin-preview" mkdir "..\..\..\build\assassin-preview"
cl /nologo /std:c++17 /EHsc /W4 /WX /O2 /Fo:"..\..\..\build\assassin-preview\engine_test.obj" /Fe:"..\..\..\build\assassin-preview\engine_test.exe" engine_test.cpp
if errorlevel 1 exit /b 1
"..\..\..\build\assassin-preview\engine_test.exe"
set result=%errorlevel%
popd
exit /b %result%
