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
#include "app_config.h"
#include "base_filter.h"
#include "median_filter.h"
#include "appui_console_callback.h"
#include "system_tools_box.h"
#include <memory>

namespace geoimgconv
{

//Главный класс консольной версии приложения.
class AppUIConsole
{
public:
	//Конструкторы-деструкторы.
	AppUIConsole() {}
	~AppUIConsole() {}

	//Геттеры-сеттеры
	std::string const& getAppPath() {return confObj_->getAppPath();};
	std::string const& getCurrPath() {return confObj_->getCurrPath();};

	//Готовит приложение к запуску.
	//Внутрь передаётся объект с конфигом, в который уже должны быть прочитаны параметры
	//командной строки.
	void InitApp(std::unique_ptr<AppConfig> conf);

	//По сути тут главный цикл.
	int RunApp();

	//Метод для запуска в тестовом режиме.
	int RunTestMode();

	//Прочий функционал.

	//Вывести сообщение в обычную (не curses) консоль в правильной кодировке.
	void PrintToConsole(const std::string &str);

	//Вывод справки.
	void PrintHelp();

private:
	//Приватные типы
	enum class SwapMode : char
	{
		//Для DetectMaxMemoryCanBeUsed
		SilentNoswap = 0,		//Не использовать swap, не спрашивать
		SilentUseswap = 1,	//При необходимости использовать swap, не спрашивать.
		Ask = 2				//В случае непоняток - спросить у юзера.
	};

	//Приватные поля
	std::string GDALErrorMsgBuffer;
	std::unique_ptr<AppConfig> confObj_ = nullptr;
	unsigned long long maxMemCanBeUsed_ = 0;	//сюда детектится количество памяти которое можно занимать
	int maxBlocksCanBeUsed_ = 0;	//Сюда детектится максимальное количество блоков, которое можно загружать в память для применяемого сейчас фильтра.
	SysResInfo sysResInfo_;		//Характеристики компа.
	std::unique_ptr<MedianFilterBase> medFilter_ = nullptr;	//Указатель на текущий объект фильтра. Для полиморфизма.

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
	bool DetectMaxMemoryCanBeUsed(const BaseFilter &filterObj, const SwapMode swapMode = SwapMode::Ask,
		ErrorInfo *errObj = NULL);

	//Задетектить инфу для sysResInfo_
	void DetectSysResInfo();

	//Задать юзверю попрос на да\нет и вернуть true если было да и false если было нет.
	bool ConsoleAnsweredYes(const std::string &messageText);

	//Напечатать в консоль сообщение об ошибке.
	void ConsolePrintError(ErrorInfo &errObj);
};

} //namespace geoimgconv