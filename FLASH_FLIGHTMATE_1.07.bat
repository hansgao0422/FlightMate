@echo off
setlocal
chcp 65001 >nul
cd /d "%~dp0"

set "PORT=%~1"
set "MODE=%~2"
set "TOOL=%~dp0tools\esptool.exe"

if not exist "%TOOL%" (
    echo [错误] 找不到 tools\esptool.exe
    pause
    exit /b 1
)

if not defined PORT set /p "PORT=请输入串口号（例如 COM7）: "
if not defined PORT (
    echo [错误] 未输入串口号。
    pause
    exit /b 1
)

if /I "%MODE%"=="fresh" goto fresh

echo FlightMate 1.07 默认升级：保留 NVS 设置并写入程序。
"%TOOL%" --chip esp32s3 --port "%PORT%" --baud 921600 --before default-reset --after hard-reset write-flash --compress ^
    0x0 "FlightMate-1.07-bootloader.bin" ^
    0x8000 "FlightMate-1.07-partitions.bin" ^
    0xe000 "FlightMate-1.07-boot_app0.bin" ^
    0x10000 "FlightMate-1.07-app.bin"
if errorlevel 1 goto fail
goto success

:fresh
echo FlightMate 1.07 全新烧录：将擦除设备全部 Flash 数据。
echo 按 Ctrl+C 可取消，按任意键继续。
pause >nul
"%TOOL%" --chip esp32s3 --port "%PORT%" --baud 921600 --before default-reset erase-flash
if errorlevel 1 goto fail
"%TOOL%" --chip esp32s3 --port "%PORT%" --baud 921600 --before default-reset --after hard-reset write-flash --compress ^
    0x0 "FlightMate-1.07-merged-16MB.bin"
if errorlevel 1 goto fail
goto success

:success
echo [完成] FlightMate 1.07 已烧录，设备将自动重启。
pause
exit /b 0

:fail
echo [失败] 请确认串口号，并尝试按住 BOOT 后重新连接 USB。
pause
exit /b 1
