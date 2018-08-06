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

#include <iostream>
#include "strings_tools_box.h"
#include "appui_console_callback.h"

using namespace std;

namespace geoimgconv
{

///////////////////////////////////////
//    Класс AppUIConsoleCallBack     //
///////////////////////////////////////

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
		isPrinted100_ = false;
		lastTextSize_ = 0;
		pixelsPerSecond_ = 0.0;
		updatePeriod_ = DEFAULT_PROGRESS_UPDATE_PERIOD;
	};
}

//"Перерисовать" "прогрессбар".
void AppUIConsoleCallBack::UpdateBar(const unsigned long &progressPosition)
{
	cout << "\r";	//Переводим вывод на начало текущей строки.
	text_ = to_string(progressPosition) + "/" + to_string(getMaxProgress()) +
		" ( " + to_string(progressPosition / (getMaxProgress() / 100)) + "% ) "
		+ StrTB::DoubleToString(pixelsPerSecond_, 2) + " пикс/с, до завершения: "
		+ StrTB::TimeDurationToString(timeLeft_);
	tempSize_ = text_.size();
	if (tempSize_ < lastTextSize_)
	{
		//Надо добавить пробелов чтобы затереть символы предыдущего вывода
		text_.append(" ", lastTextSize_ - tempSize_);
	}
	lastTextSize_ = tempSize_;
	cout << StrTB::Utf8ToConsoleCharset(text_) << std::flush;
}

//CallBack-метод, который будет выводить в консоль количество и процент уже
//обработанных пикселей.
void AppUIConsoleCallBack::CallBack(const unsigned long &progressPosition)
{
	if (progressPosition < getMaxProgress())
	{
		if (skipCounter_ < skipNumber_)
		{
			skipCounter_++;
			return;
		};
		//skipCounter_ будет обнулён позже.

		//Можно выводить.
		if (isStarted_)
		{
			//Cейчас надо проверить сколько времени занял предыдущий период вычислений
			//и если надо - подкрутить skipNumber_. Заодно вычислим количество пикселей в секунду.
			nowTime_ = chrono::time_point_cast<chrono::milliseconds>(chrono::steady_clock::now());
			timeDelta_ = nowTime_ - lastPrintTime_;
			//currMilliseconds_ = timeDelta_.total_milliseconds();
			//Всегда считаем что операция заняла хоть сколько-то времени. Иначе гроб гроб кладбище...
			if (!timeDelta_.count()) timeDelta_ = chrono::milliseconds(10);
			//И только теперь можно считать скорость и всё прочее.
			pixelsPerSecond_ = 1000.0 / (double(timeDelta_.count()) / double(skipCounter_));
			timeLeft_ = chrono::seconds(long(double((getMaxProgress() - progressPosition)) / pixelsPerSecond_));
			if (pixelsPerSecond_ < 1)
				skipNumber_ = size_t(std::ceil(updatePeriod_ / pixelsPerSecond_));
			else
				skipNumber_ = size_t(std::ceil(pixelsPerSecond_*updatePeriod_));
		}
		else
			isStarted_ = true;
		lastPrintTime_ = chrono::time_point_cast<chrono::milliseconds>(
			chrono::steady_clock::now()); //Запомним на будущее :).
		//Выводим.
		UpdateBar(progressPosition);
		//Не забыть обнулить skipCounter_!
		skipCounter_ = 0;
	}
	else
	{
		//Почему-то вышли за 100% :(. Один раз напечатаем что выполнено 100% работы.
		if (!isPrinted100_)
		{
			UpdateBar(getMaxProgress());
			isPrinted100_ = true;
		}
	}
}

//Сообщить объекту о том что операция начинается
void AppUIConsoleCallBack::OperationStart()
{
	//Очистим объект и запомним время начала операции.
	Clear();
	isNotClean_ = true;
	startTime_ = chrono::time_point_cast<chrono::milliseconds>(chrono::steady_clock::now());
}

//Сообщить объекту о том что операция завершена.
void AppUIConsoleCallBack::OperationEnd()
{
	//Запомним время завершения операции и запостим в cout последний
	//вариант прогрессбара и информацию о времени, затраченном на выполнение операции.
	endTime_ = chrono::time_point_cast<chrono::milliseconds>(chrono::steady_clock::now());
	skipCounter_ = 1;
	isStarted_ = false;	//Не даст пересчитать скорость
	cout << "\r";
	CallBack(getMaxProgress());
	cout << StrTB::Utf8ToConsoleCharset("\nЗавершено за: ")
		<< StrTB::TimeDurationToString(endTime_ - startTime_, true) << endl;
}

} //namespace geoimgconv
