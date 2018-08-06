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

//Наследник MedianFilterBase, реализующий "тупой" медианную фильтрацию
//алгоритмом Хуанга. Результат записывается в выбранный destFile

#include "median_filter_base.h"

namespace geoimgconv
{

class MedianFilterHuang : public MedianFilterBase
{
public:
	//Конструктор по умолчанию. Другие использовать нельзя.
	MedianFilterHuang(uint16_t levelsNum) : MedianFilterBase(true, levelsNum) {}
	//Обработать изображение медианным фильтром по алгоритму Хуанга
	bool ApplyFilter(CallBackBase *callBackObj = NULL, ErrorInfo *errObj = NULL) override;
};

}	//namespace geoimgconv