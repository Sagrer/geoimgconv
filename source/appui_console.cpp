///////////////////////////////////////////////////////////
//                                                       //
//                  GeoImageConverter                    //
//       Преобразователь изображений с геоданными        //
//    Copyright © 2017-2018 Александр (Sagrer) Гриднев   //
//              Распространяется на условиях             //
//                 GNU GPL v3 или выше                   //
//                  см. файл gpl.txt                     //
//                                                       //
///////////////////////////////////////////////////////////

//Место для списка авторов данного конкретного файла (если изменения
//вносили несколько человек).

////////////////////////////////////////////////////////////////////////

//Главный класс приложения. Сюда ведёт main(), отсюда программа и рулит всем.
//Здесь юзерский интерфейс и обращение к выполняющим реальную работу функциям.
//Желательно не выпускать исключения выше этого уровня.

//Пока чисто консолька, без curses.
#include <iostream>
#include <boost/filesystem.hpp>
#pragma warning(push)
#pragma warning(disable:4251)	//Потому что GDAL API использует в интерфейсе шаблонные классы STL и компилятор на это ругается.
#include <gdal_priv.h>
#pragma warning(pop)
#include "appui_console.h"
#include "median_filter.h"
#include "strings_tools_box.h"

namespace b_fs = boost::filesystem;
using namespace std;

namespace geoimgconv
{

//////////////////////////////
//    Класс AppUIConsole    //
//////////////////////////////

//--------------------------------//
//        Приватные методы        //
//--------------------------------//

//Задетектить какое максимальное количество памяти можно использовать исходя из
//характеристик компьютера и параметров, переданных в командной строке, а также вычислить
//количество блоков, которое используемый в данный момент фильтр может одновременно держать
//в памяти. Результат пишется в maxMemCanBeUsed_ (количество памяти в байтах) и в
//maxBlocksCanBeUsed_ (количество блоков). В случае ошибки туда запишется 0.
//filterObj - объект фильтра, который будет обрабатывать изображение.
//Если изображение не влезет в память вообще никак - вернёт false. При этом если
//изображение будет способно влезть в память только с попаданием части буфера в своп - метод
//самостоятельно спросит у юзверя что именно ему делать если swapMode позволяет.
//swapMode - задаёт либо интерактивный режим либо тихий режим работы, в тихом режиме swap может
//либо использоваться либо не использоваться в зависимости от выбранного режима.
//Перед запуском метода _должен_ был быть выполнен метод DetectSysResInfo()!
bool AppUIConsole::DetectMaxMemoryCanBeUsed(const BaseFilter &filterObj, const SwapMode swapMode,
	ErrorInfo *errObj)
{

	//TODO: монструозный метод! Надо или упростить логику или выделить часть кода во вспомогательные
	//методы.

	maxMemCanBeUsed_ = 0;
	maxBlocksCanBeUsed_ = 0;

	//С лимитом по адресному пространству - всё сложно. Для эффективной работы нужны непрерывные блоки
	//большого размера. Но по адресному пространству размазаны dll-ки что не позволяет иметь непрерывные
	//блоки больше примерно 900 мегабайт один блок и 400 мегабайт второй в 32-битной версии под вендами
	//при якобы 2-гиговом адресном пространстве. В прочем, я буду собирать программу с флагом
	// /LARGEADDRESSAWARE благодаря которому по крайней мере при запуске в 64-битной венде программа
	//увидит дополнительную пустую непрерывную область размером в 2 гига. Хотя запуск 32-битной версии
	//программы на 64-битной системе весьма бессмысленен, но юзеры - такие юзеры, всякое бывает.
	//Итак - если мы видим менее 2100 и более 900 мегабайт лимита по адресному пространству - лимит будет
	//установлен в 900 мегабайт. Если мы видим более 2100 и менее 4100 - лимит будет установлен в 2000
	//мегабайт. Такие дела.

	unsigned long long addrSizeLimit;
	//константы для размера в байтах:
	const unsigned long long size2100m = 2202009600;
	const unsigned long long size900m = 943718400;
	const unsigned long long size4100m = 4299161600;
	const unsigned long long size2000m = 2097152000;
	//принимаем решение:
	if ((sysResInfo_.maxProcessMemorySize < size2100m) && (sysResInfo_.maxProcessMemorySize > size900m))
	{
		addrSizeLimit = size900m;
	}
	else if ((sysResInfo_.maxProcessMemorySize < size4100m) && (sysResInfo_.maxProcessMemorySize > size2100m))
	{
		addrSizeLimit = size2000m;
	}
	else if (sysResInfo_.maxProcessMemorySize > size4100m)
	{
		//Раньше, до осознания проблемы фрагментации лимит выбирался как 90% от реального. Теперь
		//так делается только если его больше 4,1 гигабайт, т.е. это скорее всего какая-то 64-битная система.
		addrSizeLimit = 90 * (sysResInfo_.maxProcessMemorySize / 100);
	}
	else
	{
		//В крайнем случае - попробуем просто взять 40% от заявленного размера адресного пространства.
		addrSizeLimit = 40 * (sysResInfo_.maxProcessMemorySize / 100);
	}

	//Нельзя выходить за пределы доступного адресного пространства, поэтому придётся обращаться
	//не к sysResInfo_ напрямую а к локальным переменным.
	unsigned long long sysMemFreeSize;
	if (sysResInfo_.systemMemoryFreeSize < addrSizeLimit)
		sysMemFreeSize = sysResInfo_.systemMemoryFreeSize;
	else
		sysMemFreeSize = addrSizeLimit;
	//sysMemFreeSize = STB.InfoSizeToBytesNum("2m");	//for tests
	unsigned long long sysMemFullSize;
	if (sysResInfo_.systemMemoryFullSize < addrSizeLimit)
		sysMemFullSize = sysResInfo_.systemMemoryFullSize;
	else
		sysMemFullSize = addrSizeLimit;
	//sysMemFullSize = STB.InfoSizeToBytesNum("2m");	//for tests

	//Смотрим влезет ли указанный минимальный размер в оперативку и в лимит по адресному
	//пространству (что актуально для 32битной версии)
	if (filterObj.getMinMemSize() > sysResInfo_.maxProcessMemorySize)
	{
		if (errObj)
			errObj->SetError(CommonErrors::CantAllocMemory, ", минимальный размер блока, которыми \
может быть обработано изображение превышает размер адресного пространства процесса. Попробуйте \
воспользоваться 64-битной версией программы.");
		return false;
	}
	if ((swapMode == SwapMode::SilentNoswap) && (filterObj.getMinMemSize() > sysMemFreeSize))
	{
		if (errObj)
			errObj->SetError(CommonErrors::CantAllocMemory, ", попробуйте поменять настройку --memmode");
		return false;
	};

	//Теперь всё зависит от режима работы с памятью.
	if (confObj_->getMemMode() == MemoryMode::OneChunk)
	{
		if ((swapMode == SwapMode::Ask) && (filterObj.getMaxMemSize() > sysMemFreeSize))
		{
			//Нужно обрабатывать изображение одним куском, в свободную память оно не лезет, но
			//помещается в максимально доступное адресное пространство. Надо спрашивать юзера.
			cout << endl;
			if (!ConsoleAnsweredYes("Изображение не поместится в свободное ОЗУ но должно поместиться в подкачку\n\
Продолжить работу?"))
			{
				if (errObj)
					errObj->SetError(CommonErrors::CantAllocMemory, ", попробуйте поменять настройку --memmode");
				return false;
			}
		}
		maxMemCanBeUsed_ = filterObj.getMaxMemSize();
		maxBlocksCanBeUsed_ = 1;
	}
	//Далее - сначала получаем просто предельный лимит в байтах, и только потом единообразно
	//уменьшим его так, чтобы размер был кратен размеру блока.
	else if ((confObj_->getMemMode() == MemoryMode::Limit) ||
		(confObj_->getMemMode() == MemoryMode::LimitFullPrc))
	{
		//Максимальное количество памяти, которое можно использовать - задано либо явно, в байтах,
		//либо в процентах.
		unsigned long long tempLimit = 0;
		if (confObj_->getMemMode() == MemoryMode::Limit)
			tempLimit = confObj_->getMemSize();
		else if (confObj_->getMemMode() == MemoryMode::LimitFullPrc)
			tempLimit = confObj_->getMemSize() * (sysResInfo_.systemMemoryFullSize / 100);
		//То что получилось - не должно превышать размер адресного пространства.
		if (tempLimit > addrSizeLimit)
		{
			tempLimit = addrSizeLimit;
		};
		//Кроме того, нет смысла чтобы оно превышало максимальный размер, какой может потребоваться
		//для обработки изображения.
		if (tempLimit > filterObj.getMaxMemSize())
		{
			tempLimit = filterObj.getMaxMemSize();
		};
		//В любом случае - если оно помещается в свободную память - вопросов нет. Если только
		//с подкачкой - смотрим на swapMode и при необходимости спрашиваем у юзера.
		if (tempLimit > sysMemFreeSize)
		{
			switch (swapMode)
			{
			case SwapMode::Ask:
				cout << endl;
				if (ConsoleAnsweredYes("Изображение не поместится в свободное ОЗУ блоками, размер которых\n\
задан в настройках, но может поместиться блоками этого размера в \"подкачку\"\n\
Продолжить работу с \"подкачкой\" (да) или использовать только свободное ОЗУ \n\
блоками меньшего размера (нет)?"))
				{
					maxMemCanBeUsed_ = tempLimit;
				}
				else
				{
					maxMemCanBeUsed_ = sysMemFreeSize;
				};
				break;
			case SwapMode::SilentNoswap:
				maxMemCanBeUsed_ = sysMemFreeSize;
				break;
			case SwapMode::SilentUseswap:
				maxMemCanBeUsed_ = tempLimit;
			}
		}
		else
		{
			maxMemCanBeUsed_ = tempLimit;
		};

		//В случае если у нас размер строго равен максимальному для обработки изображения - работаем
		//в режиме как при обработке одним куском.
		if (maxMemCanBeUsed_ == filterObj.getMaxMemSize())
		{
			confObj_->setMemModeCmd(MemoryMode::OneChunk, 0);
			maxBlocksCanBeUsed_ = 1;
			return true;
		};

		//Могло получиться так что минимальный блок не поместится в выбранное пространство.
		//а лимит тут жёсткий, поэтому:
		if (maxMemCanBeUsed_ < filterObj.getMinMemSize())
		{
			if (errObj)
				errObj->SetError(CommonErrors::CantAllocMemory, ", попробуйте поменять настройку --memmode");
			maxMemCanBeUsed_ = 0;
			maxBlocksCanBeUsed_ = 0;
			return false;
		}
	}
	else if (confObj_->getMemMode() == MemoryMode::StayFree)
	{
		//Оставить фиксированное количество места в ОЗУ.
		if ((confObj_->getMemSize() > sysResInfo_.systemMemoryFreeSize))
		{
			//Не помещаемся никак :(
			if (errObj)
				errObj->SetError(CommonErrors::CantAllocMemory, ", попробуйте поменять настройку --memmode");
			return false;
		};

		maxMemCanBeUsed_ = sysResInfo_.systemMemoryFreeSize - confObj_->getMemSize();
		//Но нельзя превышать размер адресного пространства.
		if (maxMemCanBeUsed_ > sysMemFreeSize)
			maxMemCanBeUsed_ = sysMemFreeSize;

		//Могло получиться так что минимальный блок не поместится в выбранное пространство.
		//а лимит тут жёсткий, поэтому:
		if (maxMemCanBeUsed_ < filterObj.getMinMemSize())
		{
			if (errObj)
				errObj->SetError(CommonErrors::CantAllocMemory, ", попробуйте поменять настройку --memmode");
			maxMemCanBeUsed_ = 0;
			maxBlocksCanBeUsed_ = 0;
			return false;
		}

		//Нет смысла чтобы оно превышало максимальный размер, какой может потребоваться
		//для обработки изображения.
		if (maxMemCanBeUsed_ > filterObj.getMaxMemSize())
		{
			maxMemCanBeUsed_ = filterObj.getMaxMemSize();
			//И поскольку в память мы влазим - работаем дальше так как будто в командной
			//строке поставили режим onechunk.
			confObj_->setMemModeCmd(MemoryMode::OneChunk, 0);
			maxBlocksCanBeUsed_ = 1;
			return true;
		};
	}
	else if ((confObj_->getMemMode() == MemoryMode::LimitFreePrc) ||
		(confObj_->getMemMode() == MemoryMode::Auto))
	{
		//Процент от свободного ОЗУ. Если стоял автомат - используем 80%
		switch (confObj_->getMemMode())
		{
		case MemoryMode::LimitFreePrc:
			maxMemCanBeUsed_ = confObj_->getMemSize() * (sysResInfo_.systemMemoryFreeSize / 100);
			break;
		case MemoryMode::Auto:
			maxMemCanBeUsed_ = 80 * (sysResInfo_.systemMemoryFreeSize / 100);
		};
		//Но нельзя превышать размер адресного пространства.
		if (maxMemCanBeUsed_ > sysMemFreeSize)
			maxMemCanBeUsed_ = sysMemFreeSize;
		//Нет смысла чтобы оно превышало максимальный размер, какой может потребоваться
		//для обработки изображения.
		if (maxMemCanBeUsed_ > filterObj.getMaxMemSize())
		{
			maxMemCanBeUsed_ = filterObj.getMaxMemSize();
			//И поскольку в память мы влазим - работаем дальше так как будто в командной
			//строке поставили режим onechunk.
			confObj_->setMemModeCmd(MemoryMode::OneChunk, 0);
			maxBlocksCanBeUsed_ = 1;
			return true;
		};
		//Если получившийся объём памяти меньше минималки - в зависимости от swapMode
		//можно попробовать использовать некую долю от реального ОЗУ.
		if (maxMemCanBeUsed_ < filterObj.getMinMemSize())
		{
			maxMemCanBeUsed_ = 90 * (sysMemFreeSize / 100);
			if (!((maxMemCanBeUsed_ >= filterObj.getMinMemSize()) && (swapMode == SwapMode::Ask)
				&& ConsoleAnsweredYes(to_string(confObj_->getMemSize()) + "% \
свободной памяти недостаточно для работы фильтра. Попробовать взять\n90%?")))
			{
				//Нельзя пробовать брать больше памяти. Вся надежда на то что разрешат подкачку.
				maxMemCanBeUsed_ = 0;
			};
		};
		//Всё ещё есть вероятность что памяти недостаточно.
		if (maxMemCanBeUsed_ < filterObj.getMinMemSize())
		{
			if ((swapMode == SwapMode::SilentUseswap) || ((swapMode == SwapMode::Ask) &&
				(ConsoleAnsweredYes("Изображение поместится в память только при активном \
использовании \"подкачки\". Продолжить?"))))
			{
				//Пока непонятно сколько точно памяти лучше выделять, остановлюсь либо на 70%
				//от ОЗУ либо на минимально возможном для работы размере, смотря что больше.
				maxMemCanBeUsed_ = 70 * (sysMemFullSize / 100);
				if (maxMemCanBeUsed_ < filterObj.getMinMemSize())
					maxMemCanBeUsed_ = filterObj.getMinMemSize();
			}
			else
			{
				if (errObj)
					errObj->SetError(CommonErrors::CantAllocMemory, ", попробуйте поменять настройку --memmode");
				maxMemCanBeUsed_ = 0;
				maxBlocksCanBeUsed_ = 0;
				return false;
			}
		}
		//То что получилось - не должно превышать размер адресного пространства.
		if (maxMemCanBeUsed_ > addrSizeLimit)
		{
			maxMemCanBeUsed_ = addrSizeLimit;
		}
	}
	else
	{
		//На вход функции пришло фиг знает что.
		if (errObj)
			errObj->SetError(CommonErrors::UnknownError, ", AppUIConsole::DetectMaxMemoryCanBeUsed() - unknown MemMode.");
		return false;
	}

	//Возможно надо скорректировать полученный размер чтобы он был кратен размеру блока.
	//Одновременно вычислим количество блоков.
	if (!(confObj_->getMemMode() == MemoryMode::OneChunk))
	{
		//Фиксированная часть памяти, не обязана быть кратной размеру блока. Может быть 0.
		unsigned long long invariableMemSize = filterObj.getMinMemSize() - filterObj.getMinBlockSize();
		//Вот эта часть _должна_ будет быть кратна размеру блока после выполнения кода ниже.
		unsigned long long variableMemSize = maxMemCanBeUsed_ - invariableMemSize;
		//Если места не хватает - гроб гроб кладбище.
		if (filterObj.getMinBlockSize() > variableMemSize)
		{
			if (errObj)
				errObj->SetError(CommonErrors::UnknownError, ", AppUIConsole::DetectMaxMemoryCanBeUsed() - wrong minBlockSize.");
			maxMemCanBeUsed_ = 0;
			maxBlocksCanBeUsed_ = 0;
			return false;
		}
		//Считаем сколько реально потребуется блоков и памяти.
		maxBlocksCanBeUsed_ = int(variableMemSize / filterObj.getMinBlockSize())+2;
		maxMemCanBeUsed_ = (maxBlocksCanBeUsed_ * filterObj.getMinBlockSize())-invariableMemSize;
	}

	return true;
}

//Задетектить инфу для sysResInfo_
void AppUIConsole::DetectSysResInfo()
{
	SysTB::GetSysResInfo(sysResInfo_);
}

//Задать юзверю попрос на да\нет и вернуть true если было да и false если было нет.
bool AppUIConsole::ConsoleAnsweredYes(const string &messageText)
{
	PrintToConsole(messageText + " да\\нет? (y\\д\\n\\н) > ");
	string answer;
	cin >> answer;
	StrTB::Utf8ToLower(StrTB::ConsoleCharsetToUtf8(answer), answer);
	if ((answer == "y") || (answer == "д") || (answer == "yes") || (answer == "да"))
		return true;
	else return false;
}

//Напечатать в консоль сообщение об ошибке.
void AppUIConsole::ConsolePrintError(ErrorInfo &errObj)
{
	PrintToConsole("Ошибка: " + errObj.getErrorText() + "\n");
}

//--------------------------------//
//          Самое важное          //
//--------------------------------//

//Готовит приложение к запуску.
//Внутрь передаётся объект с конфигом, в который уже должны быть прочитаны параметры
//командной строки.
void AppUIConsole::InitApp(std::unique_ptr<AppConfig> conf)
{
	confObj_ = move(conf);
	GDALRegister_GTiff();	//Регистрация драйвера GDAL.
	//Замена обработчика ошибок GDAL на свой, корректно работающий в данном потоке.
	CPLPushErrorHandlerEx(GDALThreadErrorHandler,(void*)(&GDALErrorMsgBuffer));
	//Явное включение именно консольной кодировки и завершение инициализации объекта program options
	StrTB::SelectConsoleEncoding();
	confObj_->FinishInitialization();
	//Детектим параметры системы - память, ядра процессора
	DetectSysResInfo();
}

//По сути тут главный цикл.
int AppUIConsole::RunApp()
{
	//Поприветствуем юзверя и расскажем где мы есть.
	PrintToConsole("\nПриветствую! Это geoimgconv v."+ APP_VERSION+"\n");
	PrintToConsole("Данная программа предназначена для преобразования геокартинок.\n");
	PrintToConsole("Это прототип. Не ждите от него многого.\n");

	//Выдать больше инфы если не было передано никаких опций командной строки.
	if (confObj_->getArgc() == 1)
	{
		PrintToConsole("Вы можете вызвать geoimgconv -h чтобы увидеть справку по опциям командной\n");
		PrintToConsole("строки.\n\nПо умолчанию (если вызвать без опций) - программа откроет\n");
		PrintToConsole("файл с именем input.tif и попытается его обработать медианным фильтром,\n");
		PrintToConsole("сохранив результат в output.tif (при необходимости пересоздав этот файл).\n");
	}
	cout << endl;

	//Реагируем на опции командной строки.
	if (confObj_->getHelpAsked())
	{
		PrintHelp();
		return 0;
	}
	else if (confObj_->getVersionAsked())
	{
		//Версия и так выводится, поэтому просто делаем ничего.
		return 0;
	};

	//Выведем немного инфы о системе, где мы запущены.
	//PrintToConsole("Программа запущена по пути: "+ getAppPath() + "\n");
	//PrintToConsole("Текущий рабочий путь: "+getCurrPath() + "\n");
	/*PrintToConsole("\nОбнаружено ядер процессора: " +
	to_string(sysResInfo_.cpuCoresNumber) + "\n");*/
	PrintToConsole("Всего ОЗУ: " + StrTB::BytesNumToInfoSizeStr(sysResInfo_.systemMemoryFullSize) + ".\n");
	PrintToConsole("Доступно ОЗУ: " + StrTB::BytesNumToInfoSizeStr(sysResInfo_.systemMemoryFreeSize) + ".\n");
	PrintToConsole("Процесс может адресовать памяти: " + StrTB::BytesNumToInfoSizeStr(sysResInfo_.maxProcessMemorySize) + ".\n");
	PrintToConsole("Выбран режим работы с памятью: " + MemoryModeTexts[(unsigned char)confObj_->getMemMode()] + "\n");
	/*PrintToConsole("Размер, указанный для режима работы с памятью: " +
	to_string(confObj_->getMemSize()) + "\n\n");*/

	//Всё-таки нужно обработать картинку. В зависимости от выбранного режима создадим нужный фильтр.
	ErrorInfo errObj;
	if (confObj_->getMedfilterAlgo() == MedfilterAlgo::Stub)
	{
		medFilter_.reset(new MedianFilterStub());
		PrintToConsole("\nБудет применяться медианный фильтр: stub (имитация).\n");
	}
	else if (confObj_->getMedfilterAlgo() == MedfilterAlgo::Stupid)
	{
		medFilter_.reset(new MedianFilterStupid());
		PrintToConsole("\nБудет применяться медианный фильтр: stupid (реализация \"в лоб\").\n");
	}
	else if (confObj_->getMedfilterAlgo() == MedfilterAlgo::Huang)
	{
		medFilter_.reset(new MedianFilterHuang(confObj_->getMedfilterHuangLevelsNum()));
		PrintToConsole("\nБудет применяться медианный фильтр: huang (быстрый алгоритм Хуанга).\n");
	}
	else
	{
		//Выбран непонятно какой режим. Это очень печально. Ругаемся и умираем.
		errObj.SetError(CommonErrors::InternalError,": выбран неизвестный алгоритм медианной фильтрации.");
		ConsolePrintError(errObj);
		return 1;
	};
	//При необходимости включим режим заполнения ям (фактически это будет обычный медианный фильтр).
	PrintToConsole("Режим заполнения ям: ");
	if (confObj_->getMedfilterFillPits())
	{
		medFilter_->setFillPits(true);
		PrintToConsole("включен.\n\n");
	}
	else
	{
		medFilter_->setFillPits(false);
		PrintToConsole("отключен.\n\n");
	}
	//Настраиваем медианный фильтр и пути к файлам.
	b_fs::path inputFilePath = b_fs::absolute(StrTB::Utf8ToWstring(confObj_->getInputFileName()), StrTB::Utf8ToWstring(getCurrPath()));
	b_fs::path outputFilePath = b_fs::absolute(StrTB::Utf8ToWstring(confObj_->getOutputFileName()), StrTB::Utf8ToWstring(getCurrPath()));
	string inputFileName = StrTB::WstringToUtf8(inputFilePath.wstring());
	string outputFileName = StrTB::WstringToUtf8(outputFilePath.wstring());
	medFilter_->setAperture(confObj_->getMedfilterAperture());
	medFilter_->setThreshold(confObj_->getMedfilterThreshold());
	medFilter_->setMarginType(confObj_->getMedfilterMarginType());
	PrintToConsole("Пытаюсь открыть исходный файл:\n" + inputFileName + "\n");
	if (!medFilter_->OpenInputFile(inputFileName, &errObj))
	{
		ConsolePrintError(errObj);
		return 1;
	}
	//Детектим сколько памяти нам нужно
	if (!DetectMaxMemoryCanBeUsed(*medFilter_, SwapMode::Ask, &errObj))
	{
		ConsolePrintError(errObj);
		return 1;
	}
	//Открываем файл который будем сохранять.
	PrintToConsole("Создаю файл назначения и занимаю под него место:\n" + outputFileName + "\n");
	if (!medFilter_->OpenOutputFile(outputFileName, true, &errObj))
	{
		ConsolePrintError(errObj);
		return 1;
	};

	///////////TEST///////////
	//Тут удобно руками менять параметры чтобы проверять работу на всяких граничных значениях.
	//confObj_->setMemModeCmd(Auto, 0);
	//maxBlocksCanBeUsed_ = 13;
	///////////TEST///////////

	//Настраиваем фильтр в соответствии с полученной инфой о памяти.
	if (confObj_->getMemMode() == MemoryMode::OneChunk)
		medFilter_->setUseMemChunks(false);
	else
		medFilter_->setUseMemChunks(true);
	medFilter_->setBlocksInMem(maxBlocksCanBeUsed_);

	//PrintToConsole("Открыто. Визуализация значимых пикселей:\n");
	////medFilter.SourcePrintStupidVisToCout();
	//PrintToConsole("Устарела и пока не работает :(\n\n");
	PrintToConsole("Готово. Начинаю обработку...\n\n");

	PrintToConsole("Программа будет обрабатывать файл, используя до " +
		StrTB::BytesNumToInfoSizeStr(maxMemCanBeUsed_) + " памяти, частями по\n" +
		to_string(maxBlocksCanBeUsed_) + " блока(ов).\n");
	if (confObj_->getMemMode() != MemoryMode::OneChunk)
		PrintToConsole("Размер блока: до " + StrTB::BytesNumToInfoSizeStr(medFilter_->getMinBlockSize()) + ".\n");
	cout << endl;

	//Собственно, запуск фильтра.
	AppUIConsoleCallBack CallBackObj;
	CallBackObj.OperationStart();
	if (!medFilter_->ApplyFilter(&CallBackObj, &errObj))
	{
		ConsolePrintError(errObj);
		return 1;
	}
	CallBackObj.OperationEnd();

	//Закрываем файлы.
	medFilter_->CloseAllFiles();
	//Объект удалим сразу, не смотря на то что в крайнем случае об этом бы позаботился деструктор AppUIConsole
	medFilter_.reset(nullptr);

	PrintToConsole("Готово.\n");

	return 0;
}

//Метод для запуска в тестовом режиме.
//Вся эта жуть использовалась до того, как к проекту были прикручены юнит-тесты.
int AppUIConsole::RunTestMode()
{
	;
	////Тестируем методы проверки строк.
	////float
	//PrintToConsole("123 is float: " + STB.BoolToString(STB.CheckFloatStr("123")) + "\n");
	//PrintToConsole("-123.0 is float: " + STB.BoolToString(STB.CheckFloatStr("-123.0")) + "\n");
	//PrintToConsole("+123.0 is float: " + STB.BoolToString(STB.CheckFloatStr("+123.0")) + "\n");
	//PrintToConsole("*123.0 is float: " + STB.BoolToString(STB.CheckFloatStr("*123.0")) + "\n");
	//PrintToConsole("-123,0 is float: " + STB.BoolToString(STB.CheckFloatStr("-123,0")) + "\n");
	//PrintToConsole("123.0 is float: " + STB.BoolToString(STB.CheckFloatStr("123.0")) + "\n");
	//PrintToConsole("123,0 is float: " + STB.BoolToString(STB.CheckFloatStr("123,0")) + "\n");
	//PrintToConsole("123.0 is float: " + STB.BoolToString(STB.CheckFloatStr("123.0")) + "\n");
	//PrintToConsole("абырвалг is float: " + STB.BoolToString(STB.CheckFloatStr("абырвалг")) + "\n");
	//PrintToConsole("\"\" is float: " + STB.BoolToString(STB.CheckFloatStr("")) + "\n");
	////unsigned int
	//PrintToConsole("123.0 is unsInt: " + STB.BoolToString(STB.CheckUnsIntStr("123.0")) + "\n");
	//PrintToConsole("-123.0 is unsInt: " + STB.BoolToString(STB.CheckUnsIntStr("-123.0")) + "\n");
	//PrintToConsole("123 is unsInt: " + STB.BoolToString(STB.CheckUnsIntStr("123")) + "\n");
	//PrintToConsole("-123 is unsInt: " + STB.BoolToString(STB.CheckUnsIntStr("-123")) + "\n");
	//PrintToConsole("+123 is unsInt: " + STB.BoolToString(STB.CheckUnsIntStr("+123")) + "\n");
	//PrintToConsole("*123 is unsInt: " + STB.BoolToString(STB.CheckUnsIntStr("*123")) + "\n");
	//PrintToConsole("абырвалг is unsInt: " + STB.BoolToString(STB.CheckUnsIntStr("абырвалг")) + "\n");
	//PrintToConsole("\"\" is unsInt: " + STB.BoolToString(STB.CheckUnsIntStr("")) + "\n");
	////signed int
	//PrintToConsole("123.0 is int: " + STB.BoolToString(STB.CheckSignedIntStr("123.0")) + "\n");
	//PrintToConsole("-123.0 is int: " + STB.BoolToString(STB.CheckSignedIntStr("-123.0")) + "\n");
	//PrintToConsole("123 is int: " + STB.BoolToString(STB.CheckSignedIntStr("123")) + "\n");
	//PrintToConsole("-123 is int: " + STB.BoolToString(STB.CheckSignedIntStr("-123")) + "\n");
	//PrintToConsole("+123 is int: " + STB.BoolToString(STB.CheckSignedIntStr("-123")) + "\n");
	//PrintToConsole("*123 is int: " + STB.BoolToString(STB.CheckSignedIntStr("*123")) + "\n");
	//PrintToConsole("абырвалг is int: " + STB.BoolToString(STB.CheckSignedIntStr("абырвалг")) + "\n");
	//PrintToConsole("\"\" is int: " + STB.BoolToString(STB.CheckSignedIntStr("")) + "\n");
	////InfoSizeStr
	//PrintToConsole("123 is InfoSizeStr: " + STB.BoolToString(STB.CheckInfoSizeStr("123")) + "\n");
	//PrintToConsole("-123 is InfoSizeStr: " + STB.BoolToString(STB.CheckInfoSizeStr("-123")) + "\n");
	//PrintToConsole("123ы is InfoSizeStr: " + STB.BoolToString(STB.CheckInfoSizeStr("123ы")) + "\n");
	//PrintToConsole("123b is InfoSizeStr: " + STB.BoolToString(STB.CheckInfoSizeStr("123b")) + "\n");
	//PrintToConsole("-123b is InfoSizeStr: " + STB.BoolToString(STB.CheckInfoSizeStr("-123b")) + "\n");
	//PrintToConsole("+123b is InfoSizeStr: " + STB.BoolToString(STB.CheckInfoSizeStr("+123b")) + "\n");
	//PrintToConsole("123c is InfoSizeStr: " + STB.BoolToString(STB.CheckInfoSizeStr("123c")) + "\n");
	//PrintToConsole("123M is InfoSizeStr: " + STB.BoolToString(STB.CheckInfoSizeStr("123M")) + "\n");
	//PrintToConsole("123G is InfoSizeStr: " + STB.BoolToString(STB.CheckInfoSizeStr("123G")) + "\n");
	//PrintToConsole("123t is InfoSizeStr: " + STB.BoolToString(STB.CheckInfoSizeStr("123t")) + "\n");
	////InfoSizeStr as bytes
	//PrintToConsole("АБЫРВАЛГ is in bytes: ");
	//cout << STB.InfoSizeToBytesNum("АБЫРВАЛГ") << "\n";
	//PrintToConsole("\"\" is in bytes: ");
	//cout << STB.InfoSizeToBytesNum("") << "\n";
	//PrintToConsole("1 is in bytes: ");
	//cout << STB.InfoSizeToBytesNum("1") << "\n";
	//PrintToConsole("1ы is in bytes: ");
	//cout << STB.InfoSizeToBytesNum("1ы") << "\n";
	//PrintToConsole("1b is in bytes: ");
	//cout << STB.InfoSizeToBytesNum("1b") << "\n";
	//PrintToConsole("1K is in bytes: ");
	//cout << STB.InfoSizeToBytesNum("1K") << "\n";
	//PrintToConsole("1m is in bytes: ");
	//cout << STB.InfoSizeToBytesNum("1m") << "\n";
	//PrintToConsole("1G is in bytes: ");
	//cout << STB.InfoSizeToBytesNum("1G") << "\n";
	//PrintToConsole("1t is in bytes: ");
	//cout << STB.InfoSizeToBytesNum("1t") << "\n";

	////Тестирование AltMatrix<>::LoadFromGDALRaster
	//GDALRegister_GTiff();	//Регистрация драйвера GDAL.
	//cout << "Opening input.tif..." << endl;
	//GDALDataset *inputDataset = (GDALDataset*)GDALOpen("input.tif", GA_ReadOnly);
	//GDALRasterBand *inputRaster = inputDataset->GetRasterBand(1);
	//int rasterXSize = inputRaster->GetXSize();
	//int rasterYSize = inputRaster->GetYSize();
	//AltMatrix<double> testMatrix(true);
	//int marginSize = 50;
	////Читаем первые 150 строк в пустую матрицу, сохраняем что получилось как csv
	//testMatrix.CreateEmpty(rasterXSize + marginSize * 2, 250);
	//cout << "Reading first 200 lines..." << endl;
	//testMatrix.LoadFromGDALRaster(inputRaster, 0, 200, marginSize, File1);
	//cout << "Saving output01.csv..." << endl;
	//testMatrix.SaveToCSVFile("output01.csv");
	////Читаем следующие 150 строк.
	//cout << "Reading next 150 lines..." << endl;
	//testMatrix.LoadFromGDALRaster(inputRaster, 200, 150, marginSize, Matr, &testMatrix);
	//cout << "Saving output02.csv..." << endl;
	//testMatrix.SaveToCSVFile("output02.csv");
	////Читаем следующие 150 строк.
	//cout << "Reading next 150 lines..." << endl;
	//testMatrix.LoadFromGDALRaster(inputRaster, 350, 150, marginSize, Matr, &testMatrix);
	//cout << "Saving output03.csv..." << endl;
	//testMatrix.SaveToCSVFile("output03.csv");
	////Читаем остатки.
	//cout << "Reading next 92 lines..." << endl;
	//testMatrix.LoadFromGDALRaster(inputRaster, 500, 92, marginSize, Matr, &testMatrix);
	//cout << "Saving output04.csv..." << endl;
	//testMatrix.SaveToCSVFile("output04.csv");
	//cout << "Done." << endl;
	////Главное не забыть закрыть файл!!!
	//GDALClose(inputDataset);
	;
	////Тест выделения больших (сравнимых с размером адресного пространства) блоков в памяти.
	//string tempStr;
	//cout << endl << "Ready to start. Enter string to continue... > ";
	//cin >> tempStr;
	//cout << endl << "getting 900m..." << endl;
	//unsigned long long memSize = STB.InfoSizeToBytesNum("900m");
	//char *testArr = new char[(unsigned int)(memSize)];
	//cout << "OK!" << endl;
	//cout << endl << "getting more 200m..." << endl;
	//unsigned long long memSize2 = STB.InfoSizeToBytesNum("300m");
	//char *testArr2 = new char[(unsigned int)(memSize2)];
	//cout << "OK!" << endl;
	//cin >> tempStr;

	////Тестирование получения инфы о минимальной и максимальной высоте из
	////изображения.
	//GDALRegister_GTiff();	//Регистрация драйвера GDAL.
	//cout << "Opening input.tif..." << endl;
	//GDALDataset *inputDataset = (GDALDataset*)GDALOpen("input.tif", GA_ReadOnly);
	//GDALRasterBand *inputRaster = inputDataset->GetRasterBand(1);
	//cout << "NoDataValue: " << STB.DoubleToString(inputRaster->GetNoDataValue(),5) << endl;
	//inputRaster->SetNoDataValue(0.0);
	//cout << "New NoDataValue is: " << STB.DoubleToString(inputRaster->GetNoDataValue(), 5) << endl;
	//double minMaxArr[2];
	//inputRaster->ComputeRasterMinMax(false, &(minMaxArr[0]));
	//cout << "MinValue: " << STB.DoubleToString(minMaxArr[0], 5) << endl;
	//cout << "MaxValue: " << STB.DoubleToString(minMaxArr[1], 5) << endl;
	//cout << "LevelDelta for 255 levels is: " << STB.DoubleToString(
	//	(minMaxArr[1] - minMaxArr[0]) / 255.0, 5) << endl;
	////Главное не забыть закрыть файл!!!
	//GDALClose(inputDataset);

	//Заглушка, работает когда ничего не тестируется.
	PrintToConsole("Вы кто такие? Я вас не звал! Никаких тестов в этой версии\n\
всё равно не выполняется ;).\n");

	return 0;
}

//--------------------------------//
//       Прочий функционал        //
//--------------------------------//

//Вывести сообщение в обычную (не curses) консоль в правильной кодировке.
void AppUIConsole::PrintToConsole(const string &str)
{
	cout << StrTB::Utf8ToConsoleCharset(str) << flush;
}

//Вывод справки.
void AppUIConsole::PrintHelp()
{
	cout << confObj_->getHelpMsg();
}

} //namespace geoimgconv
