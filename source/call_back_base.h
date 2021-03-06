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

//Абстрактный класс для организации вызова калбеков. Можно было бы передавать просто
//ссылки на функцию, но тогда эти функции будут отвязаны от данных потока в котором
//должны выполняться. Поэтому имхо проще передать вместо ссылки на функцию ссылку на
//объект, в котором есть и вызываемый метод и привязанные к этому вызову данные.
//Данный класс - абстрактный, от него нужно унаследовать конкретную реализацию с нужным
//функционалом.

namespace geoimgconv
{

class CallBackBase
{	
public:		
	CallBackBase() {};
	virtual ~CallBackBase() {};

	//Доступ к полям

	unsigned long const& getMaxProgress() const { return maxProgress_; }
	void setMaxProgress(const unsigned long &maxProgress) { maxProgress_ = maxProgress; }

	//Собсно каллбек.
	virtual void CallBack(const unsigned long &progressPosition)=0;
	//Сообщить объекту о том что операция начинается
	virtual void OperationStart()=0;
	//Сообщить объекту о том что операция завершена.
	virtual void OperationEnd()=0;

private:
	unsigned long maxProgress_ = 100;	//Значение при котором прогресс считается 100%
};

}	//namespace geoimgconv