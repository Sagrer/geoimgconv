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

//Обёртка с общей для всех типов пиксела функциональностью. Работать с фильтром надо именно через
//этот класс! Не через RealMedianFilter*-ы! Это базовая абстрактная обёртка, которая не имеет метода
//для собственно применения фильтра.

#include "median_filter_base.h"
#pragma warning(push)
#pragma warning(disable:4251)
#include <gdal_priv.h>
#pragma warning(pop)
#include <boost/filesystem.hpp>
#include "strings_tools_box.h"
#include "real_median_filter.h"

namespace b_fs = boost::filesystem;
using namespace std;

namespace geoimgconv
{

//Константы со значениями по умолчанию см. в common.cpp

////////////////////////////////////////
//          MedianFilterBase          //
////////////////////////////////////////

MedianFilterBase::~MedianFilterBase()
{
	//Возможно создавался объект реального фильтра. Надо удалить.
	delete pFilterObj_;
	pFilterObj_ = NULL;
	//Возможно нужно закрыть подключённые файлы и уничтожить объекты GDAL
	CloseAllFiles();
}

//--------------------------//
//     Приватные методы     //
//--------------------------//

//Вычислить минимальные размеры блоков памяти, которые нужны для принятия решения о
//том сколько памяти разрешено использовать медианному фильтру в процессе своей работы.
void MedianFilterBase::CalcMemSizes()
{
	//Смысл в том, что фильтр обрабатывает картинку, в которой помимо самой картинки содержатся
	//граничные пиксели - сверху, снизу, слева, справа. Они либо генерируются одним из алгоритмов,
	//либо это часть самой картинки, но в данный момент эта часть считается незначимой, т.е. не
	//обрабатывается. Картинки удобно обрабатывать построчно. Ширина и высота этих незначимых "полей"
	//известна из апертуры. Получается что нам надо держать в памяти одновременно 3 блока: строки
	//уже незначимых пикселей сверху значимого блока, затем строки значимого блока (в начале и в конце
	//этих строк тоже будут лежать незначимые пиксели), а затем строки ещё незначимого блока (то что ниже).
	//Это касается матрицы с исходной информацией. Помимо этого нужно держать в памяти и матрицу
	//с результатами обработки. Поэтому здесь вычисляется во-первых минимальный разме блока с
	//исходной информацией, во-вторых минимальный размер блока с результатами, в третьих
	//минимальное количество памяти, при наличии которого фильтр вообще сможет работать.
	//Всё в байтах. Минимальные размеры блоков исходных и результирующих данных можно
	//сложить т.к. память всегда будет выделяться для того и другого одновременно.Эти значения
	//позволят некоему внешнему коду, вообще ничего не знающему о том, как работает данный
	//конкретный фильтр, вичислить количество блоков, обрабатываемых за раз так, чтобы влезть
	//в некий ограниченный лимит памяти.

	//В общем, считаем:
	unsigned long long firstLastBlockHeight = (aperture_ - 1) / 2;
	unsigned long long blockHeight;
	if (firstLastBlockHeight > imageSizeY_)
		blockHeight = imageSizeY_;
	else
		blockHeight = firstLastBlockHeight;
	unsigned long long blockWidth = imageSizeX_ + (2 * firstLastBlockHeight);
	//Надо также учесть размер массивов указателей.
	unsigned long long matrixArrBlockSize = sizeof(void*) * blockHeight;
	//Размер пиксела в исходном блоке в байтах у нас складывается из размера типа
	//элементов в матрице и размера элемента вспомогательной матрицы (это 1 байт).
	//Плюс возможно сюда добавятся структуры данных для алгоритма Хуанга.
	unsigned long long additionalBytesPerElem = 0;
	unsigned char matrixArrBlocksNum = 2;
	if (useHuangAlgo_)
	{
		//Если используется алгоритм Хуанга - потребуется дополнительная матрица с элементами
		//по 2 байта. matrixArrBlockSize будет нужно тоже на 1 больше.
		additionalBytesPerElem = 2;
		++matrixArrBlocksNum;
	}
	//Итого, размер исходного блока:
	unsigned long long minSourceBlockSize = (blockHeight * blockWidth * (dataTypeSize_ + 1 +
		additionalBytesPerElem)) + matrixArrBlockSize * matrixArrBlocksNum;
	//Важный момент - поскольку GDAL в момент чтения инфы может аллоцировать памяти от 0 до 100%
	//от объёма памяти, нужного под dest-матрицу - умножим на коэффициент размер dest-блока. И позже
	//при вычислении максимального расхода памяти это тоже учтём. Может конечно получиться такое, что
	//программа будет потреблять меньше памяти чем могла бы при сохранении эффективности, но неиллюзорная
	//опасность выйти за пределы адресного пространства на 32 битах гораздо опаснее, да и в своппинг уйти
	//тоже не сильно весело.
	long double destMatrCoeff = 2.3;
	//Размер блока с результатом
	unsigned long long minDestBlockSize = (imageSizeX_ * blockHeight * dataTypeSize_) +
		matrixArrBlockSize * 2;
	minDestBlockSize = (unsigned long long)((long double)minDestBlockSize * destMatrCoeff);
	//Минимальное допустимое количество памяти.
	minMemSize_ = (3 * minSourceBlockSize) + minDestBlockSize;
	//Для алгоритма Хуанга - надо ещё учесть размер гистограммы.
	if (useHuangAlgo_)
	{
		minMemSize_ += (huangLevelsNum_ * sizeof(unsigned long));
	}
	else
	{
		//Если это stupid-алгоритм то там в свою очередь имеет значение размер массива для пикселей окна.
		minMemSize_ += (aperture_ * aperture_ * dataTypeSize_);
	}
	//Обобщённый размер "блока", содержащего и исходные данные и результат.
	minBlockSize_ = minSourceBlockSize + minDestBlockSize;
	//Общее количество памяти, которое может потребоваться для для работы над изображением.
	maxMemSize_ = (2 * minSourceBlockSize) +		//2 граничных source-блока
		(unsigned long long)((long double)(imageSizeX_ * imageSizeY_ * dataTypeSize_) * destMatrCoeff) + //dest-матрица
		(blockWidth * imageSizeY_ * (dataTypeSize_ + 1 + additionalBytesPerElem));	//остальная source-матрица.

}

//--------------------------//
//      Прочие методы       //
//--------------------------//

//Выбрать исходный файл для дальнейшего чтения и обработки. Получает информацию о параметрах изображения,
//запоминает её в полях объекта.
bool MedianFilterBase::OpenInputFile(const string &fileName, ErrorInfo *errObj)
{
	//TODO: когда избавлюсь от Load и Save - тут должны будут создаваться объекты GDAL, существующие до
	//завершения работы с файлом. Чтобы не создавать их постоянно в процессе по-кусочечной обработки
	//картинки.

	//TODO:вот эта проверка временная! Нужна только пока используется load и save!
	if ((sourceIsAttached_) && (fileName == sourceFileName_))
		return true;

	if (sourceIsAttached_)
	{
		//Нельзя открывать файл если старый не был закрыт.
		if (errObj) errObj->SetError(CommonErrors::InternalError, ": MedianFilterBase::OpenInputFile() попытка открыть файл при не закрытом старом." );
		return false;
	}

	//А был ли файл?
	b_fs::path filePath = StrTB::Utf8ToWstring(fileName);
	if (!b_fs::is_regular_file(filePath))
	{
		if (errObj)	errObj->SetError(CommonErrors::FileNotExists, ": " + fileName);
		return false;
	}

	//Открываем картинку, определяем тип и размер данных.
	gdalSourceDataset_ = (GDALDataset*)GDALOpen(fileName.c_str(), GA_ReadOnly);
	if (!gdalSourceDataset_)
	{
		if (errObj) errObj->SetError(CommonErrors::ReadError, ": " + fileName);
		return false;
	}
	if (gdalSourceDataset_->GetRasterCount() != 1)
	{
		//Если в картинке слоёв не строго 1 штука - это какая-то непонятная картинка,
		//т.е. не похожа на карту высот, возможно там RGB или ещё что подобное...
		//Так что облом и ругаемся.
		GDALClose(gdalSourceDataset_);
		gdalSourceDataset_ = NULL;
		if (errObj) errObj->SetError(CommonErrors::UnsupportedFileFormat, ": " + fileName);
		return false;
	}
	gdalSourceRaster_ = gdalSourceDataset_->GetRasterBand(1);
	dataType_ = GDALToGIC_PixelType(gdalSourceRaster_->GetRasterDataType());
	imageSizeX_ = gdalSourceRaster_->GetXSize();
	imageSizeY_ = gdalSourceRaster_->GetYSize();
	if (dataType_ == PixelType::Unknown)
	{
		GDALClose(gdalSourceDataset_);
		gdalSourceDataset_ = NULL;
		gdalSourceRaster_ = NULL;
		if (errObj) errObj->SetError(CommonErrors::UnsupportedFileFormat, ": " + fileName);
		return false;
	}

	//Если GDAL говорит что тип пикселя - байт - надо посмотреть метаданные какой именно там
	//тип байтов.
	if (dataType_ == PixelType::Int8)
	{
		auto tempStr = gdalSourceRaster_->GetMetadataItem("PIXELTYPE", "IMAGE_STRUCTURE");
		if ((tempStr == NULL) || strcmp(tempStr, "SIGNEDBYTE"))
		{
			//Это явно не signed-байт
			dataType_ = PixelType::UInt8;
		}
	}

	//Тип данных определён, надо создать вложенный объект нужного типа и дальше все вызовы транслировать
	//уже в него.
	delete pFilterObj_;
	switch (dataType_)
	{
		case PixelType::Int8:
			pFilterObj_ = new RealMedianFilterInt8(this);
			dataTypeSize_ = sizeof(int8_t);
			break;
		case PixelType::UInt8:
			pFilterObj_ = new RealMedianFilterUInt8(this);
			dataTypeSize_ = sizeof(uint8_t);
			break;
		case PixelType::Int16:
			pFilterObj_ = new RealMedianFilterInt16(this);
			dataTypeSize_ = sizeof(int16_t);
			break;
		case PixelType::UInt16:
			pFilterObj_ = new RealMedianFilterUInt16(this);
			dataTypeSize_ = sizeof(uint16_t);
			break;
		case PixelType::Int32:
			pFilterObj_ = new RealMedianFilterInt32(this);
			dataTypeSize_ = sizeof(int32_t);
			break;
		case PixelType::UInt32:
			pFilterObj_ = new RealMedianFilterUInt32(this);
			dataTypeSize_ = sizeof(uint32_t);
			break;
		case PixelType::Float32:
			pFilterObj_ = new RealMedianFilterFloat32(this);
			dataTypeSize_ = sizeof(float);
			break;
		case PixelType::Float64:
			pFilterObj_ = new RealMedianFilterFloat64(this);
			dataTypeSize_ = sizeof(double);
			break;
		default:
			pFilterObj_ = NULL;
			dataTypeSize_ = 0;
			break;
	}
	if (pFilterObj_)
	{
		sourceIsAttached_ = true;
		currPositionY_ = 0;
		sourceFileName_ = fileName;
		CalcMemSizes();
		return true;
	}
	else
	{
		if (errObj) errObj->SetError(CommonErrors::UnknownError, "MedianFilterBase::OpenInputFile() error creating pFilterObj_!",true);
		GDALClose(gdalSourceDataset_);
		gdalSourceDataset_ = NULL;
		gdalSourceRaster_ = NULL;
		return false;
	}
}

//Подготовить целевой файл к записи в него результата. Если forceRewrite==true - перезапишет уже
//существующий файл. Иначе вернёт ошибку (false и инфу в errObj). Input-файл уже должен быть открыт.
bool MedianFilterBase::OpenOutputFile(const string &fileName, const bool &forceRewrite, ErrorInfo *errObj)
{
	//Этот метод можно вызывать только если исходный файл уже был открыт.
	if (!sourceIsAttached_)
	{
		if (errObj) errObj->SetError(CommonErrors::InternalError, ":  MedianFilterBase::OpenOutputFile исходный файл не был открыт.");
		return false;
	}
	//И только если файл назначение открыт наоборот ещё не был.
	if (destIsAttached_)
	{
		if (errObj) errObj->SetError(CommonErrors::InternalError, ":  MedianFilterBase::OpenOutputFile попытка открыть файл назначения при уже открытом файле назначения.");
		return false;
	}

	//Готовим пути.
	b_fs::path destFilePath, sourceFilePath;
	destFilePath = StrTB::Utf8ToWstring(fileName);
	sourceFilePath = StrTB::Utf8ToWstring(sourceFileName_);

	//Проверяем есть ли уже такой файл.
	boost::system::error_code errCode;
	if (b_fs::exists(destFilePath, errCode))
	{
		if (!forceRewrite)
		{
			//Запрещено перезаписывать файл, а он существует. Это печально :(.
			if(errObj) errObj->SetError(CommonErrors::FileExistsAlready, ": "+ fileName);
			return false;
		}
		//Удаляем старый файл.
		if (!b_fs::remove(destFilePath, errCode))
		{
			if (errObj) errObj->SetError(CommonErrors::WriteError, ": " +
				StrTB::SystemCharsetToUtf8(errCode.message()));
			return false;
		}
	}

	//Всё, файла быть не должно. Копируем исходный файл.
	b_fs::copy_file(sourceFilePath, destFilePath, errCode);
	if (errCode.value())
	{
		if (errObj) errObj->SetError(CommonErrors::WriteError, ": " +
			StrTB::SystemCharsetToUtf8(errCode.message()));
		return false;
	}

	//Теперь открываем в GDAL то что получилось.
	gdalDestDataset_ = (GDALDataset*)GDALOpen(fileName.c_str(), GA_Update);
	if (!gdalDestDataset_)
	{
		if (errObj)	errObj->SetError(CommonErrors::WriteError, ": " + fileName);
		return false;
	}
	gdalDestRaster_ = gdalDestDataset_->GetRasterBand(1);

	//Готово.
	destIsAttached_ = true;
	return true;
}

//Закрыть исходный файл.
void MedianFilterBase::CloseInputFile()
{
	if (sourceIsAttached_)
	{
		GDALClose(gdalSourceDataset_);
		sourceIsAttached_ = false;
		delete pFilterObj_;
		pFilterObj_ = NULL;
	}
	gdalSourceDataset_ = NULL;
	gdalSourceRaster_ = NULL;
}

//Закрыть файл назначения.
void MedianFilterBase::CloseOutputFile()
{
	if (destIsAttached_)
	{
		GDALClose(gdalDestDataset_);
		destIsAttached_ = false;
	}
	gdalDestDataset_ = NULL;
	gdalDestRaster_ = NULL;
}

//Закрыть все файлы.
void MedianFilterBase::CloseAllFiles()
{
	CloseInputFile();
	CloseOutputFile();
}

//Приводит апертуру (длина стороны окна фильтра) к имеющему смысл значению.
void MedianFilterBase::FixAperture()
{
	//Апертура меньше трёх невозможна и не имеет смысла.
	//Апертура больше трёх должна быть нечётной чтобы в любую сторону от текущего пикселя было
	//одинаковое количество пикселей до края окна. Округлять будем в бОльшую сторону.
	if (aperture_ < 3)
		aperture_ = 3;
	else
	{
		if ((aperture_ % 2) == 0)
			aperture_++;
	}
}

//"Тупая" визуализация матрицы, отправляется прямо в cout.
void MedianFilterBase::SourcePrintStupidVisToCout()
{
	//Просто проброс вызова в объект матрицы.
	pFilterObj_->SourcePrintStupidVisToCout();
}

//Вывод исходной матрицы в csv-файл, который должны понимать всякие картографические
//программы. Это значит что каждый пиксел - это одна строка в файле.
//Это "тупой" вариант вывода - метаданные нормально не сохраняются.
bool MedianFilterBase::SourceSaveToCSVFile(const string &fileName, ErrorInfo *errObj)
{
	//Просто проброс вызова в объект матрицы.
	return pFilterObj_->SourceSaveToCSVFile(fileName, errObj);
}

//Аналогично SourceSaveToCSVFile, но для матрицы с результатом.
bool MedianFilterBase::DestSaveToCSVFile(const string &fileName, ErrorInfo *errObj)
{
	//Просто проброс вызова в объект матрицы.
	return pFilterObj_->DestSaveToCSVFile(fileName, errObj);
}

} //namespace geoimgconv