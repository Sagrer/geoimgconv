@rem Скрипт для сборки iconv под 64-битный mingw. Должен быть установлен msys2
@echo off
@rem Ниже НУЖНО проверить и при необходимости исправить имя директории с исходником библиотеки.
set SRCDIR=libiconv
@rem Количество ядер процессора - можно поставить больше единицы для ускорения сборки.
set CPU_NUM=2
@rem Использовать ли очистку перед сборкой (на случай если что-то в папке с исходниками уже собиралось)
set DO_CLEAN=0

@rem Собсно скрипт

@rem Настройка окружения
@echo Preparing environment...
IF "%msysdir%"=="" (
	@echo.
    @echo Variable msysdir is not set!
    @echo.
	pause
	exit /b 1
)
set PATH=%msysdir%\MinGW64\bin;%msysdir%\usr\bin;%PATH%
set MSYSTEM=MINGW64
@rem Проверяем что sed установлен и работает.
sed --version > nul
if NOT %ERRORLEVEL% == 0 (
  @echo.
  @echo MSYS2 sed is not installed! Call pacman -S sed in msys2 console.
  @echo.
  pause
  exit /b 1
)
@rem В путях на которые будут записываться файлы релизной и отладочной сборок - надо поменять слеш на юниксовый, иначе всё ломается.
set RELEASEDIR=%~dp0%SRCDIR%\dist_release64
set RELEASEDIR=%RELEASEDIR:\=/%
set RELEASEDIR=/%RELEASEDIR::=%
echo RELEASEDIR=%RELEASEDIR%
@echo.
@echo Configuring release version...
@echo.
cd "%SRCDIR%"
%msysdir%\usr\bin\bash.exe configure --prefix=%RELEASEDIR% --disable-debug --enable-release CFLAGS="-O2"
if NOT %ERRORLEVEL% == 0 (
  @echo.
  echo FAILED!
  @echo.
  pause
  exit /b 1
)
if %DO_CLEAN% == 0 goto SKIPCLEAN
	@echo .
	@echo Cleaning release version...
	@echo .
	make clean
	if NOT %ERRORLEVEL% == 0 (
		@echo.
		echo FAILED!
		@echo.
		pause
		exit /b 1
	)
:SKIPCLEAN
@echo .
@echo Building release version...
@echo .
make -j%CPU_NUM%
if NOT %ERRORLEVEL% == 0 (
  @echo.
  echo FAILED!
  @echo.
  pause
  exit /b 1
)
@echo .
@echo Installing release version to temp location...
@echo .
make install
if NOT %ERRORLEVEL% == 0 (
  @echo.
  echo FAILED!
  @echo.
  pause
  exit /b 1
)

@rem debug-версия не собирается.

cd ..\..
@echo.
@echo Build finished!
@echo.

pause