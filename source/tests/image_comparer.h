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
#include "../alt_matrix.h"
#include <string>
#include <memory>

namespace geoimgconv
{

class ImageComparer
{
public:
	//Конструкторы-деструкторы.
	//Пока остаются по умолчанию.

	//Доступ к полям.

	//Методы всякие.

	//Загружает 2 GeoTIFF-картинки и сравнивает их содержимое. Если у них не совпадает тип
	//или размер, либо хотя-бы одну из картинок загрузить невозможно - сразу возвращает
	//0.0. Если совпадает всё вплоть до значения пикселей - вернёт 1.0. Если пиксели совпадают
	//не все - вернёт число от 0.0 до 1.0, соответствующее количеству совпавших пикселей.
	//Если вернулся 0.0 - стоит проверить наличие ошибки в errObj.
	static double CompareGeoTIFF(const std::string &fileOne, const std::string &fileTwo,
		ErrorInfo *errObj = nullptr);
private:
	//Приватные поля

	//Приватные методы.

	//Создаёт объект-матрицу и грузит в неё картинку. Возвращает умный указатель на объект.
	//Реальный тип объекта (указатель будет на базовый) зависит от содержимого картинки.
	//Если была ошибка - вернёт указатель на nullptr.
	static std::unique_ptr<AltMatrixBase> LoadMatrFromGeoTIFF(const std::string &fileName,
		ErrorInfo *errObj = nullptr);
};

}	//namespace geoimgconv