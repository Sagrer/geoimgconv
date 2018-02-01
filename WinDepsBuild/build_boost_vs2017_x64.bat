@rem Скрипт для сборки 64-битной версии BOOST под 2017 студию.
@rem В переменной окружения DevLibsVs2017_x64 должен быть указан путь к каталогу c dev-версиями необходимых библиотек!
@echo off
@rem Ниже НУЖНО проверить и при необходимости исправить имя директории с исходником библиотеки.
set SRCDIR=boost

@rem Собсно скрипт

@rem Проверим наличие необходимой переменной окружения
IF "%DevLibsVs2017_x64%"=="" GOTO VarIsNotSet

@rem Настройка окружения
@echo Preparing environment...
call detect_vs2017.bat
if NOT %ERRORLEVEL% == 0 (
  echo Visual studio 2017 was not detected! Please install latest update or set VS150COMNTOOLS env variable to Common7\Tools directory of VS2017 install directory.
  pause
  exit /b 1
)
rem Старая механика с подсовыванием path не работает. А vcvars64 теперь меняет текущий каталог.
rem Надо запомнить где мы есть и потом вернуться обратно, чего бы там не хотели мелкомягкие.
set SAVEDPATH=%CD%
call "%VS150COMNTOOLS%..\..\VC\Auxiliary\Build\vcvars64.bat"
cd /D %SAVEDPATH%
@echo.

@rem Можно начинать.
cd %SRCDIR%

@rem Собираем средства сборки boost.
@echo Building b2 build tool...
call .\bootstrap.bat
@echo.

@rem Собственно, собираем boost.
@echo Building boost...
@rem b2 возможно попытается найти скрипт vcvars, но не факт что найти - по тем путям где он ищет у меня
@rem скрипта нет. Но это неважно - скрипт мы уже запустили ручками ). Хотя, возможно то что мы пытались
@rem запустить - не сработало, а у b2 всё получится - это тоже вариант )).
.\b2 --build-dir=build_vs2017_x64 --build-type=complete stage --stagedir=build_vs2017_x64\stage -sICU_PATH=%DevLibsVs2017_x64% --abbreviate-paths toolset=msvc-14.1 boost.locale.icu=on threading=multi architecture=x86 address-model=64
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
@echo Variable DevLibsVs2017_x64 is not set!
@pause
:end