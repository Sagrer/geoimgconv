///////////////////////////////////////////////////////////
//                                                       //
//                  GeoImageConverter                    //
//       Преобразователь изображений с геоданными        //
//       Copyright © 2017 Александр (Sagrer) Гриднев     //
//              Распространяется на условиях             //
//                 GNU GPL v3 или выше                   //
//                  см. файл gpl.txt                     //
//                                                       //
///////////////////////////////////////////////////////////

//Место для списка авторов данного конкретного файла (если изменения
//вносили несколько человек).

////////////////////////////////////////////////////////////////////////

//Класс для объектов, в которых будут храниться матрицы высот.

#include "alt_matrix.h"
#include <boost/cstdint.hpp>
#pragma warning(push)
#pragma warning(disable:4251)
#include <gdal_priv.h>
#pragma warning(pop)
#include <boost/filesystem.hpp>
#include <typeinfo>
#include <iostream>
#include <cmath>
#include <fstream>
#include "small_tools_box.h"

using std::string;
using std::cout;
using std::endl;
using std::ios_base;
using namespace boost;

namespace geoimgconv{

//чОрное колдунство!
#pragma warning(push)
#pragma warning(disable:4068)
#pragma GCC push_options
#pragma GCC optimize ("O0")
template <typename CellType> void AntiUnrExtsHelper()
//Дополнение к костылю ниже. Заставляет компиллятор генерить код для всех методов.
{
	AltMatrix<CellType> temp;
	typedef void(AltMatrix<CellType>::*MethodPointer)(...);
	MethodPointer pMethod = reinterpret_cast<MethodPointer>(&AltMatrix<CellType>::SaveToFile);
	pMethod = reinterpret_cast<MethodPointer>(&AltMatrix<CellType>::SaveToGDALFile);
	pMethod = reinterpret_cast<MethodPointer>(&AltMatrix<CellType>::LoadFromFile);
	pMethod = reinterpret_cast<MethodPointer>(&AltMatrix<CellType>::LoadFromGDALFile);
	pMethod = reinterpret_cast<MethodPointer>(&AltMatrix<CellType>::SaveChunkToMatrix);
	pMethod = reinterpret_cast<MethodPointer>(&AltMatrix<CellType>::SaveChunkToFile);
	pMethod = reinterpret_cast<MethodPointer>(&AltMatrix<CellType>::Clear);
	pMethod = reinterpret_cast<MethodPointer>(&AltMatrix<CellType>::IsClear);
	pMethod = reinterpret_cast<MethodPointer>(&AltMatrix<CellType>::PrintStupidVisToCout);
	pMethod = reinterpret_cast<MethodPointer>(&AltMatrix<CellType>::SaveToCSVFile);
	pMethod = reinterpret_cast<MethodPointer>(&AltMatrix<CellType>::CreateDestMatrix);
	pMethod = reinterpret_cast<MethodPointer>(&AltMatrix<CellType>::CreateEmpty);
}

void AntiUnresolvedExternals()
//Функция-костыль. Заставляет компиллятор создать реально используемые
//реализации шаблонного класса без того чтобы добавлять реализацию всех
//методов в хидер. Ну вот не хочу я замусоривать хидер, а список типов, которые
//требуется подставлять в шаблон - фиксирован и известен заранее.
{
	AntiUnrExtsHelper<double>();
	AntiUnrExtsHelper<float>();
	AntiUnrExtsHelper<boost::int8_t>();
	AntiUnrExtsHelper<boost::uint8_t>();
	AntiUnrExtsHelper<boost::int16_t>();
	AntiUnrExtsHelper<boost::uint16_t>();
	AntiUnrExtsHelper<boost::int32_t>();
	AntiUnrExtsHelper<boost::uint32_t>();
}
#pragma GCC pop_options
#pragma warning(pop)
//END OF THE BLACK MAGIC

//--------------------------------//
//   Конструкторы-деструкторы     //
//--------------------------------//

template <typename CellType> AltMatrix<CellType>::AltMatrix() : data_(NULL),
signData_(NULL), xSize_(0), ySize_(0), matrixArr_(NULL), signMatrixArr_(NULL), dataElemsNum_(0)
{
	//Объект должен точно знать какого типа у него пиксели.
	if (typeid(CellType) == typeid(double))
		this->pixelType_ = PIXEL_FLOAT64;
	else if (typeid(CellType) == typeid(float))
		this->pixelType_ = PIXEL_FLOAT32;
	else if (typeid(CellType) == typeid(int8_t))
		this->pixelType_ = PIXEL_INT8;
	else if (typeid(CellType) == typeid(uint8_t))
		this->pixelType_ = PIXEL_UINT8;
	else if (typeid(CellType) == typeid(int16_t))
		this->pixelType_ = PIXEL_INT16;
	else if (typeid(CellType) == typeid(uint16_t))
		this->pixelType_ = PIXEL_UINT16;
	else if (typeid(CellType) == typeid(int32_t))
		this->pixelType_ = PIXEL_INT32;
	else if (typeid(CellType) == typeid(uint32_t))
		this->pixelType_ = PIXEL_UINT32;
	else this->pixelType_ = PIXEL_UNKNOWN;
}

template <typename CellType> AltMatrix<CellType>::~AltMatrix()
{
	//new нет в конструкторе, но он мог быть в других методах
	delete[] (CellType*)this->data_;
	delete[] this->signData_;
	delete[] this->matrixArr_;
	delete[] this->signMatrixArr_;
}

//--------------------------------//
//         Методы всякие          //
//--------------------------------//

template <typename CellType>
bool AltMatrix<CellType>::SaveToFile(const string &fileName, ErrorInfo *errObj) const
//Сохраняет матрицу высот в файл своего внутреннего формата. Вернёт true если всё ок.
{
	//Заглушка.
	if (errObj) errObj->SetError(CMNERR_FEATURE_NOT_READY);
	return false;
}

template <typename CellType>
bool AltMatrix<CellType>::SaveToGDALFile(const std::string &fileName, const int &xStart,
	const int &yStart, ErrorInfo *errObj) const
//Сохраняет матрицу высот в изображение через GDAL. Изображение уже должно существовать,
//т.е. мы просто заменяем там указанную часть пикселей. xStart и yStart задают прямоугольник,
//пикселы из которого будут перезаписаны. T.е. задаёт верхний левый угол в целевом изображении,
//в который попадёт верхний левый угол записываемого изображения. Размер прямоугольника
//определяется размером собственно матрицы, размеры целевого изображения должны ему
//соответствовать. Вернёт true если всё ок.
{
	//А был ли файл?
	filesystem::path filePath = STB.Utf8ToWstring(fileName);
	if (!filesystem::is_regular_file(filePath))
	{
		if (errObj)	errObj->SetError(CMNERR_FILE_NOT_EXISTS, ": " + fileName);
		return false;
	}

	//Открываем файл.
	GDALDataset *inputDataset = (GDALDataset*)GDALOpen(fileName.c_str(), GA_Update);
	if (!inputDataset)
	{
		if (errObj)	errObj->SetError(CMNERR_WRITE_ERROR, ": " + fileName);
		return false;
	}
	if (inputDataset->GetRasterCount() != 1)
	{
		//Если в картинке слоёв не строго 1 штука - это какая-то непонятная картинка,
		//т.е. не похожа на карту высот, возможно там RGB или ещё что подобное...
		//Так что облом и ругаемся.
		GDALClose(inputDataset);
		if (errObj) errObj->SetError(CMNERR_UNSUPPORTED_FILE_FORMAT, ": " + fileName);
		return false;
	}
	GDALRasterBand *inputRaster = inputDataset->GetRasterBand(1);

	//Пишем
	CPLErr gdalResult;
	gdalResult = inputRaster->RasterIO(GF_Write, xStart, yStart, this->xSize_, this->ySize_,
		(void*)(this->data_), this->xSize_, this->ySize_, GICToGDAL_PixelType(this->pixelType_),
		0, 0, NULL);
	if (gdalResult != CE_None)
	{
		GDALClose(inputDataset);
		if (errObj) errObj->SetError(CMNERR_READ_ERROR, ": " + GetLastGDALError());
		return false;
	}

	//Записали вроде.
	GDALClose(inputDataset);
	return true;
}

template <typename CellType>
bool AltMatrix<CellType>::LoadFromFile(const string &fileName, ErrorInfo *errObj)
//Загружает матрицу высот из файла своего формата. Вернёт true если всё ок.
{
	//Заглушка.
	if (errObj) errObj->SetError(CMNERR_FEATURE_NOT_READY);
	return false;
}

template <typename CellType>
bool AltMatrix<CellType>::LoadFromGDALFile(const std::string &fileName,
	const int &marginSize, ErrorInfo *errObj)
//Загружает матрицу высот из изображения через GDAL. Вернёт true если всё ок.
{	
	//Независимо от того удастся ли загрузить файл - данные в объекте
	//должны быть удалены.
	if (this->IsClear()) this->Clear();
	//А был ли файл?
	filesystem::path filePath = STB.Utf8ToWstring(fileName);
	if (!filesystem::is_regular_file(filePath))
	{
		if (errObj)	errObj->SetError(CMNERR_FILE_NOT_EXISTS, ": " + fileName);
		return false;
	}

	//Открываем файл, узнаём размер картинки
	GDALDataset *inputDataset = (GDALDataset*)GDALOpen(fileName.c_str(), GA_ReadOnly);
	if (!inputDataset)
	{
		if (errObj)	errObj->SetError(CMNERR_READ_ERROR, ": " + fileName);
		return false;
	}
	if (inputDataset->GetRasterCount() != 1)
	{
		//Если в картинке слоёв не строго 1 штука - это какая-то непонятная картинка,
		//т.е. не похожа на карту высот, возможно там RGB или ещё что подобное...
		//Так что облом и ругаемся.
		GDALClose(inputDataset);
		if (errObj) errObj->SetError(CMNERR_UNSUPPORTED_FILE_FORMAT, ": " + fileName);
		return false;
	}
	GDALRasterBand *inputRaster = inputDataset->GetRasterBand(1);
	int rasterXSize = inputRaster->GetXSize();
	int rasterYSize = inputRaster->GetYSize();
	if ((rasterXSize <= 0) || (rasterYSize <= 0))
	{
		GDALClose(inputDataset);
		if (errObj) errObj->SetError(CMNERR_UNSUPPORTED_FILE_FORMAT, ": " + fileName);
		return false;
	}
	
	//Выделяем память.
	this->CreateEmpty(rasterXSize+(marginSize * 2), rasterYSize+(marginSize * 2));

	//RasterIO умеет в spacing, поэтому нет необходимости читать картинку построчно для того
	//чтобы добавлять в начале и конце строк пустоту для marginSize. Всё можно загрузить сразу
	//одним вызовом, главное не накосячить в рассчётах, иначе картинка превращается
	//в пиксельное месиво :).
	CPLErr gdalResult;
	gdalResult = inputRaster->RasterIO(GF_Read, 0, 0, rasterXSize, rasterYSize,
		(void*)((CellType*)(this->data_) + marginSize*this->xSize_ + marginSize),
		rasterXSize, rasterYSize, GICToGDAL_PixelType(this->pixelType_), 0,
		(rasterXSize + (marginSize*2))*sizeof(CellType), NULL);
	if (gdalResult != CE_None)
	{
		GDALClose(inputDataset);
		if (errObj) errObj->SetError(CMNERR_READ_ERROR, ": " + GetLastGDALError());
		return false;
	}
	//Главное не забыть закрыть файл!!!
	GDALClose(inputDataset);

	//Осталось пробежаться по всем пикселям и откласифицировать их на значимые и незначимые.
	//Незначимым считаем пиксели равные нулю. Граничные пиксели кстати трогать нет смысла.
	int i, j;
	for (i = marginSize; i < (this->ySize_ - marginSize); i++)
	{
		for (j = marginSize; j < (this->xSize_ - marginSize); j++)
		{
			if (this->matrixArr_[i][j] != CellType(0))
				//Это значимый пиксель.
				this->signMatrixArr_[i][j] = 1;
		}
	}

	//Ок, всё ок ).
	return true;
}

template <typename CellType>
AltMatrix<CellType> &AltMatrix<CellType>::SaveChunkToMatrix(AltMatrix<CellType> &NewMatrix,
	const int &xStart, const int &yStart, const int &xEnd, const int &yEnd) const
//Создаёт в аргументе новую матрицу, вероятно меньшего размера. Старая матрица, если она
//там была - удаляется. В созданную матрицу копируется кусок this-матрицы указанного размера.
//Вернёт тот же объект, что был передан в аргументе. Если операция невозможна, например
//передан бред в размерах - вернёт пустую матрицу.
{
	//Заглушка.
	NewMatrix.Clear();
	return NewMatrix;
}

template <typename CellType>
bool AltMatrix<CellType>::SaveChunkToFile(const string &fileName,
	const int &xStart, const int &yStart, const int &xEnd, const int &yEnd,
	ErrorInfo *errObj) const
//Создаёт и сохраняет на диск в файл своего формата новую матрицу. Старая матрица, если она
//там была - удаляется. Вернёт false если что-то пошло не так.
{
	//Заглушка.
	if (errObj) errObj->SetError(CMNERR_FEATURE_NOT_READY);
	return false;
}

template <typename CellType> void AltMatrix<CellType>::Clear()
//В соответствии с названием - очищает содержимое объекта чтобы он представлял пустую матрицу.
{
	delete[] (CellType*)this->data_;
	delete[] this->signData_;
	delete[] this->matrixArr_;
	delete[] this->signMatrixArr_;
	this->data_ = NULL;
	this->matrixArr_ = NULL;
	this->xSize_ = 0;
	this->ySize_ = 0;
	this->dataElemsNum_ = 0;
}

template <typename CellType>
void AltMatrix<CellType>::CreateEmpty(const int &newX, const int &newY)
//Уничтожает содержащуюся в объекте матрицу и создаёт пустую новую указанного размера.
{
	if (!this->IsClear()) this->Clear();
	this->xSize_ = newX;
	this->ySize_ = newY;
	this->dataElemsNum_ = this->xSize_ * this->ySize_;
	//size_t debugElemSize = sizeof(CellType);	//Для отладки (т.к. что тут за CellType в отладчике не видно).
	this->data_ = (void*) new CellType[this->dataElemsNum_]();	//явно инициализовано нулями!
	this->signData_ = new char[dataElemsNum_]();
	//Массивы указателей для быстрого доступа по координатам X и Y.
	this->matrixArr_ = new CellType*[this->ySize_];
	this->signMatrixArr_ = new char*[this->ySize_];
	int i, j;
	for (i = 0; i < this->ySize_; i++)
	{
		j = i*this->xSize_;
		this->matrixArr_[i] = &((CellType*)this->data_)[j];
		this->signMatrixArr_[i] = &this->signData_[j];
	}
}

template <typename CellType>
void AltMatrix<CellType>::CreateDestMatrix(const AltMatrix<CellType> &sourceMatrix_, const int &marginSize)
//Выделить память под пустую матрицу того же размера что и матрица в аргументе, а затем
//скопировать информацию о значимых пикселях из этой матрицы в новосозданную. Если в
//объекте, в котором был вызван метод уже была некая матрица - она предварительно удаляется.
//marginSize задаёт размер незначимой краевой области в исходной матрице. Целевая
//матрица будет создана без этой области, т.е. меньшего размера.
{
	if (!this->IsClear()) this->Clear();
	if (sourceMatrix_.IsClear()) return;
	//Выделим память.
	this->CreateEmpty(sourceMatrix_.xSize_-(marginSize * 2),
		sourceMatrix_.ySize_-(marginSize * 2));
	//Теперь нужно пройтись по всей вспомогательной матрице и скопировать
	//все равные единице элементы.
	int x, y, sourceX, sourceY;
	for (y=0; y < this->ySize_; y++)
	{
		sourceY = y + marginSize;
		for (x=0; x < this->xSize_; x++)
		{
			sourceX = x + marginSize;
			if (sourceMatrix_.signMatrixArr_[sourceY][sourceX] == 1)
			{
				this->signMatrixArr_[y][x] = 1;
			}
		};
	};
}

template <typename CellType> bool AltMatrix<CellType>::IsClear() const
//Очевидно вернёт true если матрица пуста, либо false если там есть значения.
{
	//!(this->data_==NULL)==true
	return !this->data_;
}

template <typename CellType> void AltMatrix<CellType>::PrintStupidVisToCout() const
//"Тупая" визуализация матрицы, отправляется прямо в cout.
{
	//Вариант с выводом кракозябликов-псевдографики из всего двух
	//разных символов, т.е. показывает значимые и незначимые пиксели.
	//Картинка масштабируется так чтобы влезть в 80 символов - ширину
	//консоли в windows.
	CellType accum;
	int x, y, i, j;
	int scale = (int)(ceil(double(this->xSize_)/79));
	cout << endl;
	for (y = 0; y < this->ySize_-scale-1; y+=scale)
	{
		for (x = 0; (x < this->xSize_-scale-1)&&(x < 79*scale); x+=scale)
		{
			accum = CellType();		//обнуление
			for (i = 0; i < scale; i++)
				for (j = 0; j < scale; j++)
				{
					accum += this->matrixArr_[y + j][x + i];
				}
			if (accum == 0) cout << '*';
			else cout << '#';
		}
		cout << '!' << endl;
	}
}

template <typename CellType>
bool AltMatrix<CellType>::SaveToCSVFile(const std::string &fileName, ErrorInfo *errObj) const
//Вывод матрицы в csv-файл, который должны понимать всякие картографические
//программы. Это значит что каждый пиксел - это одна строка в файле.
//Это "тупой" вариант вывода - метаданные нормально не сохраняются.
{
	std::fstream fileStream;
	fileStream.open(fileName.c_str(),ios_base::binary|ios_base::trunc|ios_base::out);
	if (!fileStream.is_open())
	{
		if (errObj) errObj->SetError(CMNERR_WRITE_ERROR,": "+fileName);
		return false;
	}
	int x, y;
	for (y = 0; y < this->ySize_; y++)
	{
		for (x = 0;x < this->xSize_; x++)
		{
			fileStream << x << "," << this->ySize_ - y << "," << this->matrixArr_[y][x] << endl;
		}
	}
	fileStream.close();
	return true;
};

}	//namespace geoimgconv