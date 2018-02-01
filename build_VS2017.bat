@rem Скрипт для сборки через VisualStudio 2017.
@echo off
@rem Проверим наличие необходимой переменной окружения
IF "%DevLibsVs2017%"=="" GOTO VarIsNotSet
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
mkdir vs2017_build\Debug
mkdir vs2017_build\Release
cd vs2017_build
@rem dll-ки :(
del /Q Debug\*.dll
del /Q Release\*.dll
copy /Y %DevLibsVs2017%\lib\boost_locale*.dll Debug\
copy /Y %DevLibsVs2017%\lib\boost_system*.dll Debug\
copy /Y %DevLibsVs2017%\lib\boost_chrono*.dll Debug\
copy /Y %DevLibsVs2017%\lib\boost_thread*.dll Debug\
copy /Y %DevLibsVs2017%\lib\boost_filesystem*.dll Debug\
copy /Y %DevLibsVs2017%\lib\boost_date_time*.dll Debug\
copy /Y %DevLibsVs2017%\lib\boost_program_options*.dll Debug\
copy /Y %DevLibsVs2017%\bin\icuuc*.dll Debug\
copy /Y %DevLibsVs2017%\bin\icudt*.dll Debug\
copy /Y %DevLibsVs2017%\bin\icuin*.dll Debug\
@rem copy /Y %DevLibsVs2017%\bin\pdcurses.dll Debug\
copy /Y %DevLibsVs2017%\bin\gdal*.dll Debug\
copy /Y Debug\*.dll Release\
@rem собсно собираем.
@echo.
@echo Configuring...
@echo.
SET CMAKE_PREFIX_PATH=%DevLibsVs2017%
cmake -G "Visual Studio 15 2017" ../source
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
pause
GOTO end
:VarIsNotSet
@echo Variable DevLibsVs2017 is not set!
pause
:end