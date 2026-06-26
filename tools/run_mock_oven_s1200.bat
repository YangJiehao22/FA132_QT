@echo off
REM Offline-friendly: Python 3 only, NO pip / NO internet required.
cd /d "%~dp0"
echo Mock S1200 oven (stdlib, no pymodbus)...
echo.
py -3 mock_oven_s1200_stdlib.py --host 0.0.0.0 --port 8000
if errorlevel 1 (
  echo.
  echo If "py" not found, try: python mock_oven_s1200_stdlib.py --host 0.0.0.0 --port 8000
)
pause
