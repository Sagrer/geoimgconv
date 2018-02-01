@rem Скрипт установки 64-битной версии ICU, собранной под 2017 студию.
@rem В переменной окружения DevLibsVs2017_x64 должен быть указан путь к каталогу, в который производится установка.
@echo off

@rem Ниже НУЖНО проверить и при необходимости исправить имя директории с исходником библиотеки.
set SRCDIR=icu

@rem Проверим наличие необходимой переменной окружения
IF "%DevLibsVs2017_x64%"=="" GOTO VarIsNotSet

@rem Можно начинать.
@echo This script will install ICU for vs2017_x64 to directory:
@echo %DevLibsVs2017_x64%
@echo.
SET /P "INPUT=Continue? (y/n)> "
If /I "%INPUT%"=="y" goto yes 
If /I "%INPUT%"=="n" goto end
:yes
cd %SRCDIR%
mkdir %DevLibsVs2017_x64%\include
mkdir %DevLibsVs2017_x64%\lib
mkdir %DevLibsVs2017_x64%\bin
mkdir %DevLibsVs2017_x64%\lib64
mkdir %DevLibsVs2017_x64%\bin64
xcopy .\include %DevLibsVs2017_x64%\include /E /Y
xcopy .\lib64 %DevLibsVs2017_x64%\lib /E /Y
xcopy .\bin64 %DevLibsVs2017_x64%\bin /E /Y
@rem Системы сборки могут искать библиотеки и по путям lib64, bin64. Проверим работает ли линковка.
del /Q %DevLibsVs2017_x64%\testfile1
del /Q %DevLibsVs2017_x64%\testfile2
echo. 2>%DevLibsVs2017_x64%\testfile1
mklink /H %DevLibsVs2017_x64%\testfile2 %DevLibsVs2017_x64%\testfile1
if exist %DevLibsVs2017_x64%\testfile2 (
	@rem Линковка работает. Линкуем все файлы, имена которых встретим в исходных каталогах.
	for /r %%i in (.\lib64\*) do (
		mklink /H %DevLibsVs2017_x64%\lib64\%%~nxi %DevLibsVs2017_x64%\lib\%%~nxi
	)
	for /r %%i in (.\bin64\*) do (
		mklink /H %DevLibsVs2017_x64%\bin64\%%~nxi %DevLibsVs2017_x64%\bin\%%~nxi
	)
) ELSE (
	@rem Если линковка не работает по любой причине - просто копируем.
	xcopy .\lib64 %DevLibsVs2017_x64%\lib64 /E /Y
	xcopy .\bin64 %DevLibsVs2017_x64%\bin64 /E /Y
)
@rem В любом случае надо удалить тестовые файлы.
del /Q %DevLibsVs2017_x64%\testfile1
del /Q %DevLibsVs2017_x64%\testfile2
@echo.
@echo ICU was successfully installed!
@rem Проверяем размер файла с данными.
@rem Если icudt меньше 10 килобайт - это точно заглушка!
set checkfile=".\bin64\icudt*.dll"
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
@echo Variable DevLibsVs2017_x64 is not set!
@echo.
@pause
:end
