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

//Наследник FilterBase, реализующий "тупой" медианную фильтрацию
//алгоритмом Хуанга. Результат записывается в выбранный destFile

#include "filter_base.h"

namespace geoimgconv
{

class MedianFilterHuang : public FilterBase
{
public:
	//Конструктор по умолчанию. Другие использовать нельзя.
	MedianFilterHuang(uint16_t levelsNum) : FilterBase(true, levelsNum) {}
protected:
	//Создать объект класса, который работает с изображениями с типом пикселей dataType_
	//и соответствующим алгоритмом фильтрации и поместить указатель на него в pFilterObj_.
	//Если в dataType_ лежит что-то неправильное то туда попадёт nullptr. Кроме того, метод
	//должен установить правильный dataTypeSize_.
	void NewFilterObj() override;
};

}	//namespace geoimgconv