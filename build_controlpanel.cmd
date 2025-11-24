@echo off
REM ============================================================================
REM Build Script for ControlPanel (User-Mode Application Only)
REM No Windows Driver Kit (WDK) required!
REM ============================================================================

echo.
echo ========================================================================
echo   Building ControlPanel - Security Research Control Panel
echo ========================================================================
echo.

REM Check if we're in the right directory
if not exist "ControlPanel\ControlPanel.vcxproj" (
    echo ERROR: ControlPanel\ControlPanel.vcxproj not found!
    echo Please run this script from the HWIDSpoofer root directory.
    echo.
    pause
    exit /b 1
)

REM Find MSBuild
set MSBUILD_PATH=

REM Try Visual Studio 2022
if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" (
    set MSBUILD_PATH=C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe
    goto :found_msbuild
)
if exist "C:\Program Files\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe" (
    set MSBUILD_PATH=C:\Program Files\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe
    goto :found_msbuild
)
if exist "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\MSBuild\Current\Bin\MSBuild.exe" (
    set MSBUILD_PATH=C:\Program Files\Microsoft Visual Studio\2022\Enterprise\MSBuild\Current\Bin\MSBuild.exe
    goto :found_msbuild
)

REM Try Visual Studio 2019
if exist "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\MSBuild\Current\Bin\MSBuild.exe" (
    set MSBUILD_PATH=C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\MSBuild\Current\Bin\MSBuild.exe
    goto :found_msbuild
)
if exist "C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\MSBuild\Current\Bin\MSBuild.exe" (
    set MSBUILD_PATH=C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\MSBuild\Current\Bin\MSBuild.exe
    goto :found_msbuild
)
if exist "C:\Program Files (x86)\Microsoft Visual Studio\2019\Enterprise\MSBuild\Current\Bin\MSBuild.exe" (
    set MSBUILD_PATH=C:\Program Files (x86)\Microsoft Visual Studio\2019\Enterprise\MSBuild\Current\Bin\MSBuild.exe
    goto :found_msbuild
)

REM MSBuild not found
echo ERROR: MSBuild.exe not found!
echo.
echo Please install Visual Studio 2019 or 2022 with C++ workload.
echo Download: https://visualstudio.microsoft.com/downloads/
echo.
pause
exit /b 1

:found_msbuild
echo Found MSBuild: %MSBUILD_PATH%
echo.

REM Ask for configuration
echo Select build configuration:
echo [1] Debug   (for development and debugging)
echo [2] Release (optimized, for production/testing)
echo.
set /p BUILD_CONFIG="Enter choice (1 or 2) [default: 2]: "

if "%BUILD_CONFIG%"=="" set BUILD_CONFIG=2
if "%BUILD_CONFIG%"=="1" (
    set CONFIG=Debug
) else (
    set CONFIG=Release
)

echo.
echo Building ControlPanel in %CONFIG% configuration for x64...
echo.

REM Clean previous build
echo [1/3] Cleaning previous build...
"%MSBUILD_PATH%" ControlPanel\ControlPanel.vcxproj /t:Clean /p:Configuration=%CONFIG% /p:Platform=x64 /v:minimal /nologo

REM Build ControlPanel
echo.
echo [2/3] Building ControlPanel...
"%MSBUILD_PATH%" ControlPanel\ControlPanel.vcxproj /t:Build /p:Configuration=%CONFIG% /p:Platform=x64 /v:minimal /nologo

if errorlevel 1 (
    echo.
    echo ========================================================================
    echo   BUILD FAILED!
    echo ========================================================================
    echo.
    echo Check the error messages above for details.
    echo Common issues:
    echo   - Missing Windows SDK: Install via Visual Studio Installer
    echo   - Missing C++ workload: Install "Desktop development with C++"
    echo.
    pause
    exit /b 1
)

echo.
echo [3/3] Verifying output...

set OUTPUT_PATH=bin\x64\%CONFIG%\ControlPanel.exe

if exist "%OUTPUT_PATH%" (
    echo.
    echo ========================================================================
    echo   BUILD SUCCESSFUL!
    echo ========================================================================
    echo.
    echo Output: %OUTPUT_PATH%

    REM Get file size
    for %%A in ("%OUTPUT_PATH%") do (
        echo Size: %%~zA bytes
        echo Modified: %%~tA
    )

    echo.
    echo To run ControlPanel:
    echo   cd %OUTPUT_PATH%\..
    echo   ControlPanel.exe
    echo.
    echo NOTE: Without drivers, ControlPanel will show "NOT LOADED" status
    echo but will still provide educational information and comparisons.
    echo.
    echo To build drivers, you need Windows Driver Kit (WDK).
    echo See BUILD.md for full instructions.
    echo.

    REM Ask if user wants to run it
    set /p RUN_NOW="Run ControlPanel now? (Y/N) [default: N]: "
    if /i "%RUN_NOW%"=="Y" (
        echo.
        echo Starting ControlPanel...
        echo.
        cd /d "%~dp0%OUTPUT_PATH%\.."
        start ControlPanel.exe
    )
) else (
    echo.
    echo ========================================================================
    echo   BUILD WARNING
    echo ========================================================================
    echo.
    echo Build completed but output file not found at expected location:
    echo %OUTPUT_PATH%
    echo.
    echo Check the build output above for errors.
    echo.
)

echo.
pause
