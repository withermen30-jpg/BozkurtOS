@echo off
setlocal EnableExtensions
cd /d "%~dp0"
set "MODE=%~1"
set "RUN=%~2"
if "%MODE%"=="" set "MODE=auto"

echo ============================================
echo  BOZKIR OS - Windows -^> WSL build
echo ============================================

where wsl >nul 2>nul
if errorlevel 1 (
    echo [HATA] WSL kurulu degil.
    echo PowerShell: wsl --install
    exit /b 1
)

for /f "usebackq delims=" %%i in (`wsl -e wslpath -a "%CD%"`) do set "WSLDIR=%%i"
if not defined WSLDIR (
    echo [HATA] WSL yolu cozulamadi.
    exit /b 1
)

echo Proje (WSL): %WSLDIR%
echo Mod: %MODE%
wsl -e bash -c "cd '%WSLDIR%' && sed -i 's/\r$//' build.sh && bash build.sh %MODE% %RUN%"
if errorlevel 1 (
    echo.
    echo [HATA] Derleme basarisiz. Ustteki ciktiyi kontrol et.
    exit /b 1
)

echo.
echo [OK] ISO: %CD%\dist\bozkir.iso
echo [OK] IMG: %CD%\dist\bozkir.img
echo.
echo Test (Windows QEMU varsa):
echo   qemu-system-x86_64 -m 128 -cdrom dist\bozkir.iso
echo Test (WSL):
echo   wsl qemu-system-x86_64 -m 128 -cdrom dist/bozkir.iso -serial stdio
endlocal
exit /b 0
