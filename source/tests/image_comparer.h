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

//Класс для сравнивания двух файлов с геоданными. Работает через GDAL и AltMatrix.

#include "../errors.h"
#include "../common.h"
#pragma warning(push)
#pragma warning(disable:4251)
#include <gdal_priv.h>
#pragma warning(pop)
#include <boost/cstdint.hpp>
#include "../alt_matrix.h"

namespace geoimgconv
{

class ImageComparer
{
public:
	//Конструкторы-деструкторы.

	//Доступ к полям.

	//Методы всякие.
	
private:
	//Приватные поля
	
};

}	//namespace geoimgconv