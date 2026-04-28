REM @echo off
setlocal enabledelayedexpansion

REM -----------------------------
REM CONFIG
REM -----------------------------

set UE_PATH=E:\UE\UE_5.7
set PROJECT_DIR=%~dp0
set PROJECT=Happiness.uproject
set CONFIG_FILE=%PROJECT_DIR%Config\DefaultEngine.local.ini
set BUILD_OUTPUT=%PROJECT_DIR%Builds\AndroidShipping


REM -----------------------------
REM CLEAN BUILD OUTPUT
REM -----------------------------

if exist "%BUILD_OUTPUT%" (
    echo Cleaning build output folder...
    rd /s /q "%BUILD_OUTPUT%"
)

mkdir "%BUILD_OUTPUT%"

REM -----------------------------
REM READ CURRENT STORE VERSION
REM -----------------------------

for /f "tokens=1,2 delims==" %%A in ('findstr StoreVersion "%CONFIG_FILE%"') do (
    set VERSION=%%B
)

echo Current version: %VERSION%

REM -----------------------------
REM INCREMENT
REM -----------------------------

set /a NEW_VERSION=%VERSION%+1

echo New version: %NEW_VERSION%

REM -----------------------------
REM UPDATE INI
REM -----------------------------

REM powershell -Command "(Get-Content '%CONFIG_FILE%') -replace 'StoreVersion=%VERSION%', 'StoreVersion=%NEW_VERSION%' | Set-Content '%CONFIG_FILE%'"

REM -----------------------------
REM BUILD WIN64
REM -----------------------------
cmd /c ""%UE_PATH%\Engine\Build\BatchFiles\Build.bat" HappinessEditor Win64 Development "%PROJECT_DIR%%PROJECT%""


REM -----------------------------
REM BUILD ANDROID (SHIPPING)
REM -----------------------------
cmd /c ""%UE_PATH%\Engine\Build\BatchFiles\RunUAT.bat" BuildCookRun -nop4 -utf8output -build -cook -project="%PROJECT_DIR%%PROJECT%" -target=Happiness -platform=Android -cookflavor=Multi -installed -SkipCookingErrorSummary -stage -archive -package -pak -iostore -compressed -prereqs -distribution -nodebuginfo -archivedirectory="%BUILD_OUTPUT%" -clientconfig=Shipping"


echo.
echo Build complete. Output in:
echo %BUILD_OUTPUT%
pause