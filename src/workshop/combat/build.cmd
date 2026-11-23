@echo off
setlocal EnableDelayedExpansion
pushd "%~dp0"
if not defined WORKSHOP_MOD_ROOT (
 if exist "%~dp0..\..\d2rloader\plugins" (for %%I in ("%~dp0..\..") do set "WORKSHOP_MOD_ROOT=%%~fI") else (for %%I in ("%~dp0..\..\..") do set "WORKSHOP_MOD_ROOT=%%~fI")
)
ml64 /nologo /c /Fo monster_caption.obj ..\monster_modifiers\caption.asm
if not "!errorlevel!"=="0" exit /b 1
rc /nologo /I ..\SDK\include /fo combat.res src\virus.rc
if not "!errorlevel!"=="0" exit /b 1
cl /nologo /std:c++latest /EHsc /O2 /LD /MT /I ..\SDK\include src\virus.cpp combat.res monster_caption.obj /Fe:"%WORKSHOP_MOD_ROOT%\d2rloader\plugins\d2rl-workshop-combat.dll"
if not "!errorlevel!"=="0" exit /b 1
for %%T in (virus_test ramp_test pulse_test curses_test multitarget_test regen_timer_test soul_harvest_test) do (
 cl /nologo /std:c++latest /EHsc /O2 /MT /I ..\SDK\include src\%%T.cpp /Fe:%%T.exe
 if not "!errorlevel!"=="0" exit /b 1
 %%T.exe
 if not "!errorlevel!"=="0" exit /b 1
)
cl /nologo /std:c++latest /EHsc /O2 /MT /I ..\SDK\include aura_test.cpp /Fe:aura_test.exe
if not "!errorlevel!"=="0" exit /b 1
aura_test.exe
if not "!errorlevel!"=="0" exit /b 1
popd

pushd "%~dp0"
cl /nologo /std:c++latest /EHsc /O2 /MT /I ..\SDK\include aura_features_test.cpp /Fe:aura_features_test.exe
if not "!errorlevel!"=="0" exit /b 1
aura_features_test.exe
if not "!errorlevel!"=="0" exit /b 1
popd

pushd "%~dp0"
cl /nologo /std:c++latest /EHsc /O2 /MT /I ..\SDK\include guardian_test.cpp /Fe:guardian_test.exe
if not "!errorlevel!"=="0" exit /b 1
guardian_test.exe
if not "!errorlevel!"=="0" exit /b 1
popd

pushd "%~dp0"
cl /nologo /std:c++latest /EHsc /O2 /MT /I ..\SDK\include item_auras_test.cpp /Fe:item_auras_test.exe
if not "!errorlevel!"=="0" exit /b 1
item_auras_test.exe
if not "!errorlevel!"=="0" exit /b 1
popd

pushd "%~dp0"
cl /nologo /std:c++latest /EHsc /O2 /MT /I ..\SDK\include barbarian_test.cpp /Fe:barbarian_test.exe
if not "!errorlevel!"=="0" exit /b 1
barbarian_test.exe
if not "!errorlevel!"=="0" exit /b 1
popd

pushd "%~dp0"
cl /nologo /std:c++latest /EHsc /O2 /MT /I ..\SDK\include sorceress_test.cpp /Fe:sorceress_test.exe
if not "!errorlevel!"=="0" exit /b 1
sorceress_test.exe
if not "!errorlevel!"=="0" exit /b 1
popd

pushd "%~dp0"
cl /nologo /std:c++latest /EHsc /O2 /MT /I ..\SDK\include assassin_test.cpp /Fe:assassin_test.exe
if not "!errorlevel!"=="0" exit /b 1
assassin_test.exe
if not "!errorlevel!"=="0" exit /b 1
cl /nologo /std:c++latest /EHsc /O2 /MT ..\assassin_preview\engine_test.cpp /Fe:assassin_engine_test.exe
if not "!errorlevel!"=="0" exit /b 1
assassin_engine_test.exe
if not "!errorlevel!"=="0" exit /b 1
popd

pushd "%~dp0"
cl /nologo /std:c++latest /EHsc /O2 /MT /I ..\SDK\include amazon_test.cpp /Fe:amazon_test.exe
if not "!errorlevel!"=="0" exit /b 1
amazon_test.exe
if not "!errorlevel!"=="0" exit /b 1
popd

pushd "%~dp0"
cl /nologo /std:c++latest /EHsc /O2 /MT ..\monster_modifiers\engine_test.cpp /Fe:monster_modifiers_test.exe
if not "!errorlevel!"=="0" exit /b 1
monster_modifiers_test.exe
if not "!errorlevel!"=="0" exit /b 1
popd

pushd "%~dp0"
cl /nologo /std:c++latest /EHsc /O2 /MT /I ..\SDK\include ..\monster_modifiers\native_test.cpp /Fe:monster_native_test.exe
if not "!errorlevel!"=="0" exit /b 1
monster_native_test.exe
if not "!errorlevel!"=="0" exit /b 1
popd

pushd "%~dp0"
cl /nologo /std:c++latest /EHsc /O2 /MT /I ..\SDK\include ..\monster_modifiers\runtime_test.cpp /Fe:monster_runtime_test.exe
if not "!errorlevel!"=="0" exit /b 1
monster_runtime_test.exe
if not "!errorlevel!"=="0" exit /b 1
popd

pushd "%~dp0"
ml64 /nologo /c /Fo monster_caption_test.obj ..\monster_modifiers\caption_test.asm
if not "!errorlevel!"=="0" exit /b 1
cl /nologo /std:c++latest /EHsc /O2 /MT /I ..\SDK\include ..\monster_modifiers\caption_test.cpp monster_caption.obj monster_caption_test.obj /Fe:monster_caption_test.exe
if not "!errorlevel!"=="0" exit /b 1
monster_caption_test.exe
if not "!errorlevel!"=="0" exit /b 1
popd

pushd "%~dp0"
cl /nologo /std:c++latest /EHsc /O2 /MT /I ..\SDK\include druid_test.cpp /Fe:druid_test.exe
if not "!errorlevel!"=="0" exit /b 1
druid_test.exe
if not "!errorlevel!"=="0" exit /b 1
popd

pushd "%~dp0"
cl /nologo /std:c++latest /EHsc /O2 /MT ..\endgame\engine_test.cpp /Fe:arena_test.exe
if not "!errorlevel!"=="0" exit /b 1
arena_test.exe
if not "!errorlevel!"=="0" exit /b 1
popd

pushd "%~dp0"
cl /nologo /std:c++latest /EHsc /O2 /MT /I ..\SDK\include ..\endgame\runtime_test.cpp /Fe:arena_runtime_test.exe
if not "!errorlevel!"=="0" exit /b 1
arena_runtime_test.exe
if not "!errorlevel!"=="0" exit /b 1
popd

pushd "%~dp0"
for %%T in (engine_test runtime_test) do (
 cl /nologo /std:c++latest /EHsc /O2 /MT /I ..\SDK\include ..\goblins\%%T.cpp /Fe:goblin_%%T.exe
 if not "!errorlevel!"=="0" exit /b 1
 goblin_%%T.exe
 if not "!errorlevel!"=="0" exit /b 1
)
popd

rem League encounters use the same authoritative combat frame and lifecycle service.
pushd "%~dp0"
cl /nologo /std:c++latest /EHsc /O2 /MT /I ..\SDK\include ..\leagues\runtime_test.cpp /Fe:league_runtime_test.exe
if not "!errorlevel!"=="0" exit /b 1
league_runtime_test.exe
if not "!errorlevel!"=="0" exit /b 1
popd

pushd "%~dp0"
cl /nologo /std:c++latest /EHsc /O2 /MT /I ..\SDK\include ..\leagues\effects_test.cpp /Fe:league_effects_test.exe
if not "!errorlevel!"=="0" exit /b 1
league_effects_test.exe
if not "!errorlevel!"=="0" exit /b 1
popd

pushd "%~dp0"
cl /nologo /std:c++latest /EHsc /O2 /MT ..\leagues\craft_test.cpp /Fe:league_craft_test.exe
if not "!errorlevel!"=="0" exit /b 1
league_craft_test.exe
if not "!errorlevel!"=="0" exit /b 1
popd

pushd "%~dp0"
cl /nologo /std:c++latest /EHsc /O2 /MT /I ..\SDK\include amazon_masteries_test.cpp /Fe:amazon_masteries_test.exe
if not "!errorlevel!"=="0" exit /b 1
amazon_masteries_test.exe
if not "!errorlevel!"=="0" exit /b 1
popd
