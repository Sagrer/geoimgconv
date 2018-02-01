@rem Скрипт-детектор 2017 студии через программу vswhere. Всё потому что мелкие и мягкие решили
@rem что узнавать путь к файлам студии через переменную окружения - для слабаков. Получать такую
@rem информацию путём обращения к стильному, модному и молодёжному, основанному на .NET специально
@rem обученному API - вот выбор... этих странных и нехороших личностей :(. Посему отныне инсталлятор
@rem 2017 студии устанавливает в относительно постоянное место программку vswhere.exe, которую
@rem можно найти, что-нибудь у неё запросить, а потом выковырять из результатов запроса то что нужно.

@rem В прочем, всегда можно ручками проставить в переменную окружения VS150COMNTOOLS нужный путь - тогда
@rem скрипт будет делать строго ничего.

@echo off
if not "%VS150COMNTOOLS%"=="" (
	goto exit
)
rem Переменная не установлена. Надо искать с помощью vswhere. Проверим 2 возможных
rem места стандартной установки.
if NOT "%ProgramFiles(x86)%"=="" (
	set VSWHERE="%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
) else (
	set VSWHERE="%ProgramFiles%\Microsoft Visual Studio\Installer\vswhere.exe"
)
if exist %VSWHERE% (
	for /f "usebackq tokens=1* delims=: " %%i in (`%VSWHERE% -latest -requires Microsoft.VisualStudio.Workload.NativeDesktop`) do (
		if /i "%%i"=="installationPath" set VS150COMNTOOLS=%%j)
)
if exist %VSWHERE% (
	rem Вот это в отдельном IF т.к. повторное вычисление переменных если они изменились
	rem внутри IF происходит только за пределами этого IF
	set VS150COMNTOOLS="%VS150COMNTOOLS%\Common7\Tools\"
)
rem Если удалось что-то найти 
if not %VS150COMNTOOLS%=="" (
	goto exit
)
rem TODO
rem Теоретически сюда можно ещё вписать попытку поиска студии в стандартных местах.
if NOT "%ProgramFiles(x86)%"=="" (
	set DETECTOR_PF=%ProgramFiles(x86)%
) else (
	set DETECTOR_PF=%ProgramFiles%
)
set DETECTOR_CHECK=%DETECTOR_PF%\Microsoft Visual Studio\2017\Community\Common7\Tools\makehm.exe
if exist %DETECTOR_CHECK% (
	set VS150COMNTOOLS=%DETECTOR_PF%\Microsoft Visual Studio\2017\Community\Common7\Tools\
)
:exit
if %VS150COMNTOOLS%=="" (
	rem что-то пошло не так.
	exit /b 1
)
rem Уберём кавычки из переменной
set VS150COMNTOOLS=%VS150COMNTOOLS:"=%
rem Вывод и ожидание для отладки:
rem echo %VS150COMNTOOLS%
rem pause