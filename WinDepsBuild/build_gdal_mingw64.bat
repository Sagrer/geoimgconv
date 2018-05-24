@rem Скрипт для сборки GDAL под 64-битный mingw. Должен быть установлен msys2
@echo off
@rem Ниже НУЖНО проверить и при необходимости исправить имя директории с исходником библиотеки.
set SRCDIR=gdal
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
@rem Проверяем что aclocal установлен и работает.
%msysdir%\usr\bin\bash.exe aclocal --version > nul
if NOT %ERRORLEVEL% == 0 (
  @echo.
  @echo MSYS2 aclocal is not installed! Call pacman -S base-devel in msys2 console.
  @echo.
  pause
  exit /b 1
)
@rem В путях на которые будут записываться файлы релизной и отладочной сборок - надо поменять слеш на юниксовый, иначе всё ломается.
set RELEASEDIR=%~dp0%SRCDIR%\dist_release64
set RELEASEDIR=%RELEASEDIR:\=/%
set RELEASEDIR=/%RELEASEDIR::=%
echo RELEASEDIR=%RELEASEDIR%
set DEBUGDIR=%~dp0%SRCDIR%\dist_debug64
set DEBUGDIR=%DEBUGDIR:\=/%
set DEBUGDIR=/%DEBUGDIR::=%
echo DEBUGDIR=%DEBUGDIR%
@rem префикс mingw
set mingw_prfx=/%msysdir:\=/%/mingw64
set mingw_prfx=%mingw_prfx::=%
cd "%SRCDIR%"
@echo.
@echo Configuring release version...
@echo.
@rem Процесс конфигурирования и навешивания патчей придётся запускать из bash-скрипта
%msysdir%\usr\bin\bash.exe ..\build_gdal_mingwX_patch_and_configure.sh
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

cd ..
@echo.
@echo Build finished!
@echo.

pause