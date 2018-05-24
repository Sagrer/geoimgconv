@rem Скрипт установки iconv под mingw64.
@rem В переменной окружения DevLibsMingw64 должен быть указан путь к каталогу, в который производится установка.
@echo off

@rem Ниже НУЖНО проверить и при необходимости исправить имя директории с исходником библиотеки.
set SRCDIR=libiconv

@rem Проверим наличие необходимой переменной окружения
IF "%DevLibsMingw64%"=="" (
	@echo.
    @echo Variable DevLibsMingw64 is not set!
    @echo.
	pause
	exit /b 1
)

@rem Можно начинать.
@echo This script will install iconv for Mingw64 to directory:
@echo %DevLibsMingw64%
@echo.
SET /P "INPUT=Continue? (y/n)> "
If /I "%INPUT%"=="y" goto yes 
If /I "%INPUT%"=="n" goto end
:yes
cd "%SRCDIR%\dist_release64"
mkdir %DevLibsMingw64%\include
mkdir %DevLibsMingw64%\lib
mkdir %DevLibsMingw64%\bin
mkdir %DevLibsMingw64%\share
xcopy .\include %DevLibsMingw64%\include /E /Y
xcopy .\lib %DevLibsMingw64%\lib /E /Y
xcopy .\bin %DevLibsMingw64%\bin /E /Y
xcopy .\share %DevLibsMingw64%\share /E /Y
@echo.
@echo iconv was successfully installed!
@echo.
pause
:end
