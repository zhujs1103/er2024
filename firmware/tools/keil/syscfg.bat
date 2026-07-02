@echo off

set "SYSCFG_PATH=D:\ti\sysconfig_1.28.0\sysconfig_cli.bat"
set "SDK_ROOT=D:\ti\mspm0_sdk_2_10_00_04"

if not exist "%SYSCFG_PATH%" (
    echo Couldn't find SysConfig tool: %SYSCFG_PATH%
    exit /b 1
)

if not exist "%SDK_ROOT%\.metadata\product.json" (
    echo Couldn't find MSPM0 SDK product metadata: %SDK_ROOT%\.metadata\product.json
    exit /b 1
)

set "PROJ_DIR=%~1"
set "PROJ_DIR=%PROJ_DIR:'=%"

set "SYSCFG_FILE=%~2"
set "SYSCFG_FILE=%SYSCFG_FILE:'=%"

set "SYSCFG_DIR=%PROJ_DIR%"
set iter=0

:syscfg_search_loop
if exist "%SYSCFG_DIR%\%SYSCFG_FILE%" (
    goto syscfg_search_exit
) else if %iter% geq 5 (
    echo Couldn't find %SYSCFG_FILE% from %PROJ_DIR%
    exit /b 1
) else (
    set /a iter=%iter%+1
    set "SYSCFG_DIR=%SYSCFG_DIR%..\"
    goto syscfg_search_loop
)

:syscfg_search_exit
if "%SYSCFG_DIR:~-1%"=="\" set "SYSCFG_DIR=%SYSCFG_DIR:~0,-1%"

"%SYSCFG_PATH%" -o "%SYSCFG_DIR%" -s "%SDK_ROOT%\.metadata\product.json" --compiler keil "%SYSCFG_DIR%\%SYSCFG_FILE%"
exit /b %ERRORLEVEL%
