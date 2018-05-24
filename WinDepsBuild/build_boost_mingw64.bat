@rem Скрипт для сборки BOOST под mingw64.
@rem В переменной окружения msysdir должен быть указан путь к каталогу c установленным msys2.
@rem В самом msys должен быть установлен 64-битный mingw.
@rem помимо этого в папке с библиотеками уже должна лежать собранная версия ICU.
@echo off
@rem Ниже НУЖНО проверить и при необходимости исправить имя директории с исходником библиотеки.
set SRCDIR=boost
@rem Количество ядер процессора - можно поставить больше единицы для ускорения сборки.
set CPU_NUM=2

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
IF "%DevLibsMingw64%"=="" (
	@echo.
    @echo Variable DevLibsMingw64 is not set!
    @echo.
	pause
	exit /b 1
)
set PATH=%msysdir%\MinGW64\bin;%DevLibsMingw64%\bin;%DevLibsMingw64%\lib;%PATH%
set MSYSTEM=MINGW64
@echo.

@rem Можно начинать.
cd %SRCDIR%

@rem Собираем средства сборки boost.
@echo Building b2 build tool...
call .\bootstrap.bat
@echo.

@rem Собственно, собираем boost.
@echo Building boost...
@rem Версия с ICU
@rem .\b2 --build-dir=build_mingw64 --layout=tagged --build-type=complete stage --stagedir=build_mingw64\stage -j%CPU_NUM% -sICU_PATH=%DevLibsMingw64% -sICU_LINK="-L%DevLibsMingw64%\lib -licuuc -licuin -licudt" --abbreviate-paths toolset=gcc boost.locale.icu=on threading=multi architecture=x86 address-model=64
@rem Версия с iconv
.\b2 --build-dir=build_mingw64 --layout=tagged --build-type=complete stage --stagedir=build_mingw64\stage -j%CPU_NUM% --abbreviate-paths toolset=gcc boost.locale.icu=off boost.locale.iconv=on -sICONV_PATH=%DevLibsMingw64% threading=multi architecture=x86 address-model=64
@echo.
if NOT %ERRORLEVEL% == 0 (
  echo FAILED!!!
  pause
  GOTO end
)
@echo.
@echo Build finished!
@echo.
pause