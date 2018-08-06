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

#include "median_filter_huang.h"

using namespace std;

namespace geoimgconv
{
	
//////////////////////////////////////////
//          MedianFilterHuang           //
//////////////////////////////////////////

//Обработать изображение медианным фильтром по алгоритму Хуанга
bool MedianFilterHuang::ApplyFilter(CallBackBase *callBackObj, ErrorInfo *errObj)
{
	//Проброс вызова.
	if (getSourceIsAttached() && getDestIsAttached())
	{
		return getFilterObj().ApplyHuangFilter(callBackObj, errObj);
	}
	else
	{
		if (errObj) errObj->SetError(CommonErrors::InternalError, ": MedianFilterBase::ApplyHuangFilter no source and\\or dest \
file(s) were attached.");
		return false;
	}
}

} //namespace geoimgconv