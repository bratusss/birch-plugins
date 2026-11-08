@echo off
setlocal
set "SRC=%~dp0Birch Keys.vst3"
set "DST=%CommonProgramFiles%\VST3"
net session >nul 2>&1
if %errorlevel% neq 0 (
    echo Vajag administratora tiesibas, atveru velreiz...
    powershell -Command "Start-Process -FilePath '%~f0' -Verb RunAs"
    exit /b
)
if not exist "%DST%" mkdir "%DST%"
echo Kopeju "%SRC%" uz "%DST%" ...
xcopy /E /I /Y "%SRC%" "%DST%\Birch Keys.vst3" >nul
if %errorlevel% equ 0 (
    echo Gatavs. FL Studio: Options ^> Manage plugins ^> Find plugins.
) else (
    echo Kopesana neizdevas.
)
pause
