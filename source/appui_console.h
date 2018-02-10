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

//Главный класс приложения. Сюда ведёт main(), отсюда программа и рулит всем.
//Здесь юзерский интерфейс и обращение к выполняющим реальную работу функциям.
//Желательно не выпускать исключения выше этого уровня.

//Пока чисто консолька, без curses.

#include <string>
#include "common.h"
#include <boost/date_time/posix_time/posix_time.hpp>
#include "app_config.h"

namespace geoimgconv
{

class AppUIConsoleCallBack : public CallBackBase
//Вспомогательный класс для работы с каллбеками из методов класса AppUIConsole.
{
private:
	//Многие приватные поля здесь для того чтобы инициализировать их всего раз
	//а не при каждом вызове CallBack(...). Возможно это немного всё ускорит.
	size_t skipCounter_;   //Счётчик сколько пикселов уже было пропущено.
	size_t skipNumber_;	//По сколько пикселов пропускать до следующего пересчёта.
	bool isStarted_;		//Признак того что текст с инфой был уже выведен хотя бы раз.
	bool isNotClean_;	//Clear() очистит объект только если здесь true.
	std::string text_;	//Строка для вывода в консоль
	std::string::size_type lastTextSize_;	//Длина текста, выведенная в прошлый раз.
	std::string::size_type tempSize_;
	boost::posix_time::ptime lastPrintTime_;	//Для калибровки skipNumber_ чтобы вывод был раз в 2 секунды примерно.
	boost::posix_time::ptime nowTime_;	//---""---
	boost::posix_time::time_duration timeDelta_;		//---""---
	boost::posix_time::ptime startTime_;		//Время начала...
	boost::posix_time::ptime endTime_;		//... и завершения выполнения.
	double pixelsPerSecond_;	//Может быть дробным, что логично ).
	boost::posix_time::time_duration::tick_type currMilliseconds_;
	double updatePeriod_;		//Настройка. Раз в сколько секунд обновлять информацию.

public:

	//Доступ к полям.

	//updatePeriod
	double const& getUpdatePeriod() const { return updatePeriod_; }
	void setUpdatePeriod(const double &updatePeriod) { updatePeriod_ = updatePeriod; }

	//Переопределяем конструктор.
	AppUIConsoleCallBack();
	//Возвращает объект в состояние как будто только после инициализации.
	void Clear();
	//CallBack-метод, который будет выводить в консоль количество и процент уже
	//обработанных пикселей.
	void CallBack(const unsigned long &progressPosition);
	//Сообщить объекту о том что операция начинается
	void OperationStart();
	//Сообщить объекту о том что операция завершена.
	void OperationEnd();
};

class AppUIConsole
{
private:
	std::string GDALErrorMsgBuffer;
	AppConfig *confObj_;
public:
	//Конструкторы-деструкторы.
	AppUIConsole();
	~AppUIConsole();

	//Геттеры-сеттеры
	std::string const& getAppPath() {return this->confObj_->getAppPath();};
	std::string const& getCurrPath() {return this->confObj_->getCurrPath();};

	//Готовит приложение к запуску.
	//Внутрь передаётся объект с конфигом, в который уже должны быть прочитаны параметры
	//командной строки.
	void InitApp(AppConfig &conf);

	//По сути тут главный цикл.
	int RunApp();

	//Прочий функционал.

	//Вывести сообщение в обычную (не curses) консоль в правильной кодировке.
	void PrintToConsole(const std::string &str);

	//Вывод справки.
	void PrintHelp();
};

} //namespace geoimgconv