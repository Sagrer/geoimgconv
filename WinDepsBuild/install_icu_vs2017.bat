@rem Скрипт установки ICU под 2017 студию.
@rem В переменной окружения DevLibsVs2017 должен быть указан путь к каталогу, в который производится установка.
@echo off

@rem Ниже НУЖНО проверить и при необходимости исправить имя директории с исходником библиотеки.
set SRCDIR=icu

@rem Проверим наличие необходимой переменной окружения
IF "%DevLibsVs2017%"=="" GOTO VarIsNotSet

@rem Можно начинать.
@echo This script will install ICU for vs2017 to directory:
@echo %DevLibsVs2017%
@echo.
SET /P "INPUT=Continue? (y/n)> "
If /I "%INPUT%"=="y" goto yes 
If /I "%INPUT%"=="n" goto end
:yes
cd %SRCDIR%
mkdir %DevLibsVs2017%\include
mkdir %DevLibsVs2017%\lib
mkdir %DevLibsVs2017%\bin
xcopy .\include %DevLibsVs2017%\include /E /Y
xcopy .\lib %DevLibsVs2017%\lib /E /Y
xcopy .\bin %DevLibsVs2017%\bin /E /Y
@echo.
@echo ICU was successfully installed!
@rem Проверяем размер файла с данными.
@rem Если icudt меньше 10 килобайт - это точно заглушка!
set checkfile=".\bin\icudt*.dll"
set maxbytesize=10000
if exist %checkfile% (
    goto check
) else (
    goto stubfile
)
:check
FOR /F "usebackq" %%A IN ('%checkfile%') DO set size=%%~zA
if %size% LSS %maxbytesize% (
    goto stubfile
) ELSE (
    goto topause
)
:stubfile
@rem Файл с данными или не существует или он заглушка. Предупредим юзверя.
@echo.
@echo WARNING! icudt*.dll is too small! It seems to be a stub! Rebuild it and reinstall this library!
@echo.
:topause
@pause
GOTO end
:VarIsNotSet
@echo Variable DevLibsVs2017 is not set!
@echo.
@pause
:end
