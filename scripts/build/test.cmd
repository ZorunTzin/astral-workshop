call "C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64 >nul
cd /d "%~dp0workshop"
cl /nologo /std:c++latest /EHsc /MT /I SDK\include atlas_test.cpp /Fe:atlas_test.exe && atlas_test.exe






