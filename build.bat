@echo off
set lflags= SDL2.lib SDL2main.lib Shell32.lib

set sdl2_inlcude= /ISDL2\include\

if exist main.exe del main.exe

cl /nologo main.c %sdl2_inlcude% /link /subsystem:windows /libpath:SDL2\lib\x64 %lflags%

if exist main.exe main.exe