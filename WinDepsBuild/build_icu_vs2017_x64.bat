@rem Скрипт для сборки 64-битной версии ICU под 2017 студию.
@echo off
@rem Ниже НУЖНО проверить и при необходимости исправить имя директории с исходником библиотеки.
set SRCDIR=icu

@rem Собсно скрипт

@rem Настройка окружения
@echo Preparing environment...
call detect_vs2017.bat
if NOT %ERRORLEVEL% == 0 (
  echo Visual studio 2017 was not detected! Please install latest update or set VS150COMNTOOLS env variable to Common7\Tools directory of VS2017 install directory.
  pause
  exit /b 1
)
rem Старая механика с подсовыванием path не работает. А vcvars64 теперь меняет текущий каталог.
rem Надо запомнить где мы есть и потом вернуться обратно, чего бы там не хотели мелкомягкие.
set SAVEDPATH=%CD%
call "%VS150COMNTOOLS%..\..\VC\Auxiliary\Build\vcvars64.bat"
cd /D %SAVEDPATH%
cd %SRCDIR%
@echo.

@echo Upgrading solutions if needed. This may take a long time...
"%VS150COMNTOOLS%\..\IDE\devenv.com" .\source\allinone\allinone.sln /Upgrade
@echo.

@echo Cleaning...
@rem Вариант очистки в лоб
@rem rmdir /S /Q .\include
@rem rmdir /S /Q .\lib
@rem rmdir /S /Q .\bin
@rem Вариант очистки "по-правильному" через студию
"%VS150COMNTOOLS%\..\IDE\devenv.com" .\source\allinone\allinone.sln /Clean "Debug|x64"
@echo.
"%VS150COMNTOOLS%\..\IDE\devenv.com" .\source\allinone\allinone.sln /Clean "Release|x64"
@echo.

@echo Building Debug...
@rem Собираем Debug-версию
"%VS150COMNTOOLS%\..\IDE\devenv.com" .\source\allinone\allinone.sln /Build "Debug|x64"
@echo.
@rem release-версия
@echo Building Release...
"%VS150COMNTOOLS%\..\IDE\devenv.com" .\source\allinone\allinone.sln /Build "Release|x64"
@echo.

@rem Проверяем размер файла с данными.
@rem Если icudt меньше 10 килобайт - это точно заглушка!
set checkfile=".\bin64\icudt*.dll"
set maxbytesize=10000
if exist %checkfile% (
    goto check
) else (
    goto build
)
:check
FOR /F "usebackq" %%A IN ('%checkfile%') DO set size=%%~zA
if %size% LSS %maxbytesize% (
    goto build
) ELSE (
    goto end
)
:build
@rem Файл с данными или не существует или он заглушка. Пересоберём отдельно проект с данными.
@echo.
@echo icudt*.dll is too small! It seems to be a stub! Rebuilding it!
@echo.
"%VS150COMNTOOLS%\..\IDE\devenv.com" .\source\allinone\allinone.sln /Rebuild "Release|x64" /Project makedata
@echo.
:end
@echo.
@echo Build finished!
@echo.
pause