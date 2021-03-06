@rem Скрипт для сборки gdal под 2015 студию.
@rem В переменной окружения DevLibsVs2015 должен быть указан путь к каталогу c dev-версиями необходимых библиотек!
@echo off
@rem Ниже НУЖНО проверить и при необходимости исправить имя директории с исходником библиотеки.
set SRCDIR=gdal

@rem Собсно скрипт

@rem Проверим наличие необходимой переменной окружения
IF "%DevLibsVs2015%"=="" GOTO VarIsNotSet

@rem Настройка окружения
@echo Preparing environment...
set PATH=%VS140COMNTOOLS%..\..\VC\bin;%PATH%
set GDAL_HOME=%~dp0%srcdir%\vs2015
call vcvars32.bat
@echo.

@rem Можно начинать.
cd %SRCDIR%

@rem Очистка результатов возможной предыдущей сборки.
@echo Cleaning old info...
@echo.
rmdir /S /Q vs2015
mkdir vs2015
nmake -f makefile.vc MSVC_VER=1900 clean

@rem Собираем...
@echo.
@echo Building GDAL...
@echo.
nmake -f makefile.vc MSVC_VER=1900
if NOT %ERRORLEVEL% == 0 (
  echo FAILED!!!
  pause
  GOTO end
)
@rem Подготовка файлов для последующей инсталляции.
nmake -f makefile.vc MSVC_VER=1900 devinstall
if NOT %ERRORLEVEL% == 0 (
  echo FAILED!!!
  pause
  GOTO end
)
@rem Готово.
@echo.
@echo Build finished!
@echo.
pause
GOTO end
:VarIsNotSet
@echo Variable DevLibsVs2015 is not set!
pause
:end