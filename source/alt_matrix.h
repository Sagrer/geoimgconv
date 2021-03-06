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
#include "alt_matrix_base.h"
#include <memory>

namespace geoimgconv
{

template <typename CellType> class AltMatrix : public AltMatrixBase
{
public:
	//Конструкторы-деструкторы.
	AltMatrix(const bool useSignData = true, const bool useQuantedData = false);
	//А вот это надо запретить:
	AltMatrix(const AltMatrix&) = delete;
	AltMatrix(AltMatrix&&) = delete;
	AltMatrix& operator=(const AltMatrix&) = delete;
	AltMatrix& operator=(AltMatrix&&) = delete;

	//Доступ к приватным полям.

	//int xSize - от предка.
	//int ySize - от предка.
	//PixelType pixelType - от предка.
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
	//quantedMatrixArr
	uint16_t const& getQuantedMatrixElem(const int &yCoord, const int &xCoord) const
	{
		return quantedMatrixArr_[yCoord][xCoord];
	}
	void setQuantedMatrixElem(const int &yCoord, const int &xCoord,
		const uint16_t &value)
	{
		quantedMatrixArr_[yCoord][xCoord] = value;
	}
	//useSignData
	const bool& getUseSignData() const { return useSignData_; }
	//useQuantedData
	const bool& getUseQuantedData() const { return useQuantedData_; }

	//Методы всякие.

	//Сохраняет матрицу высот в файл своего внутреннего формата. Вернёт true если всё ок.
	bool SaveToFile(const std::string &fileName, ErrorInfo *errObj = NULL) const;

	//Сохраняет в GDALRasterBand кусок матрицы указанного размера и на указанную позицию.
	//gdalRaster - указатель на объект GDALRasterBand записываемого изображения.
	//yPosition - начальная строка в записываемом изображении, начиная с которой надо писать.
	//yToWrite - сколько строк записывать из матрицы.
	//errObj - опциональная ссылка на объект для информации об ошибке.
	//вернёт эту инфу и false если что-то пойдёт не так.
	bool SaveToGDALRaster(GDALRasterBand *gdalRaster, const int &yPosition, const int &yToWrite,
		ErrorInfo *errObj = NULL) const;

	//Загружает матрицу высот из файла своего формата. Вернёт true если всё ок.
	bool LoadFromFile(const std::string &fileName, ErrorInfo *errObj = NULL);

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
	//sourceMatrix - ссылка на матрицу из которой берутся блоки в режиме TopMarginMode::Matr.
	//errObj - информация об ошибке если она была.
	bool LoadFromGDALRaster(GDALRasterBand *gdalRaster, const int &yPosition, const int &yToRead,
		const int &marginSize, TopMarginMode marginMode, AltMatrixBase *sourceMatrix = NULL,
		ErrorInfo *errObj = NULL) override;

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
	void Clear() override;

	//Уничтожает содержащуюся в объекте матрицу и создаёт пустую новую указанного размера.
	void CreateEmpty(const int &newX, const int &newY) override;

	//Выделить память под пустую матрицу того же размера что и матрица в аргументе, а затем при
	//необходимости скопировать информацию о значимых пикселях из этой матрицы в новосозданную.
	//Если в объекте, в котором был вызван метод уже была некая матрица - она предварительно
	//удаляется. marginSize задаёт размер незначимой краевой области в исходной матрице. Целевая
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

	//Проверить степень совпадения данных данной матрицы и некоей второй. Возвращает число от 0.0 (нет
	//совпадения) до 1.0 (идеальное совпадение). Если матрицы не совпадают по типу и\или размеру - сразу
	//вернёт 0.0.
	double CompareWithAnother(AltMatrixBase *anotherMatr) const override;

private:
	//Приватные поля
	int dataElemsNum_ = 0;	//Количество пикселей (не байтов!) в data_
	std::unique_ptr<CellType[]> data_ = nullptr;	//Указатель на сырые данные. Обращаться лучше через matrixArr_ - это тот же кусок памяти.
	std::unique_ptr<char[]> signData_ = nullptr;	//Указатель на вспомогательный массив. Каждый байт соответствует пикселу в *data_ и
						//содержит код типа пиксела. 0 - неизвестно, 1 - значимий пиксел, 2 - пиксел был
						//заполнен значением для нормальной работы фильтра, но сам по себе незначим и после
						//завершения фильтрации должен быть опустошён. За пустой незначимый пиксел принимаются
						//пикселы с нулевым значением (в data_ а не в signData_!).
	std::unique_ptr<uint16_t[]> quantedData_ = nullptr;	//Ссылка на данные "квантованных" пикселей, нужных для работы алгоритма
									//Хуанга. Для 1 и 2-байтных типов пикселя использование этого массива
									//вероятно будет не оптимально, но преждевременная оптимизация тоже
									//не есть хорошо, так что пока я не городил изврат с ещё бОльшим количеством
									//шаблонов и этот массив создаётся в т.ч для матриц с однобайтовыми ячейками.
	//Ниже умные массивы голых указателей т.к. голые указатели расставляются поверх трёх буферов,
	//объявленных выше и, таким образом, ничем не владеют.
	std::unique_ptr<CellType*[]> matrixArr_ = nullptr;	//Двумерный массив для произвольного доступа к элементам по X и Y.
	std::unique_ptr<char*[]> signMatrixArr_ = nullptr;	//то же для произвольного доступа к signData_.
	std::unique_ptr<uint16_t*[]> quantedMatrixArr_ = nullptr;  //то же для произвольного доступа к quantedData_.
	bool useSignData_ = true;	//Использовать ли вспомогательную матрицу. Если false - память под неё не выделяется.
	bool useQuantedData_ = false;  //Аналогично использовать ли quantedData_.
};

}	//namespace geoimgconv