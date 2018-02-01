@rem Скрипт для установки gdal, собранного под 2015 студию.
@rem В переменной окружения DevLibsVs2015 должен быть указан путь к каталогу, в который производится установка.
@echo off

@rem Ниже НУЖНО проверить и при необходимости исправить имя директории с исходником библиотеки.
set SRCDIR=gdal

@rem Проверим наличие необходимой переменной окружения
IF "%DevLibsVs2015%"=="" GOTO VarIsNotSet

@rem Можно начинать.
@echo This script will install GDAL for vs2015 to directory:
@echo %DevLibsVs2015%
@echo.
SET /P "INPUT=Continue? (y/n)> "
If /I "%INPUT%"=="y" goto yes 
If /I "%INPUT%"=="n" goto end
:yes
set GDAL_HOME=%~dp0%srcdir%\vs2015
cd %SRCDIR%
mkdir %DevLibsVs2015%\include\gdal
mkdir %DevLibsVs2015%\lib
mkdir %DevLibsVs2015%\bin
xcopy %GDAL_HOME%\include %DevLibsVs2015%\include\gdal /E /Y
xcopy %GDAL_HOME%\lib %DevLibsVs2015%\lib /E /Y
xcopy %GDAL_HOME%\bin %DevLibsVs2015%\bin /E /Y
@echo.
@echo GDAL was successfully installed!
@echo.
pause
GOTO end
:VarIsNotSet
@echo Variable DevLibsVs2015 is not set!
@echo.
pause
:end