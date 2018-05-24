@rem Скрипт для установки boost, собранного mingw32 msys2.
@rem В переменной окружения DevLibsMingw32 должен быть указан путь к каталогу, в который производится установка.
@echo off

@rem Ниже НУЖНО проверить и при необходимости исправить имя директории с исходником библиотеки.
set SRCDIR=boost

@rem Проверим наличие необходимой переменной окружения
IF "%DevLibsMingw32%"=="" (
	@echo.
    @echo Variable DevLibsMingw32 is not set!
    @echo.
	pause
	exit /b 1
)

@rem Можно начинать.
@echo This script will install boost for Mingw32 to directory:
@echo %DevLibsMingw32%
@echo.
SET /P "INPUT=Continue? (y/n)> "
If /I "%INPUT%"=="y" goto yes 
If /I "%INPUT%"=="n" goto end
:yes
cd %SRCDIR%
mkdir %DevLibsMingw32%\include\boost
mkdir %DevLibsMingw32%\lib
xcopy .\boost %DevLibsMingw32%\include\boost /E /Y
xcopy .\build_mingw32\stage\lib %DevLibsMingw32%\lib /E /Y
@echo.
@echo BOOST was successfully installed!
@echo.
pause
:end