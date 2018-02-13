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

const std::string APP_VERSION = "0.0.3.0a";	//Временно версия будет вот так. Но нет ничего более постоянного, чем временное!
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
	if (this->isNotClean_)
	{
		this->skipCounter_ = 0;
		this->skipNumber_ = 1;
		this->isStarted_ = false;
		this->isNotClean_ = false;
		this->lastTextSize_ = 0;
		this->pixelsPerSecond_ = 0.0;
		this->updatePeriod_ = DEFAULT_PROGRESS_UPDATE_PERIOD;
	};
}


void AppUIConsoleCallBack::CallBack(const unsigned long &progressPosition)
//CallBack-метод, который будет выводить в консоль количество и процент уже
//обработанных пикселей.
{
	using namespace boost::posix_time;

	if (progressPosition < this->getMaxProgress())
	{
		if (this->skipCounter_ < this->skipNumber_)
		{
			this->skipCounter_++;
			return;
		};
		//skipCounter_ будет обнулён позже.
	};
	//Можно выводить.
	if (this->isStarted_)
	{
		cout << "\r";	//Переводим вывод на начало текущей строки.
		//А ещё сейчас надо проверить сколько времени занял предыдущий период вычислений
		//и если надо - подкрутить skipNumber_. Заодно вычислим количество пикселей в секунду.
		this->nowTime_ = microsec_clock::local_time();
		this->timeDelta_ = this->nowTime_ - this->lastPrintTime_;
		this->currMilliseconds_ = this->timeDelta_.total_milliseconds();
		//Всегда считаем что операция заняла хоть сколько-то времени. Иначе гроб гроб кладбище...
		if (!this->currMilliseconds_) this->currMilliseconds_ = 10;
		//И только теперь можно считать скорость и всё прочее.
		this->pixelsPerSecond_ = 1000.0 / (double(this->currMilliseconds_) / double(this->skipCounter_));
		if (this->pixelsPerSecond_ < 1)
			this->skipNumber_ = size_t(std::ceil(this->updatePeriod_ / this->pixelsPerSecond_));
		else
			this->skipNumber_ = size_t(std::ceil(this->pixelsPerSecond_*this->updatePeriod_));
	}
	else
		this->isStarted_ = true;
	this->lastPrintTime_ = microsec_clock::local_time(); //Запомним на будущее :).
	this->text_ = lexical_cast<string>(progressPosition) + "/" + lexical_cast<string>(this->getMaxProgress()) +
		" ( " + lexical_cast<string>(progressPosition / (this->getMaxProgress() / 100)) + "% ) "
		+ STB.DoubleToString(this->pixelsPerSecond_,2) + " пикс/с, skipNumber: "
		+ lexical_cast<string>(this->skipNumber_);
	this->tempSize_ = this->text_.size();
	if (this->tempSize_ < this->lastTextSize_)
	{
		//Надо добавить пробелов чтобы затереть символы предыдущего вывода
		this->text_.append(" ", this->lastTextSize_ - this->tempSize_);
	}
	this->lastTextSize_ = this->tempSize_;
	cout << STB.Utf8ToConsoleCharset(this->text_) << std::flush;
	//Не забыть обнулить skipCounter_!
	this->skipCounter_ = 0;
}

void AppUIConsoleCallBack::OperationStart()
//Сообщить объекту о том что операция начинается
{
	//Очистим объект и запомним время начала операции.
	this->Clear();
	this->isNotClean_ = true;
	this->startTime_ = boost::posix_time::microsec_clock::local_time();
}

void AppUIConsoleCallBack::OperationEnd()
//Сообщить объекту о том что операция завершена.
{
	//Запомним время завершения операции и запостим в cout последний
	//вариант прогрессбара и информацию о времени, затраченном на выполнение операции.
	using namespace boost::posix_time;
	this->endTime_ = microsec_clock::local_time();
	this->skipCounter_ = 1;
	this->isStarted_ = false;	//Не даст пересчитать скорость
	cout << "\r";
	this->CallBack(this->getMaxProgress());
	cout << STB.Utf8ToConsoleCharset("\nЗавершено за: ")
		<< to_simple_string(this->endTime_ - this->startTime_) << endl;
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
	this->confObj_ = &conf;
	GDALRegister_GTiff();	//Регистрация драйвера GDAL.
	//Замена обработчика ошибок GDAL на свой, корректно работающий в данном потоке.
	CPLPushErrorHandlerEx(GDALThreadErrorHandler,(void*)(&this->GDALErrorMsgBuffer));
	//Явное включение именно консольной кодировки и завершение инициализации объекта program options
	STB.SelectConsoleEncoding();
	this->confObj_->FinishInitialization();
}

int AppUIConsole::RunApp()
//По сути тут главный цикл.
{
	//Поприветствуем юзверя и расскажем где мы есть.
	this->PrintToConsole("\nПриветствую! Это geoimgconv v."+ APP_VERSION+"\n");
	this->PrintToConsole("Данная программа предназначена для преобразования геокартинок.\n");
	this->PrintToConsole("Это прототип. Не ждите от него многого.\n");
	//this->PrintToConsole("Программа запущена по пути: "+ this->getAppPath() + "\n");
	//this->PrintToConsole("Текущий рабочий путь: "+this->getCurrPath() + "\n");
	SysResInfo sysResInfo;
	STB.GetSysResInfo(sysResInfo);
	this->PrintToConsole("\nОбнаружено ядер процессора:" +
		lexical_cast<string>(sysResInfo.cpuCoresNumber) + "\n");
	this->PrintToConsole("Всего ОЗУ: " + STB.BytesNumToInfoSizeStr(sysResInfo.systemMemoryFullSize) + ".\n");
	this->PrintToConsole("Доступно ОЗУ: " + STB.BytesNumToInfoSizeStr(sysResInfo.systemMemoryFreeSize) + ".\n");
	this->PrintToConsole("Процесс может адресовать памяти: " + STB.BytesNumToInfoSizeStr(sysResInfo.maxProcessMemorySize) + ".\n\n");

	//Выдать больше инфы если не было передано никаких опций командной строки.
	if (this->confObj_->getArgc() == 1)
	{
		this->PrintToConsole("Вы можете вызвать geoimgconv -h чтобы увидеть справку по опциям командной\n");
		this->PrintToConsole("строки.\n\nПо умолчанию (если вызвать без опций) - программа откроет\n");
		this->PrintToConsole("файл с именем input.tif и попытается его обработать медианным фильтром,\n");
		this->PrintToConsole("сохранив результат в output.tif (при необходимости пересоздав этот файл).\n");
	}
	std::cout << std::endl;

	//Реагируем на опции командной строки.
	if (this->confObj_->getHelpAsked())
	{
		this->PrintHelp();
		return 0;
	}
	else if (this->confObj_->getVersionAsked())
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
	this->PrintToConsole("Пытаюсь открыть файл:\n" + inputFileName + "\n");
	if (medFilter.LoadImage(inputFileName, &errObj))
	{
		this->PrintToConsole("Открыто. Визуализация значимых пикселей:\n");
		medFilter.SourcePrintStupidVisToCout();

		//this->PrintToConsole("Для дополнительной отладки сохраняю файл: input_source.csv\n");
		//if (!medFilter.SourceSaveToCSVFile("input_source.csv", &errObj))
		//{
		//	this->PrintToConsole("Ошибка: " + errObj.errorText_ + "\n");
		//	return 1;
		//};

		this->PrintToConsole("Заполняю краевые области...\n");
		this->PrintToConsole("Тип заполнения: "+ MarginTypesTexts[medFilter.getMarginType()]+"\n");
		AppUIConsoleCallBack CallBackObj;
		CallBackObj.OperationStart();
		medFilter.FillMargins(&CallBackObj);
		CallBackObj.OperationEnd();	//Выведет время выполнения.

		//this->PrintToConsole("Для дополнительной отладки сохраняю файл: input_filled.csv\n");
		//if (!medFilter.SourceSaveToCSVFile("input_filled.csv", &errObj))
		//{
		//	this->PrintToConsole("Ошибка: " + errObj.errorText_ + "\n");
		//	return 1;
		//};

		this->PrintToConsole("Готово. Применяю \"тупую\" версию фильтра.\n");
		this->PrintToConsole("Апертура: " + lexical_cast<std::string>(medFilter.getAperture()) +
			"; Порог: " + STB.DoubleToString(medFilter.getThreshold(),5) + ".\n");
		CallBackObj.OperationStart();
		medFilter.ApplyStupidFilter(&CallBackObj);
		//medFilter.ApplyStubFilter(&CallBackObj);	//Отладочный "мгновенный" фильтр - только имитирует фильтрацию.
		CallBackObj.OperationEnd();   //Выведет время выполнения.

		//this->PrintToConsole("Для дополнительной отладки сохраняю файл: output_stupid.csv\n");
		//if (!medFilter.DestSaveToCSVFile("output_stupid.csv", &errObj))
		//{
		//	this->PrintToConsole("Ошибка: " + errObj.errorText_ + "\n");
		//	return 1;
		//};

		//Вроде всё ок, можно сохранять.
		this->PrintToConsole("Готово. Сохраняю файл:\n" + outputFileName + "\n");
		if (!medFilter.SaveImage(outputFileName, &errObj))
		{
			this->PrintToConsole("Ошибка: " + errObj.getErrorText() + "\n");
			return 1;
		};

		this->PrintToConsole("Готово.\n");
	}
	else
	{
		//Что-то пошло не так.
		this->PrintToConsole("Ошибка: " + errObj.getErrorText() + "\n");
		return 1;
	}

	return 0;
}

int AppUIConsole::RunTestMode()
//Метод для запуска в тестовом режиме.
{
	//Тестируем методы проверки строк.
	//float
	PrintToConsole("123 is float: " + STB.BoolToString(STB.CheckFloatStr("123")) + "\n");
	PrintToConsole("-123.0 is float: " + STB.BoolToString(STB.CheckFloatStr("-123.0")) + "\n");
	PrintToConsole("-123,0 is float: " + STB.BoolToString(STB.CheckFloatStr("-123,0")) + "\n");
	PrintToConsole("123.0 is float: " + STB.BoolToString(STB.CheckFloatStr("123.0")) + "\n");
	PrintToConsole("123,0 is float: " + STB.BoolToString(STB.CheckFloatStr("123,0")) + "\n");
	PrintToConsole("123.0 is float: " + STB.BoolToString(STB.CheckFloatStr("123.0")) + "\n");
	PrintToConsole("абырвалг is float: " + STB.BoolToString(STB.CheckFloatStr("абырвалг")) + "\n");
	PrintToConsole("\"\" is float: " + STB.BoolToString(STB.CheckFloatStr("")) + "\n");
	//unsigned int
	PrintToConsole("123.0 is unsInt: " + STB.BoolToString(STB.CheckUnsIntStr("123.0")) + "\n");
	PrintToConsole("-123.0 is unsInt: " + STB.BoolToString(STB.CheckUnsIntStr("-123.0")) + "\n");
	PrintToConsole("123 is unsInt: " + STB.BoolToString(STB.CheckUnsIntStr("123")) + "\n");
	PrintToConsole("-123 is unsInt: " + STB.BoolToString(STB.CheckUnsIntStr("-123")) + "\n");
	PrintToConsole("абырвалг is unsInt: " + STB.BoolToString(STB.CheckUnsIntStr("абырвалг")) + "\n");
	PrintToConsole("\"\" is unsInt: " + STB.BoolToString(STB.CheckUnsIntStr("")) + "\n");
	//signed int
	PrintToConsole("123.0 is int: " + STB.BoolToString(STB.CheckSignedIntStr("123.0")) + "\n");
	PrintToConsole("-123.0 is int: " + STB.BoolToString(STB.CheckSignedIntStr("-123.0")) + "\n");
	PrintToConsole("123 is int: " + STB.BoolToString(STB.CheckSignedIntStr("123")) + "\n");
	PrintToConsole("-123 is int: " + STB.BoolToString(STB.CheckSignedIntStr("-123")) + "\n");
	PrintToConsole("абырвалг is int: " + STB.BoolToString(STB.CheckSignedIntStr("абырвалг")) + "\n");
	PrintToConsole("\"\" is int: " + STB.BoolToString(STB.CheckSignedIntStr("")) + "\n");

//	//Заглушка, работает когда ничего не тестируется.
//	PrintToConsole("Вы кто такие? Я вас не звал! Никаких тестов в этой версии\n\
//всё равно не выполняется ;).\n");
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
	std::cout << this->confObj_->getHelpMsg();
}

} //namespace geoimgconv
