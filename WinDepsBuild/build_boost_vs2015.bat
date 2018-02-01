@rem Скрипт для сборки BOOST под 2015 студию.
@rem В переменной окружения DevLibsVs2015 должен быть указан путь к каталогу c dev-версиями необходимых библиотек!
@echo off
@rem Ниже НУЖНО проверить и при необходимости исправить имя директории с исходником библиотеки.
set SRCDIR=boost

@rem Собсно скрипт

@rem Проверим наличие необходимой переменной окружения
IF "%DevLibsVs2015%"=="" GOTO VarIsNotSet

@rem Настройка окружения
@echo Preparing environment...
set PATH=%VS140COMNTOOLS%..\..\VC\bin;%PATH%
call vcvars32.bat
@echo.

@rem Можно начинать.
cd %SRCDIR%

@rem Собираем средства сборки boost.
@echo Building b2 build tool...
call .\bootstrap.bat
@echo.

@rem Собственно, собираем boost.
@echo Building boost...
.\b2 --build-dir=build_vs2015 --build-type=complete stage --stagedir=build_vs2015\stage -sICU_PATH=%DevLibsVs2015% --abbreviate-paths toolset=msvc-14.0 boost.locale.icu=on threading=multi architecture=x86 address-model=32
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
GOTO end
:VarIsNotSet
@echo Variable DevLibsVs2015 is not set!
@pause
:end