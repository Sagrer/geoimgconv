@rem Скрипт для сборки отладочной версии через CodeBlocks
mkdir cb_build_debug
mkdir cb_build_debug\PDCurses
cd cb_build_debug
set PATH=%CodeBlocksDir%\MinGW\bin;%PATH%
set CC=%CodeBlocksDir%\MinGW\bin\gcc
set CXX=%CodeBlocksDir%\MinGW\bin\g++
cmake -G "CodeBlocks - MinGW Makefiles" -D CMAKE_BUILD_TYPE=Debug ../source
mingw32-make.exe
pause
