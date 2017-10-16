@echo off

rem --------------------------------------------------------------------------
rem                        COMPILATION
rem --------------------------------------------------------------------------

set SDLPath=C:\SDL2-2.0.5\
set SDLBinPath=%SDLPath%\lib\x64\
set SDLNetPath=C:\SDL2_net-2.0.1\
set SDLNetBinPath=%SDLNetPath%\lib\x64\

set STBPath=..\..\stb\
set RivtenPath=..\..\rivten\

set UntreatedWarnings=/wd4100 /wd4244 /wd4201 /wd4127 /wd4505 /wd4456 /wd4996 /wd4003
set CommonCompilerDebugFlags=/MT /Od /Oi /fp:fast /fp:except- /Zo /Gm- /GR- /EHa /WX /W4 %UntreatedWarnings% /Z7 /nologo /I %SDLPath%\include\ /I %SDLNetPath%\include\ /I %STBPath% /I %RivtenPath%
set CommonLinkerDebugFlags=/incremental:no /opt:ref /subsystem:console %SDLBinPath%\SDL2.lib %SDLBinPath%\SDL2main.lib %SDLNetBinPath%\SDL2_net.lib /ignore:4099

pushd ..\build\
cl %CommonCompilerDebugFlags% ..\code\sdl_synchess.cpp /link %CommonLinkerDebugFlags%
cl %CommonCompilerDebugFlags% ..\code\synchess_server.cpp /link %CommonLinkerDebugFlags%
popd

rem --------------------------------------------------------------------------
echo Compilation completed...
