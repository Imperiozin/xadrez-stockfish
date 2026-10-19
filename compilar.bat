@echo off
setlocal
cd /d "%~dp0"
where py >nul 2>nul
if not errorlevel 1 (
    py -3 tools\build.py %*
    exit /b
)
python tools\build.py %*
exit /b
