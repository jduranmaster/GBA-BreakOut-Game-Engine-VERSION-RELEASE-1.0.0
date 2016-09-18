path=D:\devkitadv\bin

gcc -c -O3 -mthumb -mthumb-interwork BreakOut.cpp
gcc -mthumb -mthumb-interwork -o BreakOut.elf BreakOut.o

objcopy -O binary BreakOut.elf BreakOut.gba

.\tools\gbafix.exe BreakOut.gba -p -tBreakOut -c8009 -m86 -r1

del *.elf
del *.o

pause