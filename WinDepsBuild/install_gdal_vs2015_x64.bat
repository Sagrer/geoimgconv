@rem Скрипт для установки 64-битной версии gdal, собранной под 2015 студию.
@rem В переменной окружения DevLibsVs2015_x64 должен быть указан путь к каталогу, в который производится установка.
@echo off

@rem Ниже НУЖНО проверить и при необходимости исправить имя директории с исходником библиотеки.
set SRCDIR=gdal

@rem Проверим наличие необходимой переменной окружения
IF "%DevLibsVs2015_x64%"=="" GOTO VarIsNotSet

@rem Можно начинать.
@echo This script will install GDAL for vs2015_x64 to directory:
@echo %DevLibsVs2015_x64%
@echo.
SET /P "INPUT=Continue? (y/n)> "
If /I "%INPUT%"=="y" goto yes 
If /I "%INPUT%"=="n" goto end
:yes
set GDAL_HOME=%~dp0%srcdir%\vs2015_x64
cd %SRCDIR%
mkdir %DevLibsVs2015_x64%\include\gdal
mkdir %DevLibsVs2015_x64%\lib
mkdir %DevLibsVs2015_x64%\bin
mkdir %DevLibsVs2015_x64%\lib64
mkdir %DevLibsVs2015_x64%\bin64
xcopy %GDAL_HOME%\include %DevLibsVs2015_x64%\include\gdal /E /Y
xcopy %GDAL_HOME%\lib %DevLibsVs2015_x64%\lib /E /Y
xcopy %GDAL_HOME%\bin %DevLibsVs2015_x64%\bin /E /Y
@rem Системы сборки могут искать библиотеки и по путям lib64, bin64. Проверим работает ли линковка.
del /Q %DevLibsVs2015_x64%\testfile1
del /Q %DevLibsVs2015_x64%\testfile2
echo. 2>%DevLibsVs2015_x64%\testfile1
mklink /H %DevLibsVs2015_x64%\testfile2 %DevLibsVs2015_x64%\testfile1
if exist %DevLibsVs2015_x64%\testfile2 (
	@rem Линковка работает. Линкуем все файлы, имена которых встретим в исходных каталогах.
	for /r "%GDAL_HOME%\lib" %%i in (*) do (
		mklink /H %DevLibsVs2015_x64%\lib64\%%~nxi %DevLibsVs2015_x64%\lib\%%~nxi
	)
	for /r "%GDAL_HOME%\bin" %%i in (*) do (
		mklink /H %DevLibsVs2015_x64%\bin64\%%~nxi %DevLibsVs2015_x64%\bin\%%~nxi
	)
) ELSE (
	@rem Если линковка не работает по любой причине - просто копируем.
	xcopy %GDAL_HOME%\lib %DevLibsVs2015_x64%\lib64 /E /Y
	xcopy %GDAL_HOME%\bin %DevLibsVs2015_x64%\bin64 /E /Y
)
@rem В любом случае надо удалить тестовые файлы.
del /Q %DevLibsVs2015_x64%\testfile1
del /Q %DevLibsVs2015_x64%\testfile2
@echo.
@echo GDAL was successfully installed!
@echo.
pause
GOTO end
:VarIsNotSet
@echo Variable DevLibsVs2015_x64 is not set!
@echo.
pause
:end