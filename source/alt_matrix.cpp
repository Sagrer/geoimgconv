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
#include <typeinfo>
#include <iostream>
#include <cmath>
#include <fstream>
#include "strings_tools_box.h"

using std::string;
using std::cout;
using std::endl;
using std::ios_base;

namespace geoimgconv{

//Делаем инстанциацию для всех вариантов класса, какие легально будет использовать.
template class AltMatrix<double>;
template class AltMatrix<float>;
template class AltMatrix<int8_t>;
template class AltMatrix<uint8_t>;
template class AltMatrix<int16_t>;
template class AltMatrix<uint16_t>;
template class AltMatrix<int32_t>;
template class AltMatrix<uint32_t>;

//--------------------------------//
//   Конструкторы-деструкторы     //
//--------------------------------//

template <typename CellType> AltMatrix<CellType>::AltMatrix(const bool useSignData,
	const bool useQuantedData) : useSignData_(useSignData), useQuantedData_(useQuantedData)
{
	//Объект должен точно знать какого типа у него пиксели.
	if (typeid(CellType) == typeid(double))
		setPixelType(PixelType::Float64);
	else if (typeid(CellType) == typeid(float))
		setPixelType(PixelType::Float32);
	else if (typeid(CellType) == typeid(int8_t))
		setPixelType(PixelType::Int8);
	else if (typeid(CellType) == typeid(uint8_t))
		setPixelType(PixelType::UInt8);
	else if (typeid(CellType) == typeid(int16_t))
		setPixelType(PixelType::Int16);
	else if (typeid(CellType) == typeid(uint16_t))
		setPixelType(PixelType::UInt16);
	else if (typeid(CellType) == typeid(int32_t))
		setPixelType(PixelType::Int32);
	else if (typeid(CellType) == typeid(uint32_t))
		setPixelType(PixelType::UInt32);
	else setPixelType(PixelType::Unknown);
}

template <typename CellType> AltMatrix<CellType>::~AltMatrix()
{
	//new нет в конструкторе, но он мог быть в других методах.
	//Если new не было то там NULL и ничего страшного не случится.
	delete[] (CellType*)data_;
	delete[] signData_;
	delete[] quantedData_;
	delete[] matrixArr_;
	delete[] signMatrixArr_;
	delete[] quantedMatrixArr_;
}

//--------------------------------//
//         Методы всякие          //
//--------------------------------//

//Сохраняет матрицу высот в файл своего внутреннего формата. Вернёт true если всё ок.
template <typename CellType>
bool AltMatrix<CellType>::SaveToFile(const string &fileName, ErrorInfo *errObj) const
{
	//Заглушка.
	if (errObj) errObj->SetError(CommonErrors::FeatureNotReady);
	return false;
}

//Сохраняет в GDALRasterBand кусок матрицы указанного размера и на указанную позицию.
//gdalRaster - указатель на объект GDALRasterBand записываемого изображения.
//yPosition - начальная строка в записываемом изображении, начиная с которой надо писать.
//yToWrite - сколько строк записывать из матрицы.
//errObj - опциональная ссылка на объект для информации об ошибке.
//вернёт эту инфу и false если что-то пойдёт не так.
template <typename CellType>
bool AltMatrix<CellType>::SaveToGDALRaster(GDALRasterBand *gdalRaster, const int &yPosition,
	const int &yToWrite, ErrorInfo *errObj) const
{
	//Среагируем на явные ошибки...
	if (!gdalRaster)
	{
		if (errObj) errObj->SetError(CommonErrors::InternalError, ":  AltMatrix<>::SaveToGDALRaster gdalRaster == NULL");
		return false;
	}
	if ((gdalRaster->GetXSize() != getXSize()) || (gdalRaster->GetYSize() < (yPosition+ yToWrite)))
	{
		if (errObj) errObj->SetError(CommonErrors::InternalError, ":  AltMatrix<>::SaveToGDALRaster wrong sizes!");
		return false;
	}

	//Собсно, запишем информацию в растер.
	CPLErr gdalResult;
	gdalResult = gdalRaster->RasterIO(GF_Write, 0, yPosition, getXSize(), yToWrite,
		(void*)(data_), getXSize(), yToWrite, GICToGDAL_PixelType(getPixelType()),
		0, 0, NULL);
	if (gdalResult != CE_None)
	{
		if (errObj) errObj->SetError(CommonErrors::ReadError, ": " + GetLastGDALError());
		return false;
	}
	
	//Всё ок вроде.
	return true;
}

//Загружает матрицу высот из файла своего формата. Вернёт true если всё ок.
template <typename CellType>
bool AltMatrix<CellType>::LoadFromFile(const string &fileName, ErrorInfo *errObj)
{
	//Заглушка.
	if (errObj) errObj->SetError(CommonErrors::FeatureNotReady);
	return false;
}

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
template <typename CellType>
bool AltMatrix<CellType>::LoadFromGDALRaster(GDALRasterBand *gdalRaster, const int &yPosition, const int &yToRead,
	const int &marginSize, TopMarginMode marginMode, AltMatrixBase *sourceMatrix, ErrorInfo *errObj)
{
	//gdalRaster не может быть NULL.
	if (!gdalRaster)
	{
		if (errObj) errObj->SetError(CommonErrors::InternalError, ":  AltMatrix<>::LoadFromGDALRaster gdalRaster == NULL");
		return false;
	}

	//Указатель на sourceMatrix нужно привести к своему типу, причём смысл в этом есть только при реальном
	//совпадении типов.
	AltMatrix<CellType> *realSourceMatrix = dynamic_cast<AltMatrix<CellType> *>(sourceMatrix);
	if ((realSourceMatrix == nullptr) && (sourceMatrix != nullptr))
	{
		//По идее сюда мы зайдём если реальный тип не был AltMatrix<CellType>.
		//dynamic_cast вернёт NULL если по ссылке было что-то не приводимое.
		if (errObj) errObj->SetError(CommonErrors::InternalError, ":  AltMatrix<>::LoadFromGDALRaster sourceMatrix has wrong real type.");
		return false;
	}
	
	//Первым делом надо или обнулить или скопировать начальную часть матрицы.
	int yStart = 0;		//Строка матрицы на которую надо будет начинать читать из файла. Может измениться ниже.
	switch (marginMode)
	{
	case TopMarginMode::File1:
	{
		//Надо обнулить.
		size_t blockSize = getXSize() * marginSize;
		size_t blockSizeBytes = blockSize * sizeof(CellType);
		memset(data_, 0, blockSizeBytes);
		if (useSignData_)
			memset(signData_, 0, blockSize);
		if (useQuantedData_)
			memset(quantedData_, 0, blockSize * sizeof(uint16_t));
		yStart = marginSize;
	}
		break;
	case TopMarginMode::Matr:
		//Надо скопировать.
		if ((realSourceMatrix == nullptr) ||
			(realSourceMatrix->getXSize() != getXSize()) ||
			(realSourceMatrix->getYSize() != getYSize()))
		{
			if (errObj) errObj->SetError(CommonErrors::InternalError, ": AltMatrix<>::LoadFromGDALRaster wrong sourceMatrix");
			return false;
		}
		//Собственно, копируем 2 блока из конца sourceMatrix в начало this.
		yStart = marginSize * 2;  //это не только yStart но и удвоенная высота блока )). В рассчётах используется и в этом смысле.
		size_t blocksSize = yStart * getXSize();
		size_t blocksSizeBytes = blocksSize * sizeof(CellType);
		size_t blocksQuantedBytes;
		if (useQuantedData_)
			blocksQuantedBytes = blocksSize * sizeof(uint16_t);
		size_t sourceOffset = (getYSize() - yStart)*getXSize();
		size_t sourceOffsetBytes = sourceOffset * sizeof(CellType);
		memcpy(data_, (char*)realSourceMatrix->data_ + sourceOffsetBytes, blocksSizeBytes);
		if (useSignData_ && realSourceMatrix->useSignData_)
			memcpy(signData_, (char*)realSourceMatrix->signData_ + sourceOffset, blocksSize);
		else if (useSignData_)
			memset(signData_, 0, blocksSize);
		if (useQuantedData_ && realSourceMatrix->useQuantedData_)
			memcpy(quantedData_, (uint16_t*)realSourceMatrix->quantedData_ + sourceOffset,
				blocksQuantedBytes);
		else if (useQuantedData_)
			memset(quantedData_, 0, blocksQuantedBytes);
	}

	//В любом случае надо обнулить информацию в следующих блоках.
	size_t blocksToNullSize = (getYSize() - yStart) * getXSize();
	size_t blocksToNullSizeBytes = blocksToNullSize * sizeof(CellType);
	memset(matrixArr_[yStart], 0, blocksToNullSizeBytes);
	if (useSignData_)
		memset(signMatrixArr_[yStart], 0, blocksToNullSize);
	if (useQuantedData_)
		memset(quantedMatrixArr_[yStart], 0, blocksToNullSize*sizeof(uint16_t));

	//Теперь надо прочитать информацию из файла.

	//RasterIO умеет в spacing, поэтому нет необходимости читать картинку построчно для того
	//чтобы добавлять в начале и конце строк пустоту для marginSize. Всё можно загрузить сразу
	//одним вызовом, главное не накосячить в рассчётах, иначе картинка превращается
	//в пиксельное месиво :).
	CPLErr gdalResult;
	int rasterXSize = gdalRaster->GetXSize();
	int rasterYSize = gdalRaster->GetYSize();
	gdalResult = gdalRaster->RasterIO(GF_Read, 0, yPosition, rasterXSize, yToRead,
		(void*)((CellType*)(data_)+yStart * getXSize() + marginSize),
		rasterXSize, yToRead, GICToGDAL_PixelType(getPixelType()), 0,
		(rasterXSize + (marginSize * 2)) * sizeof(CellType), NULL);
	if (gdalResult != CE_None)
	{
		if (errObj) errObj->SetError(CommonErrors::ReadError, ": " + GetLastGDALError());
		return false;
	}

	//Надо понять последнее ли это чтение.
	int yProcessed = yStart+yToRead;
	if (yProcessed <= getYSize())
	{
		//Последнее чтение. Классифицировать придётся меньшую часть пикселей. Для лишней части
		//вспомогательную и основную матрицы надо будет обнулить т.к. там могли остаться значения
		//с прошлых проходов.
		size_t elemsNum = (getYSize() - yProcessed) * getXSize();
		memset((void*)(matrixArr_[yProcessed]), 0, elemsNum * sizeof(CellType));
		if (useSignData_)
			memset((void*)(signMatrixArr_[yProcessed]), 0, elemsNum);
		if (useQuantedData_)
			memset((void*)(quantedMatrixArr_[yProcessed]), 0, elemsNum*sizeof(uint16_t));
	}

	//Осталось пробежаться по всем пикселям и отклассифицировать их на значимые и незначимые.
	//Незначимыми считаем пиксели равные нулю. Граничные надо обнулять безусловно.
	if (useSignData_)
	{
		int i, j;
		for (i = yStart; i < yProcessed; i++)
		{
			for (j = marginSize; j < (getXSize() - marginSize); j++)
			{
				if (matrixArr_[i][j] != CellType(0))
					//Это значимый пиксель.
					signMatrixArr_[i][j] = 1;
				else
					signMatrixArr_[i][j] = 0;
			}
			for (j = 0; j < marginSize; j++)
			{
				signMatrixArr_[i][j] = 0;
			}
			for (j = getXSize() - marginSize; j < getXSize(); j++)
			{
				signMatrixArr_[i][j] = 0;
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
	if (errObj) errObj->SetError(CommonErrors::FeatureNotReady);
	return false;
}

//В соответствии с названием - очищает содержимое объекта чтобы он представлял пустую матрицу.
template <typename CellType> void AltMatrix<CellType>::Clear()
{
	delete[] (CellType*)data_;
	delete[] signData_;
	delete[] quantedData_;
	delete[] matrixArr_;
	delete[] signMatrixArr_;
	delete[] quantedMatrixArr_;
	data_ = NULL;
	signData_ = NULL;
	quantedData_ = NULL;
	matrixArr_ = NULL;
	signMatrixArr_ = NULL;
	quantedMatrixArr_ = NULL;
	setXSize(0);
	setYSize(0);
	dataElemsNum_ = 0;
}

//Уничтожает содержащуюся в объекте матрицу и создаёт пустую новую указанного размера.
template <typename CellType>
void AltMatrix<CellType>::CreateEmpty(const int &newX, const int &newY)
{
	if (!IsClear()) Clear();
	setXSize(newX);
	setYSize(newY);
	dataElemsNum_ = newX * newY;
	//size_t debugElemSize = sizeof(CellType);	//Для отладки (т.к. что тут за CellType в отладчике не видно).

	//Выделяю память под данные, в т.ч. опциональные, расставляю ссылки на строки для быстрого
	//доступа по X и Y.
	//Основные данные.
	data_ = (void*) new CellType[dataElemsNum_]();	//явно инициализовано нулями!
	matrixArr_ = new CellType*[getYSize()];
	int i, j;
	for (i = 0; i < getYSize(); i++)
	{
		j = i * getXSize();
		matrixArr_[i] = &((CellType*)data_)[j];
	}

	//Матрица признаков значимости пикселей.
	if (useSignData_)
	{
		signData_ = new char[dataElemsNum_]();
		signMatrixArr_ = new char*[getYSize()];
		for (i = 0; i < getYSize(); i++)
		{
			j = i * getXSize();
			signMatrixArr_[i] = &signData_[j];
		}
	}

	//Матрица для алгоритма Хуанга.
	if (useQuantedData_)
	{
		quantedData_ = new uint16_t[dataElemsNum_]();
		quantedMatrixArr_ = new uint16_t*[getYSize()];
		for (i = 0; i < getYSize(); i++)
		{
			j = i * getXSize();
			quantedMatrixArr_[i] = &quantedData_[j];
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
	if (sourceMatrix_.IsClear())
	{
		if (!IsClear()) Clear();
		return;
	}
	
	//Если  размеры матриц не совпадают - перевыделим память.
	int newXSize = sourceMatrix_.getXSize() - (marginSize * 2);
	int newYSize = sourceMatrix_.getYSize() - (marginSize * 2);
	if ((newXSize != getXSize()) || (newYSize != getYSize()))
	{
		if (!IsClear()) Clear();
		useSignData_ = false;	//dest-матрице не нужен вспомогательный массив!
		useQuantedData_ = false;  //квантованная матрица тоже не нужна.
		CreateEmpty(sourceMatrix_.getXSize() - (marginSize * 2),
			sourceMatrix_.getYSize() - (marginSize * 2));
	}
	else
	{
		//Совпадает - значит просто обнулим.
		memset(data_, 0, dataElemsNum_ * sizeof(CellType));
		if (useSignData_)
		{
			//Скрипач не нужен
			useSignData_ = false;
			delete[] signData_;
			signData_ = NULL;
			delete[] signMatrixArr_;
			signMatrixArr_ = NULL;
		}
		if (useQuantedData_)
		{
			//Аналогично
			useQuantedData_ = false;
			delete[] quantedData_;
			quantedData_ = NULL;
			delete[] quantedMatrixArr_;
			quantedMatrixArr_ = NULL;
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
	int scale = (int)(ceil(double(getXSize())/79));
	cout << endl;
	for (y = 0; y < getYSize()-scale-1; y+=scale)
	{
		for (x = 0; (x < getXSize()-scale-1)&&(x < 79*scale); x+=scale)
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
		if (errObj) errObj->SetError(CommonErrors::WriteError,": "+fileName);
		return false;
	}
	int x, y;
	for (y = 0; y < getYSize(); y++)
	{
		for (x = 0;x < getXSize(); x++)
		{
			fileStream << x << "," << getYSize() - y << "," << matrixArr_[y][x] << endl;
		}
	}
	fileStream.close();
	return true;
}

//Проверить степень совпадения данных данной матрицы и некоей второй. Возвращает число от 0.0 (нет
//совпадения) до 1.0 (идеальное совпадение). Если матрицы не совпадают по типу и\или размеру - сразу
//вернёт 0.0.
template<typename CellType>
double AltMatrix<CellType>::CompareWithAnother(AltMatrixBase *anotherMatr) const
{
	//Во-первых, другая матрица должна быть.
	if (!anotherMatr)
	{
		return 0.0;
	}

	//Во-вторых она должна быть приводима к типу текущей матрицы.
	AltMatrix<CellType> *anotherMatrix = dynamic_cast<AltMatrix<CellType> *>(anotherMatr);
	if (!anotherMatrix)
	{
		return 0.0;
	}

	//В третьих, размеры матриц должны совпадать.
	if ((getXSize() != anotherMatrix->getXSize()) || (getYSize() != anotherMatrix->getYSize()))
	{
		return 0.0;
	}

	//И только теперь посчитаем сколько пикселей совпадают, а сколько нет.
	std::uint64_t pixelsNum = getXSize()*getYSize();
	std::uint64_t samePixelsNum = 0;
	CellType *i;
	CellType *j;
	for (i = static_cast<CellType*>(data_), j = static_cast<CellType*>(anotherMatrix->data_);
		i < static_cast<CellType*>(data_)+pixelsNum; ++i, ++j)
	{
		if (*i == *j) ++samePixelsNum;
	}
	return ((double)samePixelsNum / (double)pixelsNum);
}

}	//namespace geoimgconv
