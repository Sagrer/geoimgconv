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

#include "median_filter_stub.h"

using namespace std;

namespace geoimgconv
{
	
////////////////////////////////////////
//          MedianFilterStub          //
////////////////////////////////////////

//Применить "никакой" медианный фильтр.
bool MedianFilterStub::ApplyFilter(CallBackBase *callBackObj, ErrorInfo *errObj)
{
	//Проброс вызова.
	if (getSourceIsAttached() && getDestIsAttached())
	{
		return getFilterObj().ApplyStubFilter(callBackObj, errObj);
	}
	else
	{
		if (errObj) errObj->SetError(CommonErrors::InternalError, ": MedianFilterBase::ApplyStubFilter no source and\\or dest \
file(s) were attached.");
		return false;
	}
}

} //namespace geoimgconv