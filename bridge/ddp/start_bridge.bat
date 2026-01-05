@echo off
echo Stopping any running Python processes...
taskkill /F /IM python.exe 2>nul
timeout /t 2 /nobreak >nul
echo.
echo Starting DDPico Bridge v2...
python ddp_serial_bridge_v2.py
