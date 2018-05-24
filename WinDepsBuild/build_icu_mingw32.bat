@rem Скрипт для сборки ICU под 32-битный mingw. Должен быть установлен msys2
@echo off
@rem Ниже НУЖНО проверить и при необходимости исправить имя директории с исходником библиотеки.
set SRCDIR=icu
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
set PATH=%msysdir%\MinGW32\bin;%msysdir%\usr\bin;%PATH%
set MSYSTEM=MINGW32
@rem Проверяем что patch установлен и работает.
patch -v
if NOT %ERRORLEVEL% == 0 (
  @echo.
  @echo MSYS2 patch is not installed! Please call pacman -Sy patch msys/patch from MSYS2 console!
  @echo.
  pause
  exit /b 1
)
@rem В путях на которые будут записываться файлы релизной и отладочной сборок - надо поменять слеш на юниксовый, иначе всё ломается.
set RELEASEDIR=%~dp0%SRCDIR%\dist_release32
set RELEASEDIR=%RELEASEDIR:\=/%
echo RELEASEDIR=%RELEASEDIR%
set DEBUGDIR=%~dp0%SRCDIR%\dist_debug32
set DEBUGDIR=%DEBUGDIR:\=/%
echo DEBUGDIR=%DEBUGDIR%
@echo.
@echo Applying patches...
@echo.
cd "%SRCDIR%"
patch -Nbp1 -i ../icu-Alexpux-workaround-missing-locale.patch
if %ERRORLEVEL% GEQ 2 (
  echo FAILED!
  pause
  exit /b 1
)
cd ..
@echo.
@echo Configuring release version...
@echo.
cd "%SRCDIR%\source"
%msysdir%\usr\bin\bash.exe runConfigureICU MinGW --prefix=%RELEASEDIR%
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
	mingw32-make clean
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
mingw32-make -j%CPU_NUM%
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
mingw32-make install
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