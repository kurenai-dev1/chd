@echo off
rem chds.exe（旧chd.exe）の出力を TARGET_DIR 変数に格納
for /f "delims=" %%i in ('chds.exe') do set "TARGET_DIR=%%i"

if not "%TARGET_DIR%"=="" (
    cd /d "%TARGET_DIR%"
)

set TARGET_DIR=
