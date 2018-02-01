@rem Скрипт для установки gdal, собранного под 2017 студию.
@rem В переменной окружения DevLibsVs2017 должен быть указан путь к каталогу, в который производится установка.
@echo off

@rem Ниже НУЖНО проверить и при необходимости исправить имя директории с исходником библиотеки.
set SRCDIR=gdal

@rem Проверим наличие необходимой переменной окружения
IF "%DevLibsVs2017%"=="" GOTO VarIsNotSet

@rem Можно начинать.
@echo This script will install GDAL for vs2017 to directory:
@echo %DevLibsVs2017%
@echo.
SET /P "INPUT=Continue? (y/n)> "
If /I "%INPUT%"=="y" goto yes 
If /I "%INPUT%"=="n" goto end
:yes
set GDAL_HOME=%~dp0%srcdir%\vs2017
cd %SRCDIR%
mkdir %DevLibsVs2017%\include\gdal
mkdir %DevLibsVs2017%\lib
mkdir %DevLibsVs2017%\bin
xcopy %GDAL_HOME%\include %DevLibsVs2017%\include\gdal /E /Y
xcopy %GDAL_HOME%\lib %DevLibsVs2017%\lib /E /Y
xcopy %GDAL_HOME%\bin %DevLibsVs2017%\bin /E /Y
@echo.
@echo GDAL was successfully installed!
@echo.
pause
GOTO end
:VarIsNotSet
@echo Variable DevLibsVs2017 is not set!
@echo.
pause
:end