#pragma once

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

//Главный класс приложения (консольная версия). Сюда ведёт main(), отсюда программа
//и рулит всем. Здесь юзерский интерфейс и обращение к выполняющим реальную работу
//функциям. Желательно не выпускать исключения выше этого уровня.

//Пока чисто консолька, без curses.

//TODO по-хорошему это всё надо-бы разделить на View и Presenter например (по MVP).

#include <string>
#include "common.h"
#include <chrono>
#include "app_config.h"
#include "small_tools_box.h"
#include "base_filter.h"
#include "median_filter.h"

namespace geoimgconv
{

//Вспомогательный класс для работы с каллбеками из методов класса AppUIConsole.
class AppUIConsoleCallBack : public CallBackBase
{
private:
	//Используемые в этом классе типы.
	using MSecTimePoint = std::chrono::time_point<std::chrono::steady_clock, std::chrono::milliseconds>;
	//Многие приватные поля здесь для того чтобы инициализировать их всего раз
	//а не при каждом вызове CallBack(...). Возможно это немного всё ускорит.
	size_t skipCounter_ = 0;   //Счётчик сколько пикселов уже было пропущено.
	size_t skipNumber_ = 1;	//По сколько пикселов пропускать до следующего пересчёта.
	bool isStarted_ = false;		//Признак того что текст с инфой был уже выведен хотя бы раз.
	bool isNotClean_ = false;	//Clear() очистит объект только если здесь true.
	bool isPrinted100_ = false;	//Признак того что 100% уже напечатано.
	std::string text_;	//Строка для вывода в консоль
	std::string::size_type lastTextSize_ = 0;	//Длина текста, выведенная в прошлый раз.
	std::string::size_type tempSize_;
	MSecTimePoint  lastPrintTime_;	//Для калибровки skipNumber_ чтобы вывод был раз в 2 секунды примерно.
	MSecTimePoint nowTime_;	//---""---
	std::chrono::milliseconds timeDelta_;		//---""---
	std::chrono::seconds timeLeft_;		//Приблизительное время, оставшееся до конца обработки.
	MSecTimePoint startTime_;		//Время начала...
	MSecTimePoint endTime_;		//... и завершения выполнения.
	double pixelsPerSecond_ = 0.0;	//Может быть дробным, что логично ).
	double updatePeriod_ = DEFAULT_PROGRESS_UPDATE_PERIOD;		//Настройка. Раз в сколько секунд обновлять информацию.

	//"Перерисовать" "прогрессбар".
	void UpdateBar(const unsigned long &progressPosition);
public:

	//Доступ к полям.

	//updatePeriod
	double const& getUpdatePeriod() const { return updatePeriod_; }
	void setUpdatePeriod(const double &updatePeriod) { updatePeriod_ = updatePeriod; }

	//Конструктор и деструктор.
	AppUIConsoleCallBack() {}
	~AppUIConsoleCallBack() override {}
	//Возвращает объект в состояние как будто только после инициализации.
	void Clear();
	//CallBack-метод, который будет выводить в консоль количество и процент уже
	//обработанных пикселей.
	void CallBack(const unsigned long &progressPosition) override;
	//Сообщить объекту о том что операция начинается
	void OperationStart() override;
	//Сообщить объекту о том что операция завершена.
	void OperationEnd() override;
};

//Главный класс консольной версии приложения.
class AppUIConsole
{
private:
	//Приватные типы
	enum SwapMode : char
	//Для DetectMaxMemoryCanBeUsed
	{
		SWAPMODE_SILENT_NOSWAP = 0,		//Не использовать swap, не спрашивать
		SWAPMODE_SILENT_USESWAP = 1,	//При необходимости использовать swap, не спрашивать.
		SWAPMODE_ASK = 2				//В случае непоняток - спросить у юзера.
	};

	//Приватные поля
	std::string GDALErrorMsgBuffer;
	AppConfig *confObj_ = nullptr;
	unsigned long long maxMemCanBeUsed_ = 0;	//сюда детектится количество памяти которое можно занимать
	int maxBlocksCanBeUsed_ = 0;	//Сюда детектится максимальное количество блоков, которое можно загружать в память для применяемого сейчас фильтра.
	SysResInfo sysResInfo_;		//Характеристики компа.
	MedianFilterBase *medFilter_ = nullptr;	//Ссылка на текущий объект фильтра. Хранить как поле - удобно чтобы деструктор уничтожил потом объект по этой ссылке.

	//Приватные методы

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
	bool DetectMaxMemoryCanBeUsed(const BaseFilter &filterObj, const SwapMode swapMode = SWAPMODE_ASK,
		ErrorInfo *errObj = NULL);

	//Задетектить инфу для sysResInfo_
	void DetectSysResInfo();

	//Задать юзверю попрос на да\нет и вернуть true если было да и false если было нет.
	bool ConsoleAnsweredYes(const std::string &messageText);

	//Напечатать в консоль сообщение об ошибке.
	void ConsolePrintError(ErrorInfo &errObj);
public:
	//Конструкторы-деструкторы.
	AppUIConsole() {}
	~AppUIConsole();

	//Геттеры-сеттеры
	std::string const& getAppPath() {return confObj_->getAppPath();};
	std::string const& getCurrPath() {return confObj_->getCurrPath();};

	//Готовит приложение к запуску.
	//Внутрь передаётся объект с конфигом, в который уже должны быть прочитаны параметры
	//командной строки.
	void InitApp(AppConfig &conf);

	//По сути тут главный цикл.
	int RunApp();

	//Метод для запуска в тестовом режиме.
	int RunTestMode();

	//Прочий функционал.

	//Вывести сообщение в обычную (не curses) консоль в правильной кодировке.
	void PrintToConsole(const std::string &str);

	//Вывод справки.
	void PrintHelp();
};

} //namespace geoimgconv