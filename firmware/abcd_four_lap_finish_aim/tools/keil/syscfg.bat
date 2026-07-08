@echo off
setlocal

set "SYSCFG_CLI=D:\ti\sysconfig_1.28.0\sysconfig_cli.bat"
set "PRODUCT_JSON=D:\ti\mspm0_sdk_2_10_00_04\.metadata\product.json"
set "OUT_DIR=%~dp0..\.."

for %%I in ("%OUT_DIR%") do set "OUT_DIR=%%~fI"

set "ARG1=%~1"
set "ARG1=%ARG1:'=%"
set "ARG2=%~2"
set "ARG2=%ARG2:'=%"

if "%ARG2%"=="" (
    if "%ARG1%"=="" (
        set "SYSCFG_FILE=empty.syscfg"
    ) else (
        set "SYSCFG_FILE=%ARG1%"
    )
) else (
    set "SYSCFG_FILE=%ARG2%"
)

set "SYSCFG_PATH=%OUT_DIR%\%SYSCFG_FILE%"

if not exist "%SYSCFG_CLI%" (
    echo Could not find SysConfig CLI: %SYSCFG_CLI%
    exit /b 1
)

if not exist "%PRODUCT_JSON%" (
    echo Could not find MSPM0 SDK product metadata: %PRODUCT_JSON%
    exit /b 1
)

if not exist "%SYSCFG_PATH%" (
    echo Could not find SysConfig input: %SYSCFG_PATH%
    exit /b 1
)

call "%SYSCFG_CLI%" -s "%PRODUCT_JSON%" -o "%OUT_DIR%" --compiler keil "%SYSCFG_PATH%"
exit /b %ERRORLEVEL%
