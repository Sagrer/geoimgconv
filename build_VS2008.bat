@rem Скрипт для сборки через VisualStudio 2008.
@echo off
@rem Проверим наличие необходимой переменной окружения
IF "%DevLibsVs2008%"=="" GOTO VarIsNotSet
@rem Сборка не запускается, т.к. у бесплатной 2008 студии нет консольного сборщика.
@echo.
@echo Preparing and cleaning project directory...
@echo.
mkdir vs2008_build\Debug
mkdir vs2008_build\Release
cd vs2008_build
@rem dll-ки :(
del /Q Debug\*.dll
del /Q Release\*.dll
copy /Y %DevLibsVs2008%\lib\boost_locale*.dll Debug\
copy /Y %DevLibsVs2008%\lib\boost_system*.dll Debug\
copy /Y %DevLibsVs2008%\lib\boost_chrono*.dll Debug\
copy /Y %DevLibsVs2008%\lib\boost_thread*.dll Debug\
copy /Y %DevLibsVs2008%\lib\boost_filesystem*.dll Debug\
copy /Y %DevLibsVs2008%\lib\boost_date_time*.dll Debug\
copy /Y %DevLibsVs2008%\lib\boost_program_options*.dll Debug\
copy /Y %DevLibsVs2008%\bin\icuuc*.dll Debug\
copy /Y %DevLibsVs2008%\bin\icudt*.dll Debug\
copy /Y %DevLibsVs2008%\bin\icuin*.dll Debug\
copy /Y %DevLibsVs2008%\bin\gdal*.dll Debug\
copy /Y Debug\*.dll Release\
@rem собсно собираем.
@echo.
@echo Configuring...
@echo.
SET CMAKE_PREFIX_PATH=%DevLibsVs2008%
cmake -G "Visual Studio 9 2008" ../source
if NOT %ERRORLEVEL% == 0 (
  echo FAILED!!!
  pause
  exit /B 1
)
@echo.
@echo To build binaries - go to vs2008_build directory and make *.sln file with Visual Studio!
@echo.
pause
GOTO end
:VarIsNotSet
@echo Variable DevLibsVs2008 is not set!
pause
:end