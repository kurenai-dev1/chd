@echo off
rem 一時ファイルの名前を定義（ユーザーの一時フォルダ内）
set "TEMP_OUT=%TEMP%\chd_target.tmp"

rem %~dp0 を使って、このバッチと同じフォルダにある chds.py を実行します
rem （%~dp0 は最後に「\」が自動で付くため、%~dp0chds.py と書きます）
python "%~dp0chds.py" > "%TEMP_OUT%"

rem 一時ファイルから確定したパスを読み込む
if exist "%TEMP_OUT%" (
    set /p TARGET_DIR=<"%TEMP_OUT%"
    del "%TEMP_OUT%"
)

rem パスが空でなければ移動
if not "%TARGET_DIR%"=="" (
    cd /d "%TARGET_DIR%"
)

rem 変数を掃除
set TARGET_DIR=
set TEMP_OUT=
