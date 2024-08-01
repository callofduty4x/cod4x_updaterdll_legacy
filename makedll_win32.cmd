@echo off

echo Compiling: release
gcc -m32 -Wall -O1 -s -c -std=c99 blake\*.c
gcc -m32 -Wall -O1 -s -c *.c
echo Linking
gcc -m32 -s -shared -o cod4update.dll *.o -static-libgcc -lwinmm -L..\
echo Cleaning up
del *.o

pause