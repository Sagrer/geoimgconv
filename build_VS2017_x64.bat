@rem Скрипт для сборки 64-битной версии через VisualStudio 2017.
@echo off

@rem Запускать ли тесты (yes\no).
set RUN_TESTS="yes"

@rem Проверим наличие необходимой переменной окружения
IF "%DevLibsVs2017_x64%"=="" (
	echo Variable DevLibsVs2017_x64 is not set!
	pause
	exit /b 1
)
@rem По умолчанию собирается релизная сборка.
@echo.
@echo Preparing environment...
@echo.
call detect_vs2017.bat
if NOT %ERRORLEVEL% == 0 (
  echo Visual studio 2017 was not detected! Please install latest update or set VS150COMNTOOLS env variable to Common7\Tools directory of VS2017 install directory.
  pause
  exit /b 1
)
@echo.
@echo Preparing and cleaning project directory...
@echo.
mkdir vs2017_x64_build\Debug
mkdir vs2017_x64_build\Release
cd vs2017_x64_build
@rem dll-ки :(
del /Q Debug\*.dll
del /Q Release\*.dll
copy /Y %DevLibsVs2017_x64%\lib\boost_locale*.dll Debug\
copy /Y %DevLibsVs2017_x64%\lib\boost_system*.dll Debug\
copy /Y %DevLibsVs2017_x64%\lib\boost_chrono*.dll Debug\
copy /Y %DevLibsVs2017_x64%\lib\boost_thread*.dll Debug\
copy /Y %DevLibsVs2017_x64%\lib\boost_filesystem*.dll Debug\
copy /Y %DevLibsVs2017_x64%\lib\boost_date_time*.dll Debug\
copy /Y %DevLibsVs2017_x64%\lib\boost_program_options*.dll Debug\
copy /Y %DevLibsVs2017_x64%\lib\boost_unit_test_framework*.dll Debug\
copy /Y %DevLibsVs2017_x64%\bin\icuuc*.dll Debug\
copy /Y %DevLibsVs2017_x64%\bin\icudt*.dll Debug\
copy /Y %DevLibsVs2017_x64%\bin\icuin*.dll Debug\
@rem copy /Y %DevLibsVs2017_x64%\bin\pdcurses.dll Debug\
copy /Y %DevLibsVs2017_x64%\bin\gdal*.dll Debug\
copy /Y Debug\*.dll Release\
@rem собсно собираем.
@echo.
@echo Configuring...
@echo.
SET CMAKE_PREFIX_PATH=%DevLibsVs2017_x64%
@rem BOOST_SUPR_OUTDATED подавляет вывод мусора при сборке с бустом 1.66.0
cmake -DBOOST_SUPR_OUTDATED=ON -G "Visual Studio 15 2017 Win64" ../source
if NOT %ERRORLEVEL% == 0 (
  echo FAILED!!!
  pause
  exit /B 1
)
@echo.
@echo Building...
@echo.
"%VS150COMNTOOLS%\..\IDE\devenv.com" geoimgconv.sln /build Release
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
	.\release\geoimgconv_tests.exe --show_progress
)
if %RUN_TESTS% == "yes" (
	if NOT %ERRORLEVEL% == 0 (
		echo FAILED!!!
		pause
		exit /B 1
	)
)
pause