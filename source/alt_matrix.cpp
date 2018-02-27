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

#include "alt_matrix.h"
#include <boost/cstdint.hpp>
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
//Дополнение к костылю ниже. Заставляет компиллятор генерить код для всех методов.
template <typename CellType> void AntiUnrExtsHelper()
{
	AltMatrix<CellType> temp;
	typedef void(AltMatrix<CellType>::*MethodPointer)(...);
	MethodPointer pMethod = reinterpret_cast<MethodPointer>(&AltMatrix<CellType>::SaveToFile);
	pMethod = reinterpret_cast<MethodPointer>(&AltMatrix<CellType>::SaveToGDALFile);
	pMethod = reinterpret_cast<MethodPointer>(&AltMatrix<CellType>::LoadFromFile);
	pMethod = reinterpret_cast<MethodPointer>(&AltMatrix<CellType>::LoadFromGDALFile);
	pMethod = reinterpret_cast<MethodPointer>(&AltMatrix<CellType>::LoadFromGDALRaster);
	pMethod = reinterpret_cast<MethodPointer>(&AltMatrix<CellType>::SaveChunkToMatrix);
	pMethod = reinterpret_cast<MethodPointer>(&AltMatrix<CellType>::SaveChunkToFile);
	pMethod = reinterpret_cast<MethodPointer>(&AltMatrix<CellType>::Clear);
	pMethod = reinterpret_cast<MethodPointer>(&AltMatrix<CellType>::IsClear);
	pMethod = reinterpret_cast<MethodPointer>(&AltMatrix<CellType>::PrintStupidVisToCout);
	pMethod = reinterpret_cast<MethodPointer>(&AltMatrix<CellType>::SaveToCSVFile);
	pMethod = reinterpret_cast<MethodPointer>(&AltMatrix<CellType>::CreateDestMatrix);
	pMethod = reinterpret_cast<MethodPointer>(&AltMatrix<CellType>::CreateEmpty);
}

//Функция-костыль. Заставляет компиллятор создать реально используемые
//реализации шаблонного класса без того чтобы добавлять реализацию всех
//методов в хидер. Ну вот не хочу я замусоривать хидер, а список типов, которые
//требуется подставлять в шаблон - фиксирован и известен заранее.
void AntiUnresolvedExternals()
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

template <typename CellType> AltMatrix<CellType>::AltMatrix(const bool useSignData) : data_(NULL),
signData_(NULL), xSize_(0), ySize_(0), matrixArr_(NULL), signMatrixArr_(NULL), dataElemsNum_(0),
useSignData_(useSignData)
{
	//Объект должен точно знать какого типа у него пиксели.
	if (typeid(CellType) == typeid(double))
		pixelType_ = PIXEL_FLOAT64;
	else if (typeid(CellType) == typeid(float))
		pixelType_ = PIXEL_FLOAT32;
	else if (typeid(CellType) == typeid(int8_t))
		pixelType_ = PIXEL_INT8;
	else if (typeid(CellType) == typeid(uint8_t))
		pixelType_ = PIXEL_UINT8;
	else if (typeid(CellType) == typeid(int16_t))
		pixelType_ = PIXEL_INT16;
	else if (typeid(CellType) == typeid(uint16_t))
		pixelType_ = PIXEL_UINT16;
	else if (typeid(CellType) == typeid(int32_t))
		pixelType_ = PIXEL_INT32;
	else if (typeid(CellType) == typeid(uint32_t))
		pixelType_ = PIXEL_UINT32;
	else pixelType_ = PIXEL_UNKNOWN;
}

template <typename CellType> AltMatrix<CellType>::~AltMatrix()
{
	//new нет в конструкторе, но он мог быть в других методах
	delete[] (CellType*)data_;
	delete[] signData_;
	delete[] matrixArr_;
	delete[] signMatrixArr_;
}

//--------------------------------//
//         Методы всякие          //
//--------------------------------//

//Сохраняет матрицу высот в файл своего внутреннего формата. Вернёт true если всё ок.
template <typename CellType>
bool AltMatrix<CellType>::SaveToFile(const string &fileName, ErrorInfo *errObj) const
{
	//Заглушка.
	if (errObj) errObj->SetError(CMNERR_FEATURE_NOT_READY);
	return false;
}

//Сохраняет матрицу высот в изображение через GDAL. Изображение уже должно существовать,
//т.е. мы просто заменяем там указанную часть пикселей. yStart и yStart задают прямоугольник,
//пикселы из которого будут перезаписаны. T.е. задаёт верхний левый угол в целевом изображении,
//в который попадёт верхний левый угол записываемого изображения. Размер прямоугольника
//определяется размером собственно матрицы, размеры целевого изображения должны ему
//соответствовать. Вернёт true если всё ок.
template <typename CellType>
bool AltMatrix<CellType>::SaveToGDALFile(const std::string &fileName, const int &xStart,
	const int &yStart, ErrorInfo *errObj) const
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
	gdalResult = inputRaster->RasterIO(GF_Write, xStart, yStart, xSize_, ySize_,
		(void*)(data_), xSize_, ySize_, GICToGDAL_PixelType(pixelType_),
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

//Загружает матрицу высот из файла своего формата. Вернёт true если всё ок.
template <typename CellType>
bool AltMatrix<CellType>::LoadFromFile(const string &fileName, ErrorInfo *errObj)
{
	//Заглушка.
	if (errObj) errObj->SetError(CMNERR_FEATURE_NOT_READY);
	return false;
}

//Загружает матрицу высот из изображения через GDAL. Вернёт true если всё ок.
template <typename CellType>
bool AltMatrix<CellType>::LoadFromGDALFile(const std::string &fileName,
	const int &marginSize, ErrorInfo *errObj)
{
	//Независимо от того удастся ли загрузить файл - данные в объекте
	//должны быть удалены.
	if (IsClear()) Clear();
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
	CreateEmpty(rasterXSize+(marginSize * 2), rasterYSize+(marginSize * 2));

	//RasterIO умеет в spacing, поэтому нет необходимости читать картинку построчно для того
	//чтобы добавлять в начале и конце строк пустоту для marginSize. Всё можно загрузить сразу
	//одним вызовом, главное не накосячить в рассчётах, иначе картинка превращается
	//в пиксельное месиво :).
	CPLErr gdalResult;
	gdalResult = inputRaster->RasterIO(GF_Read, 0, 0, rasterXSize, rasterYSize,
		(void*)((CellType*)(data_) + marginSize*xSize_ + marginSize),
		rasterXSize, rasterYSize, GICToGDAL_PixelType(pixelType_), 0,
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
	if (useSignData_)
	{
		int i, j;
		for (i = marginSize; i < (ySize_ - marginSize); i++)
		{
			for (j = marginSize; j < (xSize_ - marginSize); j++)
			{
				if (matrixArr_[i][j] != CellType(0))
					//Это значимый пиксель.
					signMatrixArr_[i][j] = 1;
			}
		}
	}

	//Ок, всё ок ).
	return true;
}

//Загружает из GDALRasterBand кусочек матрицы высот указанного размера, при этом верхние 2 блока
//берёт либо из файла либо из нижней чести другой (или из себя если дана ссылка на this) матрицы.
//Матрица уже должна иметь достаточный для загрузки размер.
//gdalRaster - указатель на объект GDALRasterBand исходного изображения.
//yPosition - начальная строка в исходном изображении, начиная с которой надо читать.
//yToRead - сколько строк читать.
//marginSize - высота блока, совпадающая с размером области граничных пикселей.
//marginMode - константа из enum TopMarginMode, определяет откуда будут взяты 2 верхних блока.
//sourceMatrix - ссылка на матрицу из которой берутся блоки в режиме TOP_MM_MATR.
//errObj - информация об ошибке если она была.
template <typename CellType>
bool AltMatrix<CellType>::LoadFromGDALRaster(GDALRasterBand *gdalRaster, const int &yPosition, const int &yToRead,
	const int &marginSize, TopMarginMode marginMode, AltMatrix<CellType> *sourceMatrix, ErrorInfo *errObj)
{
	//Первым делом надо или обнулить или скопировать начальную часть матрицы.
	int yStart = 0;		//Строка матрицы на которую надо будет начинать читать из файла. Может измениться ниже.
	switch (marginMode)
	{
	case TOP_MM_FILE1:
	{
		//Надо обнулить.
		size_t blockSize = xSize_ * marginSize;
		size_t blockSizeBytes = blockSize * sizeof(CellType);
		memset(data_, 0, blockSizeBytes);
		if (useSignData_)
			memset(signData_, 0, blockSize);
		yStart = marginSize;
	}
		break;
	case TOP_MM_MATR:
		//Надо скопировать.
		if ((sourceMatrix == NULL) ||
			(sourceMatrix->xSize_ != xSize_) ||
			(sourceMatrix->ySize_ != ySize_))
		{
			if (errObj) errObj->SetError(CMNERR_INTERNAL_ERROR, ": AltMatrix<>::LoadFromGDALRaster wrong sourceMatrix");
			return false;
		}
		//Собственно, копируем 2 блока из конца sourceMatrix в начало this.
		yStart = marginSize * 2;  //это не только yStart но и удвоенная высота блока )). В рассчётах используется и в этом смысле.
		size_t blocksSize = yStart * xSize_;
		size_t blocksSizeBytes = blocksSize * sizeof(CellType);
		size_t sourceOffset = (ySize_ - yStart)*xSize_;
		size_t sourceOffsetBytes = sourceOffset * sizeof(CellType);
		memcpy(data_, (char*)sourceMatrix->data_ + sourceOffsetBytes, blocksSizeBytes);
		if (useSignData_ && sourceMatrix->useSignData_)
			memcpy(signData_, (char*)sourceMatrix->signData_ + sourceOffset, blocksSize);
		else if (useSignData_)
			memset(signData_, 0, blocksSize);
	}

	//Теперь надо прочитать информацию из файла.

	//RasterIO умеет в spacing, поэтому нет необходимости читать картинку построчно для того
	//чтобы добавлять в начале и конце строк пустоту для marginSize. Всё можно загрузить сразу
	//одним вызовом, главное не накосячить в рассчётах, иначе картинка превращается
	//в пиксельное месиво :).
	CPLErr gdalResult;
	int rasterXSize = gdalRaster->GetXSize();
	int rasterYSize = gdalRaster->GetYSize();
	gdalResult = gdalRaster->RasterIO(GF_Read, 0, yPosition, rasterXSize, yToRead,
		(void*)((CellType*)(data_)+yStart * xSize_ + marginSize),
		rasterXSize, yToRead, GICToGDAL_PixelType(pixelType_), 0,
		(rasterXSize + (marginSize * 2)) * sizeof(CellType), NULL);
	if (gdalResult != CE_None)
	{
		if (errObj) errObj->SetError(CMNERR_READ_ERROR, ": " + GetLastGDALError());
		return false;
	}

	//Надо понять последнее ли это чтение.
	int yProcessed = yStart+yToRead;
	if (yProcessed <= (ySize_ - marginSize))
	{
		//Последнее чтение. Классифицировать придётся меньшую часть пикселей. Для лишней части
		//вспомогательную и основную матрицы надо будет обнулить т.к. там могли остаться значения
		//с прошлых проходов.
		size_t elemsNum = (ySize_ - yProcessed) * xSize_;
		memset((void*)(matrixArr_[yProcessed]), 0, elemsNum * sizeof(CellType));
		if (useSignData_)
			memset((void*)(signMatrixArr_[yProcessed]), 0, elemsNum);
	}

	//Осталось пробежаться по всем пикселям и отклассифицировать их на значимые и незначимые.
	//Незначимыми считаем пиксели равные нулю. Граничные пиксели кстати трогать нет смысла.
	if (useSignData_)
	{
		int i, j;
		for (i = yStart; i < yProcessed; i++)
		{
			for (j = marginSize; j < (xSize_ - marginSize); j++)
			{
				if (matrixArr_[i][j] != CellType(0))
					//Это значимый пиксель.
					signMatrixArr_[i][j] = 1;
			}
		}
	}

	//Готово.
	return true;
}

//Создаёт в аргументе новую матрицу, вероятно меньшего размера. Старая матрица, если она
//там была - удаляется. В созданную матрицу копируется кусок this-матрицы указанного размера.
//Вернёт тот же объект, что был передан в аргументе. Если операция невозможна, например
//передан бред в размерах - вернёт пустую матрицу.
template <typename CellType>
AltMatrix<CellType> &AltMatrix<CellType>::SaveChunkToMatrix(AltMatrix<CellType> &NewMatrix,
	const int &xStart, const int &yStart, const int &xEnd, const int &yEnd) const
{
	//Заглушка.
	NewMatrix.Clear();
	return NewMatrix;
}

//Создаёт и сохраняет на диск в файл своего формата новую матрицу. Старая матрица, если она
//там была - удаляется. Вернёт false если что-то пошло не так.
template <typename CellType>
bool AltMatrix<CellType>::SaveChunkToFile(const string &fileName,
	const int &xStart, const int &yStart, const int &xEnd, const int &yEnd,
	ErrorInfo *errObj) const
{
	//Заглушка.
	if (errObj) errObj->SetError(CMNERR_FEATURE_NOT_READY);
	return false;
}

//В соответствии с названием - очищает содержимое объекта чтобы он представлял пустую матрицу.
template <typename CellType> void AltMatrix<CellType>::Clear()
{
	delete[] (CellType*)data_;
	delete[] signData_;
	delete[] matrixArr_;
	delete[] signMatrixArr_;
	data_ = NULL;
	matrixArr_ = NULL;
	xSize_ = 0;
	ySize_ = 0;
	dataElemsNum_ = 0;
}

//Уничтожает содержащуюся в объекте матрицу и создаёт пустую новую указанного размера.
template <typename CellType>
void AltMatrix<CellType>::CreateEmpty(const int &newX, const int &newY)
{
	if (!IsClear()) Clear();
	xSize_ = newX;
	ySize_ = newY;
	dataElemsNum_ = xSize_ * ySize_;
	//size_t debugElemSize = sizeof(CellType);	//Для отладки (т.к. что тут за CellType в отладчике не видно).
	data_ = (void*) new CellType[dataElemsNum_]();	//явно инициализовано нулями!
	//Массивы указателей для быстрого доступа по координатам X и Y.
	matrixArr_ = new CellType*[ySize_];
	if (useSignData_)
	{
		//Почти совпадающий код внутри веток if - чтобы не делать кучу проверок
		//useSignData_ в цикле. Так быстрее. Фиг знает отоптимизировал ли бы это компиллятор.
		signData_ = new char[dataElemsNum_]();
		signMatrixArr_ = new char*[ySize_];
		int i, j;
		for (i = 0; i < ySize_; i++)
		{
			j = i * xSize_;
			matrixArr_[i] = &((CellType*)data_)[j];
			signMatrixArr_[i] = &signData_[j];
		}
	}
	else
	{
		signMatrixArr_ = new char*[ySize_];
		int i, j;
		for (i = 0; i < ySize_; i++)
		{
			j = i * xSize_;
			matrixArr_[i] = &((CellType*)data_)[j];
		}
	}
}

//Выделить память под пустую матрицу того же размера что и матрица в аргументе, а затем при
//необходимости скопировать информацию о значимых пикселях из этой матрицы в новосозданную.
//Если в объекте, в котором был вызван метод уже была некая матрица - она предварительно
//удаляется. marginSize задаёт размер незначимой краевой области в исходной матрице. Целевая
//матрица будет создана без этой области, т.е. меньшего размера.
template <typename CellType>
void AltMatrix<CellType>::CreateDestMatrix(const AltMatrix<CellType> &sourceMatrix_, const int &marginSize)
{
	if (!IsClear()) Clear();
	if (sourceMatrix_.IsClear()) return;
	//Выделим память.
	CreateEmpty(sourceMatrix_.xSize_-(marginSize * 2),
		sourceMatrix_.ySize_-(marginSize * 2));

	if (useSignData_)
	{
		//Теперь нужно пройтись по всей вспомогательной матрице и скопировать
		//все равные единице элементы.
		int x, y, sourceX, sourceY;
		for (y = 0; y < ySize_; y++)
		{
			sourceY = y + marginSize;
			for (x = 0; x < xSize_; x++)
			{
				sourceX = x + marginSize;
				if (sourceMatrix_.signMatrixArr_[sourceY][sourceX] == 1)
				{
					signMatrixArr_[y][x] = 1;
				}
			}
		}
	}

}

//Очевидно вернёт true если матрица пуста, либо false если там есть значения.
template <typename CellType> bool AltMatrix<CellType>::IsClear() const
{
	//!(data_==NULL)==true
	return !data_;
}

//"Тупая" визуализация матрицы, отправляется прямо в cout.
template <typename CellType> void AltMatrix<CellType>::PrintStupidVisToCout() const
{
	//Вариант с выводом кракозябликов-псевдографики из всего двух
	//разных символов, т.е. показывает значимые и незначимые пиксели.
	//Картинка масштабируется так чтобы влезть в 80 символов - ширину
	//консоли в windows.
	CellType accum;
	int x, y, i, j;
	int scale = (int)(ceil(double(xSize_)/79));
	cout << endl;
	for (y = 0; y < ySize_-scale-1; y+=scale)
	{
		for (x = 0; (x < xSize_-scale-1)&&(x < 79*scale); x+=scale)
		{
			accum = CellType();		//обнуление
			for (i = 0; i < scale; i++)
				for (j = 0; j < scale; j++)
				{
					accum += matrixArr_[y + j][x + i];
				}
			if (accum == 0) cout << '*';
			else cout << '#';
		}
		cout << '!' << endl;
	}
}

//Вывод матрицы в csv-файл, который должны понимать всякие картографические
//программы. Это значит что каждый пиксел - это одна строка в файле.
//Это "тупой" вариант вывода - метаданные нормально не сохраняются.
template <typename CellType>
bool AltMatrix<CellType>::SaveToCSVFile(const std::string &fileName, ErrorInfo *errObj) const
{
	std::fstream fileStream;
	fileStream.open(fileName.c_str(),ios_base::binary|ios_base::trunc|ios_base::out);
	if (!fileStream.is_open())
	{
		if (errObj) errObj->SetError(CMNERR_WRITE_ERROR,": "+fileName);
		return false;
	}
	int x, y;
	for (y = 0; y < ySize_; y++)
	{
		for (x = 0;x < xSize_; x++)
		{
			fileStream << x << "," << ySize_ - y << "," << matrixArr_[y][x] << endl;
		}
	}
	fileStream.close();
	return true;
};

}	//namespace geoimgconv
