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
#include "small_tools_box.h"
#include "median_filter.h"
#include <boost/lexical_cast.hpp>
#include <cmath>

using namespace boost;
using namespace std;

namespace geoimgconv
{

const std::string APP_VERSION = "0.3.0.0a";	//Временно версия будет вот так. Но нет ничего более постоянного, чем временное!
const double DEFAULT_PROGRESS_UPDATE_PERIOD = 2.8;	//С какой периодичностью выводить прогресс в консоль во избежание тормозов от вывода. В секундах.

///////////////////////////////////////
//    Класс AppUIConsoleCallBack     //
///////////////////////////////////////

//--------------------------------//
//   Конструкторы-деструкторы     //
//--------------------------------//

AppUIConsoleCallBack::AppUIConsoleCallBack() : skipCounter_(0), skipNumber_(1),
	isStarted_(false), isNotClean_(false), lastTextSize_(0), pixelsPerSecond_(0.0),
	updatePeriod_(DEFAULT_PROGRESS_UPDATE_PERIOD)
{

}

//--------------------------------//
//        Прочие методы           //
//--------------------------------//

//Возвращает объект в состояние как будто только после инициализации.
void AppUIConsoleCallBack::Clear()
{
	if (isNotClean_)
	{
		skipCounter_ = 0;
		skipNumber_ = 1;
		isStarted_ = false;
		isNotClean_ = false;
		lastTextSize_ = 0;
		pixelsPerSecond_ = 0.0;
		updatePeriod_ = DEFAULT_PROGRESS_UPDATE_PERIOD;
	};
}


void AppUIConsoleCallBack::CallBack(const unsigned long &progressPosition)
//CallBack-метод, который будет выводить в консоль количество и процент уже
//обработанных пикселей.
{
	using namespace boost::posix_time;

	if (progressPosition < getMaxProgress())
	{
		if (skipCounter_ < skipNumber_)
		{
			skipCounter_++;
			return;
		};
		//skipCounter_ будет обнулён позже.
	};
	//Можно выводить.
	if (isStarted_)
	{
		cout << "\r";	//Переводим вывод на начало текущей строки.
		//А ещё сейчас надо проверить сколько времени занял предыдущий период вычислений
		//и если надо - подкрутить skipNumber_. Заодно вычислим количество пикселей в секунду.
		nowTime_ = microsec_clock::local_time();
		timeDelta_ = nowTime_ - lastPrintTime_;
		currMilliseconds_ = timeDelta_.total_milliseconds();
		//Всегда считаем что операция заняла хоть сколько-то времени. Иначе гроб гроб кладбище...
		if (!currMilliseconds_) currMilliseconds_ = 10;
		//И только теперь можно считать скорость и всё прочее.
		pixelsPerSecond_ = 1000.0 / (double(currMilliseconds_) / double(skipCounter_));
		if (pixelsPerSecond_ < 1)
			skipNumber_ = size_t(std::ceil(updatePeriod_ / pixelsPerSecond_));
		else
			skipNumber_ = size_t(std::ceil(pixelsPerSecond_*updatePeriod_));
	}
	else
		isStarted_ = true;
	lastPrintTime_ = microsec_clock::local_time(); //Запомним на будущее :).
	text_ = lexical_cast<string>(progressPosition) + "/" + lexical_cast<string>(getMaxProgress()) +
		" ( " + lexical_cast<string>(progressPosition / (getMaxProgress() / 100)) + "% ) "
		+ STB.DoubleToString(pixelsPerSecond_,2) + " пикс/с, skipNumber: "
		+ lexical_cast<string>(skipNumber_);
	tempSize_ = text_.size();
	if (tempSize_ < lastTextSize_)
	{
		//Надо добавить пробелов чтобы затереть символы предыдущего вывода
		text_.append(" ", lastTextSize_ - tempSize_);
	}
	lastTextSize_ = tempSize_;
	cout << STB.Utf8ToConsoleCharset(text_) << std::flush;
	//Не забыть обнулить skipCounter_!
	skipCounter_ = 0;
}

void AppUIConsoleCallBack::OperationStart()
//Сообщить объекту о том что операция начинается
{
	//Очистим объект и запомним время начала операции.
	Clear();
	isNotClean_ = true;
	startTime_ = boost::posix_time::microsec_clock::local_time();
}

void AppUIConsoleCallBack::OperationEnd()
//Сообщить объекту о том что операция завершена.
{
	//Запомним время завершения операции и запостим в cout последний
	//вариант прогрессбара и информацию о времени, затраченном на выполнение операции.
	using namespace boost::posix_time;
	endTime_ = microsec_clock::local_time();
	skipCounter_ = 1;
	isStarted_ = false;	//Не даст пересчитать скорость
	cout << "\r";
	CallBack(getMaxProgress());
	cout << STB.Utf8ToConsoleCharset("\nЗавершено за: ")
		<< to_simple_string(endTime_ - startTime_) << endl;
}

//////////////////////////////
//    Класс AppUIConsole    //
//////////////////////////////

//--------------------------------//
//   Конструкторы-деструкторы     //
//--------------------------------//

AppUIConsole::AppUIConsole()
{
}

AppUIConsole::~AppUIConsole()
{
}

//--------------------------------//
//          Самое важное          //
//--------------------------------//

void AppUIConsole::InitApp(AppConfig &conf)
//Готовит приложение к запуску.
//Внутрь передаётся объект с конфигом, в который уже должны быть прочитаны параметры
//командной строки.
{
	confObj_ = &conf;
	GDALRegister_GTiff();	//Регистрация драйвера GDAL.
	//Замена обработчика ошибок GDAL на свой, корректно работающий в данном потоке.
	CPLPushErrorHandlerEx(GDALThreadErrorHandler,(void*)(&GDALErrorMsgBuffer));
	//Явное включение именно консольной кодировки и завершение инициализации объекта program options
	STB.SelectConsoleEncoding();
	confObj_->FinishInitialization();
}

int AppUIConsole::RunApp()
//По сути тут главный цикл.
{
	//Поприветствуем юзверя и расскажем где мы есть.
	PrintToConsole("\nПриветствую! Это geoimgconv v."+ APP_VERSION+"\n");
	PrintToConsole("Данная программа предназначена для преобразования геокартинок.\n");
	PrintToConsole("Это прототип. Не ждите от него многого.\n");
	//PrintToConsole("Программа запущена по пути: "+ getAppPath() + "\n");
	//PrintToConsole("Текущий рабочий путь: "+getCurrPath() + "\n");
	SysResInfo sysResInfo;
	STB.GetSysResInfo(sysResInfo);
	PrintToConsole("\nОбнаружено ядер процессора: " +
		lexical_cast<string>(sysResInfo.cpuCoresNumber) + "\n");
	PrintToConsole("Всего ОЗУ: " + STB.BytesNumToInfoSizeStr(sysResInfo.systemMemoryFullSize) + ".\n");
	PrintToConsole("Доступно ОЗУ: " + STB.BytesNumToInfoSizeStr(sysResInfo.systemMemoryFreeSize) + ".\n");
	PrintToConsole("Процесс может адресовать памяти: " + STB.BytesNumToInfoSizeStr(sysResInfo.maxProcessMemorySize) + ".\n");
	PrintToConsole("Выбран режим работы с памятью: " + MemoryModeTexts[confObj_->getMemMode()] + "\n");
	PrintToConsole("Размер, указанный для режима работы с памятью: " +
		lexical_cast<string>(confObj_->getMemSize()) + "\n\n");

	//Выдать больше инфы если не было передано никаких опций командной строки.
	if (confObj_->getArgc() == 1)
	{
		PrintToConsole("Вы можете вызвать geoimgconv -h чтобы увидеть справку по опциям командной\n");
		PrintToConsole("строки.\n\nПо умолчанию (если вызвать без опций) - программа откроет\n");
		PrintToConsole("файл с именем input.tif и попытается его обработать медианным фильтром,\n");
		PrintToConsole("сохранив результат в output.tif (при необходимости пересоздав этот файл).\n");
	}
	std::cout << std::endl;

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

	//Всё-таки нужно обработать картинку. Настраиваем медианный фильтр и пути к файлам.
	filesystem::path inputFilePath = filesystem::absolute(STB.Utf8ToWstring(confObj_->getInputFileName()), STB.Utf8ToWstring(getCurrPath()));
	filesystem::path outputFilePath = filesystem::absolute(STB.Utf8ToWstring(confObj_->getOutputFileName()), STB.Utf8ToWstring(getCurrPath()));
	string inputFileName = STB.WstringToUtf8(inputFilePath.wstring());
	string outputFileName = STB.WstringToUtf8(outputFilePath.wstring());
	MedianFilter medFilter;
	ErrorInfo errObj;
	medFilter.setAperture(confObj_->getMedfilterAperture());
	medFilter.setThreshold(confObj_->getMedfilterThreshold());
	medFilter.setMarginType(confObj_->getMedfilterMarginType());
	PrintToConsole("Пытаюсь открыть файл:\n" + inputFileName + "\n");
	if (medFilter.LoadImage(inputFileName, &errObj))
	{
		PrintToConsole("Открыто. Визуализация значимых пикселей:\n");
		medFilter.SourcePrintStupidVisToCout();

		//PrintToConsole("Для дополнительной отладки сохраняю файл: input_source.csv\n");
		//if (!medFilter.SourceSaveToCSVFile("input_source.csv", &errObj))
		//{
		//	PrintToConsole("Ошибка: " + errObj.errorText_ + "\n");
		//	return 1;
		//};

		PrintToConsole("Заполняю краевые области...\n");
		PrintToConsole("Тип заполнения: "+ MarginTypesTexts[medFilter.getMarginType()]+"\n");
		AppUIConsoleCallBack CallBackObj;
		CallBackObj.OperationStart();
		medFilter.FillMargins(&CallBackObj);
		CallBackObj.OperationEnd();	//Выведет время выполнения.

		//PrintToConsole("Для дополнительной отладки сохраняю файл: input_filled.csv\n");
		//if (!medFilter.SourceSaveToCSVFile("input_filled.csv", &errObj))
		//{
		//	PrintToConsole("Ошибка: " + errObj.errorText_ + "\n");
		//	return 1;
		//};

		PrintToConsole("Готово. Применяю \"тупую\" версию фильтра.\n");
		PrintToConsole("Апертура: " + lexical_cast<std::string>(medFilter.getAperture()) +
			"; Порог: " + STB.DoubleToString(medFilter.getThreshold(),5) + ".\n");
		CallBackObj.OperationStart();
		medFilter.ApplyStupidFilter(&CallBackObj);
		//medFilter.ApplyStubFilter(&CallBackObj);	//Отладочный "мгновенный" фильтр - только имитирует фильтрацию.
		CallBackObj.OperationEnd();   //Выведет время выполнения.

		//PrintToConsole("Для дополнительной отладки сохраняю файл: output_stupid.csv\n");
		//if (!medFilter.DestSaveToCSVFile("output_stupid.csv", &errObj))
		//{
		//	PrintToConsole("Ошибка: " + errObj.errorText_ + "\n");
		//	return 1;
		//};

		//Вроде всё ок, можно сохранять.
		PrintToConsole("Готово. Сохраняю файл:\n" + outputFileName + "\n");
		if (!medFilter.SaveImage(outputFileName, &errObj))
		{
			PrintToConsole("Ошибка: " + errObj.getErrorText() + "\n");
			return 1;
		};

		PrintToConsole("Готово.\n");
	}
	else
	{
		//Что-то пошло не так.
		PrintToConsole("Ошибка: " + errObj.getErrorText() + "\n");
		return 1;
	}

	return 0;
}

int AppUIConsole::RunTestMode()
//Метод для запуска в тестовом режиме.
{
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

	//Заглушка, работает когда ничего не тестируется.
	PrintToConsole("Вы кто такие? Я вас не звал! Никаких тестов в этой версии\n\
всё равно не выполняется ;).\n");
	return 0;
}

//--------------------------------//
//       Прочий функционал        //
//--------------------------------//

void AppUIConsole::PrintToConsole(const std::string &str)
//Вывести сообщение в обычную (не curses) консоль в правильной кодировке.
{
	std::cout << STB.Utf8ToConsoleCharset(str) << std::flush;
}

void AppUIConsole::PrintHelp()
//Вывод справки.
{
	std::cout << confObj_->getHelpMsg();
}

} //namespace geoimgconv
