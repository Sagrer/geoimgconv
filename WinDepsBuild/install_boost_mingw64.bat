@rem Скрипт для установки boost, собранного mingw64 msys2.
@rem В переменной окружения DevLibsMingw64 должен быть указан путь к каталогу, в который производится установка.
@echo off

@rem Ниже НУЖНО проверить и при необходимости исправить имя директории с исходником библиотеки.
set SRCDIR=boost

@rem Проверим наличие необходимой переменной окружения
IF "%DevLibsMingw64%"=="" (
	@echo.
    @echo Variable DevLibsMingw64 is not set!
    @echo.
	pause
	exit /b 1
)

@rem Можно начинать.
@echo This script will install boost for Mingw64 to directory:
@echo %DevLibsMingw64%
@echo.
SET /P "INPUT=Continue? (y/n)> "
If /I "%INPUT%"=="y" goto yes 
If /I "%INPUT%"=="n" goto end
:yes
cd %SRCDIR%
mkdir %DevLibsMingw64%\include\boost
mkdir %DevLibsMingw64%\lib
xcopy .\boost %DevLibsMingw64%\include\boost /E /Y
xcopy .\build_mingw64\stage\lib %DevLibsMingw64%\lib /E /Y
@echo.
@echo BOOST was successfully installed!
@echo.
pause
:end