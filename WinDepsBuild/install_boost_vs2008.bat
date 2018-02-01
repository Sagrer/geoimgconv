@rem Скрипт для установки boost, собранного под 2008 студию.
@rem В переменной окружения DevLibsVs2008 должен быть указан путь к каталогу, в который производится установка.
@echo off

@rem Ниже НУЖНО проверить и при необходимости исправить имя директории с исходником библиотеки.
set SRCDIR=boost

@rem Проверим наличие необходимой переменной окружения
IF "%DevLibsVs2008%"=="" GOTO VarIsNotSet

@rem Можно начинать.
@echo This script will install boost for vs2008 to directory:
@echo %DevLibsVs2008%
@echo.
SET /P "INPUT=Continue? (y/n)> "
If /I "%INPUT%"=="y" goto yes 
If /I "%INPUT%"=="n" goto end
:yes
cd %SRCDIR%
mkdir %DevLibsVs2008%\include\boost
mkdir %DevLibsVs2008%\lib
xcopy .\boost %DevLibsVs2008%\include\boost /E /Y
xcopy .\build_vs2008\stage\lib %DevLibsVs2008%\lib /E /Y
@echo.
@echo BOOST was successfully installed!
@echo.
pause
GOTO end
:VarIsNotSet
@echo Variable DevLibsVs2008 is not set!
@echo.
pause
:end