#pragma once

///////////////////////////////////////////////////////////
//                                                       //
//                  GeoImageConverter                    //
//       Преобразователь изображений с геоданными        //
//      Copyright © 2018 Александр (Sagrer) Гриднев      //
//              Распространяется на условиях             //
//                 GNU GPL v3 или выше                   //
//                  см. файл gpl.txt                     //
//                                                       //
///////////////////////////////////////////////////////////

//Место для списка авторов данного конкретного файла (если изменения
//вносили несколько человек).

////////////////////////////////////////////////////////////////////////

#include "call_back_base.h"

namespace geoimgconv
{
	
//Базовый класс для графических фильтров.
class BaseFilter
{
public:
	//Пока тут только несколько полей, которые должны быть в объекте любого фильтра.
		
	//imageSizeX
	virtual int const& getImageSizeX() const = 0;
	//imageSizeY
	virtual int const& getImageSizeY() const = 0;
	//minBlockSize
	virtual unsigned long long const& getMinBlockSize() const = 0;
	//minMemSize
	virtual unsigned long long const& getMinMemSize() const = 0;
	//maxMemSize
	virtual unsigned long long const& getMaxMemSize() const = 0;

	//Метод для применения фильтра - тоже нужен.
	virtual bool ApplyFilter(CallBackBase *callBackObj = NULL, ErrorInfo *errObj = NULL) = 0;
};
	
}	//namespace geoimgconv