@echo off
rem Builds the ShortStack editor modules and saves the full compiler output to build_log.txt.
rem Double-click it (with the Unreal Editor closed). To use a specific engine, pass its folder:
rem   BuildLog.bat "D:\Epic Games\UE_5.5"
setlocal EnableExtensions EnableDelayedExpansion
cd /d "%~dp0"
set "PROJECT=%~dp0ShortStack.uproject"
set "LOG=%~dp0build_log.txt"

rem ---- Find Unreal Engine: an argument wins, then Epic Games Launcher installs (newest first).
set "UE_ROOT=%~1"
if not defined UE_ROOT (
  for %%V in (5.9 5.8 5.7 5.6 5.5 5.4 5.3 5.2 5.1) do (
    if not defined UE_ROOT (
      for %%K in ("HKLM\SOFTWARE\EpicGames\Unreal Engine\%%V" "HKLM\SOFTWARE\WOW6432Node\EpicGames\Unreal Engine\%%V") do (
        if not defined UE_ROOT (
          for /f "tokens=2,*" %%A in ('reg query %%K /v InstalledDirectory 2^>nul ^| find "InstalledDirectory"') do set "UE_ROOT=%%B"
        )
      )
    )
  )
)
if not defined UE_ROOT (
  for /d %%D in ("%ProgramFiles%\Epic Games\UE_5.*") do set "UE_ROOT=%%~fD"
)
if not defined UE_ROOT (
  echo Could not find Unreal Engine 5. Run this again with the engine folder, for example:
  echo   BuildLog.bat "C:\Program Files\Epic Games\UE_5.5"
  goto :done
)
if not exist "%UE_ROOT%\Engine\Build\BatchFiles\Build.bat" (
  echo "%UE_ROOT%" does not look like an Unreal Engine folder: Engine\Build\BatchFiles\Build.bat is missing.
  goto :done
)
echo Unreal Engine: %UE_ROOT%

rem ---- Unreal compiles C++ with Visual Studio's MSVC toolchain.
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
set "VS_PATH="
if exist "%VSWHERE%" (
  "%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath > "%TEMP%\shortstack_vs.txt" 2>nul
  set /p VS_PATH=<"%TEMP%\shortstack_vs.txt"
)
if defined VS_PATH (
  echo Visual Studio C++ tools: !VS_PATH!
) else (
  echo.
  echo *** Visual Studio's C++ compiler was not found. ***
  echo Install Visual Studio 2022 Community ^(free^) from https://visualstudio.microsoft.com/
  echo and tick the "Game development with C++" workload. In its details panel, also tick
  echo "Unreal Engine installer" and a "Windows 11 SDK". Then run this file again.
  echo Trying the build anyway so the log shows what Unreal reports...
  echo.
)

echo Building ShortStackEditor ^(Win64 Development^). The first build takes a few minutes...
call "%UE_ROOT%\Engine\Build\BatchFiles\Build.bat" ShortStackEditor Win64 Development -Project="%PROJECT%" -WaitMutex > "%LOG%" 2>&1
set "RESULT=%ERRORLEVEL%"

echo.
if "%RESULT%"=="0" (
  echo BUILD SUCCEEDED. Double-click ShortStack.uproject to open the game in the editor.
) else (
  echo BUILD FAILED ^(exit code %RESULT%^). The first errors:
  echo ------------------------------------------------------------------
  findstr /i /r /c:"error [A-Z]*[0-9]" /c:": error" /c:"fatal error" /c:"ERROR:" "%LOG%" > "%TEMP%\shortstack_errors.txt"
  set /a SHOWN=0
  for /f "usebackq delims=" %%L in ("%TEMP%\shortstack_errors.txt") do (
    if !SHOWN! LSS 25 echo %%L
    set /a SHOWN+=1
  )
  if !SHOWN! EQU 0 type "%LOG%"
  echo ------------------------------------------------------------------
  echo Full output: %LOG%
  echo Paste those lines, or attach build_log.txt, in your chat with Claude.
)

:done
echo.
pause
