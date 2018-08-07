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

//Вспомогательный класс для работы с каллбеками из методов класса AppUIConsole.

#include <string>
#include "common.h"
#include <chrono>
#include "call_back_base.h"

namespace geoimgconv
{

class AppUIConsoleCallBack : public CallBackBase
{
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
};

} //namespace geoimgconv