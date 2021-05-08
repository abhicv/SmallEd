@echo off
set lflags= SDL2.lib SDL2main.lib Shell32.lib User32.lib Shcore.lib

set sdl2_inlcude= /I..\SDL2\include\
set smalled_src= /I..\src\
set stb_include= /I..\STB\

set exec= sed_app.exe

if not exist bin mkdir bin
pushd bin

if exist %exec% del %exec%

rc /r /nologo smalled.rc
cl /nologo /O2 /Oi ..\src\sed_app.c %sdl2_inlcude% %smalled_src% %stb_include% /link smalled.res /incremental:no /subsystem:windows /libpath:..\SDL2\lib\x64 %lflags%

popd bin
