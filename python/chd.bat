@echo off
set "TEMP_OUT=%TEMP%\chd_target.tmp"

rem python でスクリプトを実行し、結果を一時ファイルへ
python chds.py > "%TEMP_OUT%"

if exist "%TEMP_OUT%" (
    set /p TARGET_DIR=<"%TEMP_OUT%"
    del "%TEMP_OUT%"
)

if not "%TARGET_DIR%"=="" (
    cd /d "%TARGET_DIR%"
)

set TARGET_DIR=
set TEMP_OUT=
