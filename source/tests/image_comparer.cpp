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

#include "image_comparer.h"
#include <boost/filesystem.hpp>
#include "../small_tools_box.h"

using std::string;
using std::unique_ptr;
namespace b_fs = boost::filesystem;

namespace geoimgconv{

//--------------------------------//
//   Конструкторы-деструкторы     //
//--------------------------------//

//--------------------------------//
//         Методы всякие          //
//--------------------------------//

//Загружает 2 GeoTIFF-картинки и сравнивает их содержимое. Если у них не совпадает тип
//или размер, либо хотя-бы одну из картинок загрузить невозможно - сразу возвращает
//0.0. Если совпадает всё вплоть до значения пикселей - вернёт 1.0. Если пиксели совпадают
//не все - вернёт число от 0.0 до 1.0, соответствующее количеству совпавших пикселей.
//Если вернулся 0.0 - стоит проверить наличие ошибки в errObj.
double ImageComparer::CompareGeoTIFF(const string & fileOne, const string & fileTwo, ErrorInfo *errObj)
{
	//Пытаемся прочитать файлы.
	unique_ptr<AltMatrixBase> matrOne(nullptr), matrTwo(nullptr);
	matrOne = LoadMatrFromGeoTIFF(fileOne, errObj);
	if (!matrOne) return 0.0;
	matrTwo = LoadMatrFromGeoTIFF(fileTwo, errObj);
	if (!matrTwo) return 0.0;

	//Сравниваем что получилось.
	return matrOne->CompareWithAnother(matrTwo.get());
}

//Создаёт объект-матрицу и грузит в неё картинку. Возвращает умный указатель на объект.
//Реальный тип объекта (указатель будет на базовый) зависит от содержимого картинки.
//Если была ошибка - вернёт указатель на nullptr.
unique_ptr<AltMatrixBase> ImageComparer::LoadMatrFromGeoTIFF(const string &fileName, ErrorInfo *errObj)
{
	//TODO: метод во многом копипаста из MedianFilterBase::OpenInputFile. Надо придумать как лучше сделать
	//так, чтобы и там и там использовать один и тот же код. Возможно, через базовый класс матрицы.
	//Возможно как-то ещё. Проблема в том что тут надо именно загрузить готовую матрицу а там - матрица
	//грузится в процессе работы медианного фильтра и метод только готовится к её загрузке, собирая
	//инфу о параметрах файла. Т.е. возможно нужен какой-то класс-адаптер для этого. Или вообще пока не
	//заморачиваться, благо данный метод нужен только для тестов.

	//А был ли файл?
	b_fs::path filePath = STB.Utf8ToWstring(fileName);
	if (!b_fs::is_regular_file(filePath))
	{
		if (errObj)	errObj->SetError(CMNERR_FILE_NOT_EXISTS, ": " + fileName);
		return unique_ptr<AltMatrixBase>(nullptr);
	}

	//Открываем картинку, определяем тип и размер данных. Умный указатель тут нельзя,
	//поскольку объектом по указателю управляет GDAL и удаляем мы его тоже через GDAL API.
	GDALDataset *sourceDataset = (GDALDataset*)GDALOpen(fileName.c_str(), GA_ReadOnly);
	if (!sourceDataset)
	{
		if (errObj) errObj->SetError(CMNERR_READ_ERROR, ": " + fileName);
		return unique_ptr<AltMatrixBase>(nullptr);
	}
	if (sourceDataset->GetRasterCount() != 1)
	{
		//Если в картинке слоёв не строго 1 штука - это какая-то непонятная картинка,
		//т.е. не похожа на карту высот, возможно там RGB или ещё что подобное...
		//Так что облом и ругаемся.
		GDALClose(sourceDataset);
		sourceDataset = nullptr;
		if (errObj) errObj->SetError(CMNERR_UNSUPPORTED_FILE_FORMAT, ": " + fileName);
		return unique_ptr<AltMatrixBase>(nullptr);
	}
	GDALRasterBand *sourceRaster = sourceDataset->GetRasterBand(1);
	PixelType dataType = GDALToGIC_PixelType(sourceRaster->GetRasterDataType());
	int imageSizeX = sourceRaster->GetXSize();
	int imageSizeY = sourceRaster->GetYSize();
	if (dataType == PIXEL_UNKNOWN)
	{
		GDALClose(sourceDataset);
		sourceDataset = nullptr;
		sourceRaster = nullptr;
		if (errObj) errObj->SetError(CMNERR_UNSUPPORTED_FILE_FORMAT, ": " + fileName);
		return unique_ptr<AltMatrixBase>(nullptr);
	}

	//Если GDAL говорит что тип пикселя - байт - надо посмотреть метаданные какой именно там
	//тип байтов.
	if (dataType == PIXEL_INT8)
	{
		const char *tempStr = sourceRaster->GetMetadataItem("PIXELTYPE", "IMAGE_STRUCTURE");
		if ((tempStr == NULL) || strcmp(tempStr, "SIGNEDBYTE"))
		{
			//Это явно не signed-байт
			dataType = PIXEL_UINT8;
		}
	}

	//Тип данных определён, надо создать объект нужного типа и дальше все вызовы транслировать
	//уже в него.
	unique_ptr<AltMatrixBase> result;
	switch (dataType)
	{
	case PIXEL_INT8:
		result.reset(new AltMatrix<boost::int8_t>(false, false));
		break;
	case PIXEL_UINT8:
		result.reset(new AltMatrix<boost::uint8_t>(false, false));
		break;
	case PIXEL_INT16:
		result.reset(new AltMatrix<boost::int16_t>(false, false));
		break;
	case PIXEL_UINT16:
		result.reset(new AltMatrix<boost::uint16_t>(false, false));
		break;
	case PIXEL_INT32:
		result.reset(new AltMatrix<boost::int32_t>(false, false));
		break;
	case PIXEL_UINT32:
		result.reset(new AltMatrix<boost::uint32_t>(false, false));
		break;
	case PIXEL_FLOAT32:
		result.reset(new AltMatrix<float>(false, false));
		break;
	case PIXEL_FLOAT64:
		result.reset(new AltMatrix<double>(false, false));
		break;
	default:
		result.reset(nullptr);
		break;
	}
	if (!result)
	{
		//Не получилось создать матрицу :(
		if (errObj) errObj->SetError(CMNERR_UNKNOWN_ERROR, "ImageComparer::LoadMatrFromGeoTIFF error creating AltMatrix<T>!", true);
		GDALClose(sourceDataset);
		sourceDataset = nullptr;
		sourceRaster = nullptr;
		return unique_ptr<AltMatrixBase>(nullptr);
	}

	//Читаем файл в получившуюся матрицу.
	result->CreateEmpty(imageSizeX, imageSizeY);
	if (!result->LoadFromGDALRaster(sourceRaster, 0, imageSizeY, 0, TOP_MM_FILE1,
		nullptr, errObj))
	{
		//Не удалось загрузить. errObj уже заполнен сообщением об ошибке.
		GDALClose(sourceDataset);
		sourceDataset = nullptr;
		sourceRaster = nullptr;
		return unique_ptr<AltMatrixBase>(nullptr);
	}

	//Готово, возвращаем то что получилось.
	GDALClose(sourceDataset);
	sourceDataset = nullptr;
	sourceRaster = nullptr;
	return result;
}

}	//namespace geoimgconv
