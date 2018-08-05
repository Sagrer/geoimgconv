@rem Скрипт для сборки через mingw64 с CodeBlocks.
@rem Релизная версия.
@echo off

@rem Количество ядер процессора - можно поставить больше единицы для ускорения сборки.
set CPU_NUM=2
@rem Запускать ли тесты (yes\no).
set RUN_TESTS="yes"

@rem Проверим наличие необходимых переменных окружения
IF "%DevLibsMingw64%"=="" (
	echo Variable DevLibsMingw64 is not set!
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
set PATH=%msysdir%\MinGW64\bin;%PATH%
set MSYSTEM=MINGW64
@echo.
@echo Preparing and cleaning project directory...
@echo.
mkdir cb_mingw64_build_release
cd cb_mingw64_build_release
@rem dll-ки :(. Тащим boost, gdal, icu и iconv.
@rem Два последних - т.к. неизвестно от чего именно зависит boost, он мог быть
@rem собран и так и так.
del /Q .\bin\*.dll
mkdir bin
copy /Y %DevLibsMingw64%\lib\libboost_locale*.dll bin\
copy /Y %DevLibsMingw64%\lib\libboost_system*.dll bin\
copy /Y %DevLibsMingw64%\lib\libboost_thread*.dll bin\
copy /Y %DevLibsMingw64%\lib\libboost_filesystem*.dll bin\
copy /Y %DevLibsMingw64%\lib\libboost_program_options*.dll bin\
copy /Y %DevLibsMingw64%\lib\libboost_unit_test_framework*.dll bin\
copy /Y %DevLibsMingw64%\lib\icuuc?*.dll bin\
copy /Y %DevLibsMingw64%\lib\icudt?*.dll bin\
copy /Y %DevLibsMingw64%\lib\icuin?*.dll bin\
copy /Y %DevLibsMingw64%\bin\libiconv-2.dll bin\
copy /Y %DevLibsMingw64%\bin\libgdal-20.dll bin\
@rem dll-ки компиллятора.
copy /Y %msysdir%\MinGW64\bin\libgcc_s_dw2-1.dll bin\
copy /Y %msysdir%\MinGW64\bin\libwinpthread-1.dll bin\
copy /Y "%msysdir%\MinGW64\bin\libstdc++-6.dll" bin\
copy /Y "%msysdir%\MinGW64\bin\libgcc_s_seh-1.dll" bin\
@rem собсно собираем.
@echo.
@echo Configuring...
@echo.
SET CMAKE_PREFIX_PATH=%DevLibsMingw64%
cmake -G "CodeBlocks - MinGW Makefiles" -D CMAKE_BUILD_TYPE=Release -D CMAKE_RUNTIME_OUTPUT_DIRECTORY=.\bin ..\source
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
if %RUN_TESTS% == "yes" (
	echo Running tests...
	echo.
	.\bin\geoimgconv_tests.exe --show_progress
)
if %RUN_TESTS% == "yes" (
	if NOT %ERRORLEVEL% == 0 (
		echo FAILED!!!
		pause
		exit /B 1
	)
)
pause