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

//Наследник MedianFilterBase, реализующий "никакой" фильтр. По сути это просто копирование.
//Может быть полезно для отладки. Результат записывается в выбранный destFile

#include "median_filter_base.h"

namespace geoimgconv
{

class MedianFilterStub : public MedianFilterBase
{
public:
	//Конструктор по умолчанию. Другие использовать нельзя.
	MedianFilterStub() : MedianFilterBase() {}
	//Применить "никакой" медианный фильтр.
	bool ApplyFilter(CallBackBase *callBackObj = nullptr, ErrorInfo *errObj = nullptr) override;
};

}	//namespace geoimgconv