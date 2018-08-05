@rem Скрипт для сборки через VisualStudio 2015.
@echo off

@rem Запускать ли тесты (yes\no).
set RUN_TESTS="yes"

@rem Проверим наличие необходимой переменной окружения
IF "%DevLibsVs2015%"=="" (
	echo Variable DevLibsVs2015 is not set!
	pause
	exit /b 1
)
@rem По умолчанию собирается релизная сборка.
@echo.
@echo Preparing and cleaning project directory...
@echo.
mkdir vs2015_build\Debug
mkdir vs2015_build\Release
cd vs2015_build
@rem dll-ки :(
del /Q Debug\*.dll
del /Q Release\*.dll
copy /Y %DevLibsVs2015%\lib\boost_locale*.dll Debug\
copy /Y %DevLibsVs2015%\lib\boost_system*.dll Debug\
copy /Y %DevLibsVs2015%\lib\boost_chrono*.dll Debug\
copy /Y %DevLibsVs2015%\lib\boost_thread*.dll Debug\
copy /Y %DevLibsVs2015%\lib\boost_filesystem*.dll Debug\
copy /Y %DevLibsVs2015%\lib\boost_date_time*.dll Debug\
copy /Y %DevLibsVs2015%\lib\boost_program_options*.dll Debug\
copy /Y %DevLibsVs2015%\lib\boost_unit_test_framework*.dll Debug\
copy /Y %DevLibsVs2015%\bin\icuuc*.dll Debug\
copy /Y %DevLibsVs2015%\bin\icudt*.dll Debug\
copy /Y %DevLibsVs2015%\bin\icuin*.dll Debug\
@rem copy /Y %DevLibsVs2015%\bin\pdcurses.dll Debug\
copy /Y %DevLibsVs2015%\bin\gdal*.dll Debug\
copy /Y Debug\*.dll Release\
@rem собсно собираем.
@echo.
@echo Configuring...
@echo.
SET CMAKE_PREFIX_PATH=%DevLibsVs2015%
cmake -G "Visual Studio 14 2015" ../source
if NOT %ERRORLEVEL% == 0 (
  echo FAILED!!!
  pause
  exit /B 1
)
@echo.
@echo Building...
@echo.
"%VS140COMNTOOLS%\..\IDE\devenv.com" geoimgconv.sln /build Release
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