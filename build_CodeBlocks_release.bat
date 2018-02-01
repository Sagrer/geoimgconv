@rem Скрипт для сборки релизной версии через CodeBlocks
mkdir cb_build_release
mkdir cb_build_release\PDCurses
cd cb_build_release
set PATH=%CodeBlocksDir%\MinGW\bin;%PATH%
set CC=%CodeBlocksDir%\MinGW\bin\gcc
set CXX=%CodeBlocksDir%\MinGW\bin\g++
cmake -G "CodeBlocks - MinGW Makefiles" -D CMAKE_BUILD_TYPE=Release ../source
mingw32-make.exe
pause
