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

//Наследник MedianFilterBase, реализующий "тупой" фильтр. Алгоритм медианной
//фильтрации работает "в лоб". Результат записывается в выбранный destFile

#include "median_filter_base.h"

namespace geoimgconv
{

class MedianFilterStupid : public MedianFilterBase
{
public:
	//Конструктор по умолчанию. Другие использовать нельзя.
	MedianFilterStupid() : MedianFilterBase() {}
	//Применить "тупой" медианный фильтр.
	bool ApplyFilter(CallBackBase *callBackObj = NULL, ErrorInfo *errObj = NULL) override;
};

}	//namespace geoimgconv