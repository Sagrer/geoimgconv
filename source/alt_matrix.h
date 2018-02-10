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

//Класс для объектов, в которых будут храниться матрицы высот.

#include "errors.h"
#include "common.h"

namespace geoimgconv
{

template <typename CellType> class AltMatrix
{
private:
	//Приватные поля
	int dataElemsNum_;
	void *data_;		//Ссылка на сырые данные. Обращаться лучше через matrixArr_ - это тот же кусок памяти.
	char *signData_;	//Ссылка на вспомогательный массив. Каждый байт соответствует пикселу в *data_ и
						//содержит код типа пиксела. 0 - неизвестно, 1 - значимий пиксел, 2 - пиксел был
						//заполнен значением для нормальной работы фильтра, но сам по себе незначим и после
						//завершения фильтрации должен быть опустошён. За пустой незначимый пиксел принимаются
						//пикселы с нулевым значением (в data_ а не в signData_!).
	int xSize_;	//Размер матрицы по X и Y
	int ySize_;	//--''--
	CellType **matrixArr_;	//Двумерный массив для произвольного доступа к элементам по X и Y.
	char **signMatrixArr_;	//то же для произвольного доступа к signData_.
	PixelType pixelType_;	//Конструктор запишет сюда значение исходя из типа CellType.
public:
	//Конструкторы-деструкторы.
	AltMatrix();
	~AltMatrix();

	//Доступ к приватным полям.

	//xSize
	int const& getXSize() const { return xSize_; }
	void setXSize(const int &xSize) { xSize_ = xSize; }
	//ySize
	int const& getYSize() const { return ySize_; }
	void setYSize(const int &ySize) { xSize_ = ySize; }
	//matrixArr
	CellType const& getMatrixElem(const int &yCoord, const int &xCoord) const 
	{
		return matrixArr_[yCoord][xCoord];
	}
	void setMatrixElem(const int &yCoord, const int &xCoord, const CellType &value)
	{
		matrixArr_[yCoord][xCoord] = value;
	}
	//signMatrixArr
	char const& getSignMatrixElem(const int &yCoord, const int &xCoord) const
	{
		return signMatrixArr_[yCoord][xCoord];
	}
	void setSignMatrixElem(const int &yCoord, const int &xCoord, const char &value)
	{
		signMatrixArr_[yCoord][xCoord] = value;
	}

	//Методы всякие.

	//Сохраняет матрицу высот в файл своего внутреннего формата. Вернёт true если всё ок.
	bool SaveToFile(const std::string &fileName, ErrorInfo *errObj = NULL) const;

	//Сохраняет матрицу высот в изображение через GDAL. Изображение уже должно существовать,
	//т.е. мы просто заменяем там указанную часть пикселей. xStart и yStart задают прямоугольник,
	//пикселы из которого будут перезаписаны. T.е. задаёт верхний левый угол в целевом изображении,
	//в который попадёт верхний левый угол записываемого изображения. Размер прямоугольника
	//определяется размером собственно матрицы, размеры целевого изображения должны ему
	//соответствовать. Вернёт true если всё ок.
	bool SaveToGDALFile(const std::string &fileName, const int &xStart, const int &yStart,
		ErrorInfo *errObj = NULL) const;
		
	//Загружает матрицу высот из файла своего формата. Вернёт true если всё ок.
	bool LoadFromFile(const std::string &fileName, ErrorInfo *errObj = NULL);

	//Загружает матрицу высот из изображения через GDAL. Вернёт true если всё ок.
	bool LoadFromGDALFile(const std::string &fileName, const int &marginSize = 0, ErrorInfo *errObj = NULL);

	//Создаёт в аргументе новую матрицу, вероятно меньшего размера. Старая матрица, если она
	//там была - удаляется. В созданную матрицу копируется кусок this-матрицы указанного размера.
	//Вернёт тот же объект, что был передан в аргументе. Если операция невозможна, например
	//передан бред в размерах - вернёт пустую матрицу.
	AltMatrix<CellType> &SaveChunkToMatrix(AltMatrix<CellType> &newMatrix, const int &xStart,
		const int &yStart, const int &xEnd, const int &yEnd) const;

	//Создаёт и сохраняет на диск в файл своего формата новую матрицу. Старая матрица, если она
	//там была - удаляется. Вернёт false если что-то пошло не так.
	bool SaveChunkToFile(const std::string &fileName, const int &xStart,
		const int &yStart, const int &xEnd, const int &yEnd, ErrorInfo *errObj = NULL) const;

	//В соответствии с названием - очищает содержимое объекта чтобы он представлял пустую матрицу.
	void Clear();

	//Уничтожает содержащуюся в объекте матрицу и создаёт пустую новую указанного размера.
	void CreateEmpty(const int &newX, const int &newY);

	//Выделить память под пустую матрицу того же размера что и матрица в аргументе, а затем
	//скопировать информацию о значимых пикселях из этой матрицы в новосозданную. Если в
	//объекте, в котором был вызван метод уже была некая матрица - она предварительно удаляется.
	//marginSize задаёт размер незначимой краевой области в исходной матрице. Целевая
	//матрица будет создана без этой области, т.е. меньшего размера.
	void CreateDestMatrix(const AltMatrix<CellType> &sourceMatrix_, const int &marginSize = 0);

	//Очевидно вернёт true если матрица пуста, либо false если там есть значения.
	bool IsClear() const;

	//"Тупая" визуализация матрицы, отправляется прямо в cout.
	void PrintStupidVisToCout() const;
	
	//Вывод матрицы в csv-файл, который должны понимать всякие картографические
	//программы. Это значит что каждый пиксел - это одна строка в файле.
	//Это "тупой" вариант вывода - метаданные нормально не сохраняются.
	bool SaveToCSVFile(const std::string &fileName, ErrorInfo *errObj = NULL) const;
};

}	//namespace geoimgconv