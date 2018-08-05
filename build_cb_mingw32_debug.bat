@rem Скрипт для сборки через mingw32 с CodeBlocks.
@rem Отладочная версия.
@echo off

@rem Количество ядер процессора - можно поставить больше единицы для ускорения сборки.
set CPU_NUM=2

@rem Проверим наличие необходимых переменных окружения
IF "%DevLibsMingw32%"=="" (
	echo Variable DevLibsMingw32 is not set!
	pause
	exit /b 1
)
IF "%msysdir%"=="" (
	echo Variable msysdir is not set!
	pause
	exit /b 1
)

@echo.
@echo Preparing environment...
@echo.
set PATH=%msysdir%\MinGW32\bin;%PATH%
set MSYSTEM=MINGW32
@echo.
@echo Preparing and cleaning project directory...
@echo.
mkdir cb_mingw32_build_debug
cd cb_mingw32_build_debug
@rem dll-ки :(. Тащим boost, gdal, icu и iconv.
@rem Два последних - т.к. неизвестно от чего именно зависит boost, он мог быть
@rem собран и так и так.
del /Q .\bin\*.dll
mkdir bin
copy /Y %DevLibsMingw32%\lib\libboost_locale*.dll bin\
copy /Y %DevLibsMingw32%\lib\libboost_system*.dll bin\
copy /Y %DevLibsMingw32%\lib\libboost_thread*.dll bin\
copy /Y %DevLibsMingw32%\lib\libboost_filesystem*.dll bin\
copy /Y %DevLibsMingw32%\lib\libboost_program_options*.dll bin\
copy /Y %DevLibsMingw32%\lib\libboost_unit_test_framework*.dll bin\
copy /Y %DevLibsMingw32%\lib\icuuc?*.dll bin\
copy /Y %DevLibsMingw32%\lib\icudt?*.dll bin\
copy /Y %DevLibsMingw32%\lib\icuin?*.dll bin\
copy /Y %DevLibsMingw32%\bin\libiconv-2.dll bin\
copy /Y %DevLibsMingw32%\bin\libgdal-20.dll bin\
@rem dll-ки компиллятора.
copy /Y %msysdir%\MinGW32\bin\libgcc_s_dw2-1.dll bin\
copy /Y %msysdir%\MinGW32\bin\libwinpthread-1.dll bin\
copy /Y "%msysdir%\MinGW32\bin\libstdc++-6.dll" bin\
copy /Y "%msysdir%\MinGW32\bin\libgcc_s_seh-1.dll" bin\
@rem собсно собираем.
@echo.
@echo Configuring...
@echo.
SET CMAKE_PREFIX_PATH=%DevLibsMingw32%
cmake -G "CodeBlocks - MinGW Makefiles" -D CMAKE_BUILD_TYPE=Debug -D CMAKE_RUNTIME_OUTPUT_DIRECTORY=.\bin ..\source
if NOT %ERRORLEVEL% == 0 (
  echo FAILED!!!
  pause
  exit /B 1
)
@echo.
@echo Building...
@echo.
mingw32-make.exe -j%CPU_NUM%
if NOT %ERRORLEVEL% == 0 (
  echo FAILED!!!
  pause
  exit /B 1
)
@echo.
@echo Build finished succesfully.
@echo.
pause