@rem Скрипт установки iconv под mingw32.
@rem В переменной окружения DevLibsMingw32 должен быть указан путь к каталогу, в который производится установка.
@echo off

@rem Ниже НУЖНО проверить и при необходимости исправить имя директории с исходником библиотеки.
set SRCDIR=libiconv

@rem Проверим наличие необходимой переменной окружения
IF "%DevLibsMingw32%"=="" (
	@echo.
    @echo Variable DevLibsMingw32 is not set!
    @echo.
	pause
	exit /b 1
)

@rem Можно начинать.
@echo This script will install iconv for Mingw32 to directory:
@echo %DevLibsMingw32%
@echo.
SET /P "INPUT=Continue? (y/n)> "
If /I "%INPUT%"=="y" goto yes 
If /I "%INPUT%"=="n" goto end
:yes
cd "%SRCDIR%\dist_release32"
mkdir %DevLibsMingw32%\include
mkdir %DevLibsMingw32%\lib
mkdir %DevLibsMingw32%\bin
mkdir %DevLibsMingw32%\share
xcopy .\include %DevLibsMingw32%\include /E /Y
xcopy .\lib %DevLibsMingw32%\lib /E /Y
xcopy .\bin %DevLibsMingw32%\bin /E /Y
xcopy .\share %DevLibsMingw32%\share /E /Y
@echo.
@echo iconv was successfully installed!
@echo.
pause
:end
