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

//Базовый класс для классов-наследников для матриц высот с пикселями разных типов.
//Полиморфизм нужен чтобы выполнять некий минимальный набор действий над этими
//матрицами независимо от типа пикселей.

#include "errors.h"
#include "common.h"
#pragma warning(push)
#pragma warning(disable:4251)
#include <gdal_priv.h>
#pragma warning(pop)

namespace geoimgconv
{

class AltMatrixBase
{
public:
	//Конструкторы-деструкторы.
	AltMatrixBase() {}
	virtual ~AltMatrixBase() {}

	//Доступ к части приватных полей можно сделать и тут, в этом есть смысл и необходимость.

	//xSize
	int const& getXSize() const { return xSize_; }
	void setXSize(const int &xSize) { xSize_ = xSize; }
	//ySize
	int const& getYSize() const { return ySize_; }
	void setYSize(const int &ySize) { ySize_ = ySize; }
	//pixelType
	PixelType const& getPixelType() const { return pixelType_; }

	//Методы всякие. Тут только то, что реально надо было сделать виртуальным. При необходимости этот
	//список вполне можно расширить.

	//Загружает из GDALRasterBand кусочек матрицы высот указанного размера, при этом верхние 2 блока
	//берёт либо из файла либо из нижней части другой (или из себя если дана ссылка на this) матрицы.
	//Особое отношение к этим блокам т.к. первый из них - возможно был последним в прошлом проходе
	//фильтра а второй - был граничным. В новом проходе первый блок будет граничным, второй будет
	//обрабатываться, и его возможно нет смысла второй раз читать из файла.
	//Матрица уже должна иметь достаточный для загрузки размер.
	//gdalRaster - указатель на объект GDALRasterBand исходного изображения.
	//yPosition - начальная строка в исходном изображении, начиная с которой надо читать.
	//yToRead - сколько строк читать.
	//marginSize - высота блока, совпадающая с размером области граничных пикселей. Также это
	//количество пикселей остаётся пустым в начале и конце всех строк матрицы.
	//marginMode - константа из enum TopMarginMode, определяет откуда будут взяты 2 верхних блока.
	//sourceMatrix - ссылка на матрицу из которой берутся блоки в режиме TOP_MM_MATR.
	//errObj - информация об ошибке если она была.
	virtual bool LoadFromGDALRaster(GDALRasterBand *gdalRaster, const int &yPosition, const int &yToRead,
		const int &marginSize, TopMarginMode marginMode, AltMatrixBase *sourceMatrix = NULL,
		ErrorInfo *errObj = NULL) = 0;

	//В соответствии с названием - очищает содержимое объекта чтобы он представлял пустую матрицу.
	virtual void Clear() = 0;

	//Уничтожает содержащуюся в объекте матрицу и создаёт пустую новую указанного размера.
	virtual void CreateEmpty(const int &newX, const int &newY) = 0;

	//Проверить степень совпадения данных данной матрицы и некоей второй. Возвращает число от 0.0 (нет
	//совпадения) до 1.0 (идеальное совпадение). Если матрицы не совпадают по типу и\или размеру - сразу
	//вернёт 0.0.
	virtual double CompareWithAnother(AltMatrixBase *anotherMatr) const = 0;
protected:
	//Эти поля могут менять только потомки.

	//pixelType
	void setPixelType(const PixelType &pixelType) { pixelType_ = pixelType; }
private:
	//Приватные поля
	int xSize_ = 0;	//Размер матрицы по X и Y
	int ySize_ = 0;	//--''--
	PixelType pixelType_ = PIXEL_UNKNOWN;	//Конструктор должен записать сюда значение исходя из типа CellType (типа пикселя в производном классе).
};

}	//namespace geoimgconv