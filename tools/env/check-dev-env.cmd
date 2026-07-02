@echo off
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0check-dev-env.ps1"
exit /b %ERRORLEVEL%
