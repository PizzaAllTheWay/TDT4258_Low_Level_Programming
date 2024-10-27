@echo off
REM Clean up old files
del /Q *.o *.axf

REM Compile C source to object file
arm-none-eabi-gcc -Wall -g -O1 -mcpu=cortex-a9 -nostdlib -c breakout.c -o breakout.o

REM Assemble the startup assembly file
arm-none-eabi-as startup.s -o startup.o

REM Link the object files
arm-none-eabi-gcc "-Wl,--defsym,arm_program_mem=0x0" "-Wl,--defsym,arm_available_mem_size=0x3ffffff8" "-Wl,--defsym,__cs3_stack=0x3ffffff8" -T"C:/intelFPGA_lite/18.1/University_Program/Monitor_Program/build/altera-socfpga-hosted.ld" breakout.o startup.o -o breakout.axf -nostdlib

REM Check if the build succeeded
if %errorlevel% neq 0 (
    echo Build failed with error %errorlevel%
    exit /b %errorlevel%
) else (
    echo Build succeeded!
)

pause
