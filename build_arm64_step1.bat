@echo off
setlocal enabledelayedexpansion
REM --- locate Visual Studio (with ARM64 tools) via vswhere, no hardcoded path ---
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
for /f "usebackq tokens=*" %%i in (`"!VSWHERE!" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.ARM64 -property installationPath`) do set "VSINSTALL=%%i"
if not defined VSINSTALL (echo ERROR: Visual Studio with ARM64 build tools not found & exit /b 1)
call "!VSINSTALL!\VC\Auxiliary\Build\vcvarsamd64_arm64.bat"
if errorlevel 1 (echo VCVARS_FAILED & exit /b 1)
REM --- discover python via uv (skia/harfbuzz need python + python3) ---
for /f "delims=" %%i in ('uv python find 3.12') do set "PYEXE=%%i"
if defined PYEXE (
    for %%F in ("!PYEXE!") do set "PYDIR=%%~dpF"
    if "!PYDIR:~-1!"=="\" set "PYDIR=!PYDIR:~0,-1!"
    set "PATH=!PYDIR!;%PATH%"
    if not exist "!PYDIR!\python3.exe" copy "!PYEXE!" "!PYDIR!\python3.exe" >nul
)
cd /d "%~dp0"
echo === conan install (host=win-arm64, build=win-x86_64) ===
conan install . --build=missing -pr:h=conan\profiles\win-arm64 -pr:b=conan\profiles\win-x86_64 -o onetbb/*:tbbproxy=False -o skia-infinipaint/*:use_vulkan=True -c tools.build:jobs=8
echo CONAN_EXIT=%errorlevel%
