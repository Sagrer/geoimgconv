@rem Скрипт для сборки ICU под 2015 студию.
@echo off
@rem Ниже НУЖНО проверить и при необходимости исправить имя директории с исходником библиотеки.
set SRCDIR=icu

@rem Собсно скрипт
cd %SRCDIR%

@rem Настройка окружения
@echo Preparing environment...
set PATH=%VS140COMNTOOLS%..\..\VC\bin;%PATH%
call vcvars32.bat

@echo Upgrading solutions if needed. This may take a long time...
"%VS140COMNTOOLS%\..\IDE\devenv.com" .\source\allinone\allinone.sln /Upgrade
@echo.

@echo Cleaning...
@rem Вариант очистки в лоб
@rem rmdir /S /Q .\include
@rem rmdir /S /Q .\lib
@rem rmdir /S /Q .\bin
@rem Вариант очистки "по-правильному" через студию
"%VS140COMNTOOLS%\..\IDE\devenv.com" .\source\allinone\allinone.sln /Clean "Debug|Win32"
@echo.
"%VS140COMNTOOLS%\..\IDE\devenv.com" .\source\allinone\allinone.sln /Clean "Release|Win32"
@echo.

@echo Building Debug...
@rem Собираем Debug-версию
"%VS140COMNTOOLS%\..\IDE\devenv.com" .\source\allinone\allinone.sln /Build "Debug|Win32"
@echo.
@rem release-версия
@echo Building Release...
"%VS140COMNTOOLS%\..\IDE\devenv.com" .\source\allinone\allinone.sln /Build "Release|Win32"
@echo.

@rem Проверяем размер файла с данными.
@rem Если icudt меньше 10 килобайт - это точно заглушка!
set checkfile=".\bin\icudt*.dll"
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
"%VS140COMNTOOLS%\..\IDE\devenv.com" .\source\allinone\allinone.sln /Rebuild "Release|Win32" /Project makedata
@echo.
:end
@echo.
@echo Build finished!
@echo.
pause