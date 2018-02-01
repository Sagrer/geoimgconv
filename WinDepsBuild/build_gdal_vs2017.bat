@rem Скрипт для сборки gdal под 2017 студию.
@rem В переменной окружения DevLibsVs2017 должен быть указан путь к каталогу c dev-версиями необходимых библиотек!
@echo off
@rem Ниже НУЖНО проверить и при необходимости исправить имя директории с исходником библиотеки.
set SRCDIR=gdal

@rem Собсно скрипт

@rem Проверим наличие необходимой переменной окружения
IF "%DevLibsVs2017%"=="" GOTO VarIsNotSet

@rem Настройка окружения
@echo Preparing environment...
call detect_vs2017.bat
if NOT %ERRORLEVEL% == 0 (
  echo Visual studio 2017 was not detected! Please install latest update or set VS150COMNTOOLS env variable to Common7\Tools directory of VS2017 install directory.
  pause
  exit /b 1
)
rem Старая механика с подсовыванием path не работает. А vcvars32 теперь меняет текущий каталог.
rem Надо запомнить где мы есть и потом вернуться обратно, чего бы там не хотели мелкомягкие.
set SAVEDPATH=%CD%
call "%VS150COMNTOOLS%..\..\VC\Auxiliary\Build\vcvars32.bat"
cd /D %SAVEDPATH%
set GDAL_HOME=%~dp0%srcdir%\vs2017
@echo.

@rem Можно начинать.
cd %SRCDIR%

@rem Очистка результатов возможной предыдущей сборки.
@echo Cleaning old info...
@echo.
rmdir /S /Q vs2017
mkdir vs2017
nmake -f makefile.vc MSVC_VER=1910 clean

@rem Собираем...
@echo.
@echo Building GDAL...
@echo.
nmake -f makefile.vc MSVC_VER=1910
if NOT %ERRORLEVEL% == 0 (
  echo FAILED!!!
  pause
  GOTO end
)
@rem Подготовка файлов для последующей инсталляции.
nmake -f makefile.vc MSVC_VER=1910 devinstall
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
@echo Variable DevLibsVs2017 is not set!
pause
:end