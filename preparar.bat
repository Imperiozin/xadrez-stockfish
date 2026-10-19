@echo off
setlocal
cd /d "%~dp0"
set "OUT=%~1"
if not defined OUT set "OUT=bin\Windows"
where py >nul 2>nul
if not errorlevel 1 (
    py -3 tools\setup_engine.py --output "%OUT%"
    exit /b
)
python tools\setup_engine.py --output "%OUT%"
exit /b
