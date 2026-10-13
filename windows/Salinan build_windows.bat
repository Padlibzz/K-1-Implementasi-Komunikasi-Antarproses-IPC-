@echo off
rem build_windows.bat - kompilasi klien pesanan.exe di Windows
rem Pakai g++ (MinGW/MSYS2) jika ada, kalau tidak pakai cl (Visual Studio Build Tools).
where g++ >nul 2>nul
if %errorlevel%==0 (
    echo Memakai g++ ...
    g++ -std=c++17 -O2 -Wall -o pesanan.exe pesanan.cpp -lws2_32
    goto selesai
)
where cl >nul 2>nul
if %errorlevel%==0 (
    echo Memakai cl ...
    cl /nologo /EHsc /std:c++17 /O2 /Fe:pesanan.exe pesanan.cpp ws2_32.lib
    goto selesai
)
echo Tidak menemukan g++ maupun cl.
echo Pasang MSYS2 (g++) atau "Developer Command Prompt for VS", lalu jalankan lagi.
exit /b 1
:selesai
if exist pesanan.exe (echo Berhasil: pesanan.exe) else (echo Gagal kompilasi)
