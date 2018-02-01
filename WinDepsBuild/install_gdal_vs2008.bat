@rem Скрипт для установки gdal, собранного под 2008 студию.
@rem В переменной окружения DevLibsVs2008 должен быть указан путь к каталогу, в который производится установка.
@echo off

@rem Ниже НУЖНО проверить и при необходимости исправить имя директории с исходником библиотеки.
set SRCDIR=gdal

@rem Проверим наличие необходимой переменной окружения
IF "%DevLibsVs2008%"=="" GOTO VarIsNotSet

@rem Можно начинать.
@echo This script will install GDAL for vs2008 to directory:
@echo %DevLibsVs2008%
@echo.
SET /P "INPUT=Continue? (y/n)> "
If /I "%INPUT%"=="y" goto yes 
If /I "%INPUT%"=="n" goto end
:yes
set GDAL_HOME=%~dp0%srcdir%\vs2008
cd %SRCDIR%
mkdir %DevLibsVs2008%\include\gdal
mkdir %DevLibsVs2008%\lib
mkdir %DevLibsVs2008%\bin
xcopy %GDAL_HOME%\include %DevLibsVs2008%\include\gdal /E /Y
xcopy %GDAL_HOME%\lib %DevLibsVs2008%\lib /E /Y
xcopy %GDAL_HOME%\bin %DevLibsVs2008%\bin /E /Y
@echo.
@echo GDAL was successfully installed!
@echo.
pause
GOTO end
:VarIsNotSet
@echo Variable DevLibsVs2008 is not set!
@echo.
pause
:end