@echo off
@set path=%LOCALAPPDATA%\nasm;%path%
@set path=C:\MinGW\bin;%path%

echo Compiling: release
gcc -m32 -Wall -O1 -s -c -std=c99 blake\*.c
gcc -m32 -Wall -O1 -s -c *.c
echo Linking
gcc -m32 -s -shared -o cod4update.dll *.o -static-libgcc -lwinmm -Ltomcrypt -lws2_32 -lwsock32 -ltomcrypt -lgdi32
echo Cleaning up
del *.o

pause