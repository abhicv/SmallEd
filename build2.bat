@echo off
set lflags= SDL2.lib SDL2main.lib Shell32.lib

set sdl2_inlcude= /I..\SDL2\include\
set smalled_src= /I..\src\
set stb_include= /I..\STB\

if not exist bin mkdir bin
pushd bin
if exist main_2.exe del main_2.exe
cl /Zi /nologo ..\src\main_2.c %sdl2_inlcude% %smalled_src% %stb_include% /link /incremental:no /subsystem:console /libpath:..\SDL2\lib\x64 %lflags%
if exist main_2.exe main_2.exe
popd bin
