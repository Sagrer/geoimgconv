@rem Скрипт для установки 64-битной версии boost, собранной под 2015 студию.
@rem В переменной окружения DevLibsVs2015_x64 должен быть указан путь к каталогу, в который производится установка.
@echo off

@rem Ниже НУЖНО проверить и при необходимости исправить имя директории с исходником библиотеки.
set SRCDIR=boost

@rem Проверим наличие необходимой переменной окружения
IF "%DevLibsVs2015_x64%"=="" GOTO VarIsNotSet

@rem Можно начинать.
@echo This script will install boost for vs2015_x64 to directory:
@echo %DevLibsVs2015_x64%
@echo.
SET /P "INPUT=Continue? (y/n)> "
If /I "%INPUT%"=="y" goto yes 
If /I "%INPUT%"=="n" goto end
:yes
cd %SRCDIR%
mkdir %DevLibsVs2015_x64%\include\boost
mkdir %DevLibsVs2015_x64%\lib
mkdir %DevLibsVs2015_x64%\lib64
xcopy .\boost %DevLibsVs2015_x64%\include\boost /E /Y
xcopy .\build_vs2015_x64\stage\lib %DevLibsVs2015_x64%\lib /E /Y
@rem Системы сборки могут искать библиотеки и по путям lib64, bin64. Проверим работает ли линковка.
del /Q %DevLibsVs2015_x64%\testfile1
del /Q %DevLibsVs2015_x64%\testfile2
echo. 2>%DevLibsVs2015_x64%\testfile1
mklink /H %DevLibsVs2015_x64%\testfile2 %DevLibsVs2015_x64%\testfile1
if exist %DevLibsVs2015_x64%\testfile2 (
	@rem Линковка работает. Линкуем все файлы, имена которых встретим в исходных каталогах.
	for /r %%i in (.\build_vs2015_x64\stage\lib\*) do (
		mklink /H %DevLibsVs2015_x64%\lib64\%%~nxi %DevLibsVs2015_x64%\lib\%%~nxi
	)
) ELSE (
	@rem Если линковка не работает по любой причине - просто копируем.
	xcopy .\build_vs2015_x64\stage\lib %DevLibsVs2015_x64%\lib64 /E /Y
)
@rem В любом случае надо удалить тестовые файлы.
del /Q %DevLibsVs2015_x64%\testfile1
del /Q %DevLibsVs2015_x64%\testfile2
@echo.
@echo BOOST was successfully installed!
@echo.
pause
GOTO end
:VarIsNotSet
@echo Variable DevLibsVs2015_x64 is not set!
@echo.
pause
:end