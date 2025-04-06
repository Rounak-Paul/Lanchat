@echo off
echo Compiling ChatApp for Windows...

g++ ChatApp.cpp -o ChatApp.exe -lws2_32 -static -static-libgcc -static-libstdc++

if %ERRORLEVEL% neq 0 (
    echo Compilation failed.
) else (
    echo Build successful! Output: ChatApp.exe
)
pause
