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

//Класс для работы с медианным фильтром.

#include "median_filter.h"
#pragma warning(push)
#pragma warning(disable:4251)
#include <gdal_priv.h>
#pragma warning(pop)
#include <boost/filesystem.hpp>
#include <iostream>
#include <algorithm>
#include <cstdlib>
#include "small_tools_box.h"

using namespace boost;
using namespace std;

namespace geoimgconv
{

//Константы для возможной подкрутки параметров.
//TODO: избавиться от дублирования констант в разных файлах!
//Как минимум есть проблема с апертурой.
const size_t DEFAULT_APERTURE = 101;
const double DEFAULT_THRESHOLD = 0.5;
//const MarginType DEFAULT_MARGIN_TYPE = MARGIN_SIMPLE_FILLING;
const MarginType DEFAULT_MARGIN_TYPE = MARGIN_MIRROR_FILLING;

////////////////////////////////////////////
//       RealMedianFilterTemplBase        //
////////////////////////////////////////////

//--------------------------------//
//       Приватные методы         //
//--------------------------------//

template <typename CellType>
bool RealMedianFilterTemplBase<CellType>::PixelStep(int &x, int &y, const PixelDirection direction)
//Сделать шаг по пиксельным координатам в указанном направлении.
//Вернёт false если координаты ушли за границы изображения, иначе true.
{
	if (direction == PIXEL_DIR_UP)
	{
		y--;
	}
	else if (direction == PIXEL_DIR_DOWN)
	{
		y++;
	}
	else if (direction == PIXEL_DIR_RIGHT)
	{
		x++;
	}
	else if (direction == PIXEL_DIR_LEFT)
	{
		x--;
	}
	else if (direction == PIXEL_DIR_UP_RIGHT)
	{
		x++;
		y--;
	}
	else if (direction == PIXEL_DIR_UP_LEFT)
	{
		x--;
		y--;
	}
	else if (direction == PIXEL_DIR_DOWN_RIGHT)
	{
		x++;
		y++;
	}
	else if (direction == PIXEL_DIR_DOWN_LEFT)
	{
		x--;
		y++;
	}
	if ((x >= 0) && (y >= 0) && (x < sourceMatrix_.getXSize()) && (y < sourceMatrix_.getYSize()))
		return true;
	else
		return false;
}

//Получает координаты действительного пикселя, зеркального для
//currXY по отношению к центральному xy. Результат записывается
//в outXY.
template <typename CellType>
void RealMedianFilterTemplBase<CellType>::GetMirrorPixel(const int &x, const int &y, const int &currX, const int &currY, int &outX, int &outY, const int recurseDepth)
{
	int shift = 0;	// это cмещение в сторону центра если пиксель оказался недействительным.
	int delta, firstX, firstY, tempX, tempY;
	bool found = false;
	outX = x;
	outY = y;
	do
	{
		if (x != currX)
		{
			delta = std::abs(x-currX)-shift;
			if ((x-currX)<0) delta = -delta;
			outX=x+delta;
		};
		if (y != currY)
		{
			delta = std::abs(y-currY)-shift;
			if ((y-currY)<0) delta = -delta;
			outY=y+delta;
		};
		if (shift==0)
		{
			//Запоминаем координаты на случай необходимости рекурсивного вызова.
			firstX = outX;
			firstY = outY;
		};
		if ((outX >= 0) && (outX < sourceMatrix_.getXSize()) &&
			(outY >= 0) && (outY < sourceMatrix_.getYSize()))
		{
			if (sourceMatrix_.getSignMatrixElem(outY,outX) == 1)
			{
				//Найден значимый пиксел. В идеале эта ветка должна
				//отрабатывать с первого раза, исключение - если слой
				//значимых пикселей был неглубоким. В крайнем случае
				//цикл остановится при xy==outXY.
				if ((recurseDepth<10)&&(shift!=0))
				{
					//По идее можно остановиться и здесь, но есть вероятность
					//что это создаст горы из неудачно выпавшего на пик одинакового
					//пикселя. Поэтому лучше зеркалить рекурсивно, взяв найденный
					//пиксель за центр а изначально зеркальный (при shift==0) за тот,
					//зеркальный к которому мы ищем.
					tempX=outX;
					tempY=outY;
					GetMirrorPixel(tempX,tempY,firstX,firstY,outX,outY,recurseDepth+1);
				};
				found = true;
			}		
		};
		if (found==false) shift++; //Ничего не найдено - поищем ближе к центру.
	} while(found==false);
};

template <typename CellType>
void RealMedianFilterTemplBase<CellType>::SimpleFiller(const int &x, const int &y,
	const PixelDirection direction, const int &marginSize)
//Заполнять пиксели простым алгоритмом в указанном направлении
{
	int currX = x;
	int currY = y;
	int stepCounter = 0;

	while (PixelStep(currX, currY, direction))
	{
		stepCounter++;
		if((sourceMatrix_.getSignMatrixElem(currY,currX)==1) ||
			(stepCounter > marginSize) )
		{
			//Значимый пиксель, либо ушли за пределы окна. Прекращаем заполнение.
			return;
		}
		else if ((sourceMatrix_.getSignMatrixElem(currY,currX) == 0) ||
			(direction == PIXEL_DIR_UP) || (direction == PIXEL_DIR_DOWN) ||
			(direction == PIXEL_DIR_RIGHT) || (direction == PIXEL_DIR_LEFT))
		{
			//Заполняем либо вообще не заполненные пиксели, либо любые незначимые
			//если двигаемся не по диагонали чтобы возможно переписать те, что были
			//уже заполнены диагональным заполнителем.
			sourceMatrix_.setMatrixElem(currY,currX,sourceMatrix_.getMatrixElem(y,x));
			sourceMatrix_.setSignMatrixElem(currY,currX,2); //Заполненный незначимый пиксель.
		}
	}
}

template <typename CellType>
void RealMedianFilterTemplBase<CellType>::MirrorFiller(const int &x, const int &y,
	const PixelDirection direction, const int &marginSize)
//Заполнять пиксели зеркальным алгоритмом в указанном направлении
{
	int currX = x;
	int currY = y;
	int mirrorX, mirrorY;
	int stepCounter = 0;

	while (PixelStep(currX, currY, direction))
	{
		stepCounter++;
		if((sourceMatrix_.getSignMatrixElem(currY,currX) == 1) ||
			(stepCounter > marginSize) )
		{
			//Значимый пиксель, либо ушли за пределы окна. Прекращаем заполнение.
			return;
		}
		else if ((sourceMatrix_.getSignMatrixElem(currY,currX) == 0) ||
			(direction == PIXEL_DIR_UP) || (direction == PIXEL_DIR_DOWN) ||
			(direction == PIXEL_DIR_RIGHT) || (direction == PIXEL_DIR_LEFT))
		{
			//Заполняем либо вообще не заполненные пиксели, либо любые незначимые
			//если двигаемся не по диагонали чтобы возможно переписать те, что были
			//уже заполнены диагональным заполнителем.
			GetMirrorPixel(x,y,currX,currY,mirrorX,mirrorY);	//Координаты зеркального пикселя.
			sourceMatrix_.setMatrixElem(currY,currX,sourceMatrix_.getMatrixElem(mirrorY,mirrorX));
			sourceMatrix_.setSignMatrixElem(currY,currX,2); //Заполненный незначимый пиксель.
		}
	}
}

template <typename CellType>
void RealMedianFilterTemplBase<CellType>::FillMargins_PixelBasedAlgo(const TFillerMethod FillerMethod,
	CallBackBase *callBackObj)
//Костяк алгоритма, общий для Simple и Mirror
{
	//Двигаемся построчно, пока не найдём значимый пиксель. В границы не лезем,
	//т.к. они значимыми быть не могут.
	int x, y, marginSize;
	marginSize = (getOwnerObj().getAperture() - 1) / 2;
	//Настроим объект, выводящий информацию о прогрессе обработки.
	if (callBackObj)
		callBackObj->setMaxProgress((sourceMatrix_.getXSize() - marginSize*2) *
			(sourceMatrix_.getYSize() - marginSize*2));
	unsigned long progressPosition = 0;
	//Поехали.
	for (y = marginSize; y < (sourceMatrix_.getYSize() - marginSize); y++)
	{
		for (x = marginSize; x < (sourceMatrix_.getXSize() - marginSize); x++)
		{
			progressPosition++;
			if (sourceMatrix_.getSignMatrixElem(y,x) == 1)
			{
				//Теперь будем проверять значимы ли пикселы вверху, внизу, справа, слева и
				//по всем диагоналям. Если незначимы (независимо от фактической заполненности)
				//- запускается процесс заполнения, который будет двигаться и заполнять пиксели
				//в данном направлении пока не достигнет края картинки либо пока не встретит
				//значимый пиксель. При этом вертикальное и горизонтальное заполнение имеет
				//приоритет над диагональным, т.е. диагональные пикселы будут перезаписаны если
				//встретятся на пути. Поехали.
				
				(this->*FillerMethod)(x, y, PIXEL_DIR_UP, marginSize);
				(this->*FillerMethod)(x, y, PIXEL_DIR_DOWN, marginSize);
				(this->*FillerMethod)(x, y, PIXEL_DIR_RIGHT, marginSize);
				(this->*FillerMethod)(x, y, PIXEL_DIR_LEFT, marginSize);
				(this->*FillerMethod)(x, y, PIXEL_DIR_UP_RIGHT, marginSize);
				(this->*FillerMethod)(x, y, PIXEL_DIR_UP_LEFT, marginSize);
				(this->*FillerMethod)(x, y, PIXEL_DIR_DOWN_RIGHT, marginSize);
				(this->*FillerMethod)(x, y, PIXEL_DIR_DOWN_LEFT, marginSize);
				//Сообщение о прогрессе.
				if (callBackObj) callBackObj->CallBack(progressPosition);
			}
		}
	}
}

template <typename CellType>
void RealMedianFilterTemplBase<CellType>::FillMargins_Simple(CallBackBase *callBackObj)
//Заполнить пустые пиксели source-матрицы простым алгоритмом (сплошной цвет).
{
	FillMargins_PixelBasedAlgo(&RealMedianFilterTemplBase<CellType>::SimpleFiller,
		callBackObj);
}

template <typename CellType>
void RealMedianFilterTemplBase<CellType>::FillMargins_Mirror(CallBackBase *callBackObj)
//Заполнить пустые пиксели source-матрицы зеркальным алгоритмом.
{
	FillMargins_PixelBasedAlgo(&RealMedianFilterTemplBase<CellType>::MirrorFiller,
		callBackObj);
}

//--------------------------------//
//       Прочий функционал        //
//--------------------------------//

template <typename CellType>
bool RealMedianFilterTemplBase<CellType>::LoadImage(const std::string &fileName, ErrorInfo *errObj,
	CallBackBase *callBackObj)
//Читает изображение в матрицу так чтобы по краям оставалось место для создания граничных
//пикселей.
{
	//Чистим матрицу и грузим в неё файл.
	bool result;
	sourceMatrix_.Clear();
	getOwnerObj().FixAperture();
	result = sourceMatrix_.LoadFromGDALFile(fileName, (getOwnerObj().getAperture() - 1) / 2, errObj);
	if (result)
		getOwnerObj().setSourceFileName(fileName);
	return result;
}

template <typename CellType>
bool  RealMedianFilterTemplBase<CellType>::SaveImage(const std::string &fileName, ErrorInfo *errObj)
//Сохраняет матрицу в изображение. За основу берётся ранее загруженная через LoadImage
//картинка - файл копируется под новым именем и затем в него вносятся изменённые пиксели.
///В первую очередь это нужно чтобы оставить метаданные в неизменном оригинальном виде.
{
	//Недопилено.
	filesystem::path destFilePath, sourceFilePath, tempFilePath, basePath;
	system::error_code errCode;
	destFilePath = STB.Utf8ToWstring(fileName);
	sourceFilePath = STB.Utf8ToWstring(getOwnerObj().getSourceFileName());
	basePath = destFilePath.parent_path();

	//PARANOYA MODE ON. Надо убедиться что файл есть куда записывать.
	if (!filesystem::is_directory(basePath))
	{
		if (!filesystem::exists(basePath))
		{
			if (!filesystem::create_directories(basePath, errCode))
			{
				//Нет директории для файла и не удалось её создать. Облом :(.
				if (errObj) errObj->SetError(CMNERR_WRITE_ERROR, ": " + errCode.message());
				return false;
			}
		}
		else
		{
			//Путь есть но это не директория - лучше ничего не трогать. Облом :(
			if (errObj) errObj->SetError(CMNERR_WRITE_ERROR,
				": невозможно создать каталог для записи файла");
			return false;
		}
	}
	//PARANOYA MODE OFF

	//Скопируем исходный файл в новый временный, в том же каталоге что и целевое имя файла.
	tempFilePath = basePath;
	tempFilePath /= filesystem::unique_path("%%%%-%%%%-%%%%-%%%%.tmp");
	string tempFileName = STB.WstringToUtf8(tempFilePath.wstring());
	if (!filesystem::exists(sourceFilePath) || filesystem::exists(tempFilePath))
	{
		if (errObj) errObj->SetError(CMNERR_WRITE_ERROR,
			": не удалось скопировать исходный файл во временный");
		return false;
	}
	system::error_code boostErrCode;
	filesystem::copy_file(sourceFilePath, tempFilePath, boostErrCode);
	if (boostErrCode.value())
	{
		if (errObj) errObj->SetError(CMNERR_WRITE_ERROR,
			": не удалось скопировать исходный файл во временный");
		return false;
	}
	
	//Собсно, запись.
	if (!destMatrix_.SaveToGDALFile(tempFileName, 0, 0, errObj))
	{
		filesystem::remove(tempFilePath, boostErrCode);
		return false;
	};

	//Всё ок. Осталось переименовать.
	filesystem::rename(tempFilePath, destFilePath, boostErrCode);
	if (boostErrCode.value())
	{
		if (errObj) errObj->SetError(CMNERR_WRITE_ERROR,
			": не удалось переименовать временный файл в "+destFilePath.string());
		return false;
	}

	//Всё ок вроде
	return true;
}

template <typename CellType>
void RealMedianFilterTemplBase<CellType>::FillMargins(CallBackBase *callBackObj)
//Заполняет граничные (пустые пиксели) области вокруг значимых пикселей в соответствии с
//выбранным алгоритмом.
{
	switch (getOwnerObj().getMarginType())
	{
	case MARGIN_SIMPLE_FILLING: FillMargins_Simple(callBackObj); break;
	case MARGIN_MIRROR_FILLING: FillMargins_Mirror(callBackObj);
	}		
}

template <typename CellType>
void RealMedianFilterTemplBase<CellType>::ApplyStupidFilter(CallBackBase *callBackObj)
//Обрабатывает матрицу sourceMatrix_ "тупым" фильтром. Результат записывает в destMatrix_.
{
	//Данный метод применяет медианный фильтр "в лоб", т.ч. тупо для каждого пикселя создаёт
	//массив из всех пикселей, входящих в окно, после чего сортирует массив и медиану подставляет
	//на место пикселя в dest-матрице. Применять имеет смысл разве что в тестовых целях и для
	//сравнения с оптимизированными алгоритмами.
	int destX, destY, windowX, windowY, sourceX, sourceY, marginSize, medianArrPos;
	marginSize = (getOwnerObj().getAperture() - 1) / 2;
	destMatrix_.CreateDestMatrix(sourceMatrix_,marginSize);
	//Сразу вычислим индексы и указатели чтобы не считать в цикле.
	//И выделим память под массив для пикселей окна, в котором ищем медиану.
	int medianArrSize = getOwnerObj().getAperture() * getOwnerObj().getAperture();
	CellType *medianArr = new CellType[medianArrSize];
	//Целочисленное деление, неточностью пренебрегаем ибо ожидаем окно со
	//стороной в десятки пикселей.
	CellType *medianPos = medianArr+(medianArrSize/2);
	CellType *medianArrEnd = medianArr+medianArrSize; //Элемент за последним в массиве!
	//Настроим объект, выводящий информацию о прогрессе обработки.
	if (callBackObj)
		callBackObj->setMaxProgress(destMatrix_.getXSize() * destMatrix_.getYSize());
	unsigned long progressPosition = 0;
	
	//Поехали.
	for (destY = 0; destY < destMatrix_.getYSize(); destY++)
	{
		sourceY=destY+marginSize;
		for (destX = 0; destX < destMatrix_.getXSize(); destX++)
		{
			sourceX=destX+marginSize;
			progressPosition++;
			//Незначимые пиксели не трогаем.
			if (sourceMatrix_.getSignMatrixElem(sourceY,sourceX) !=1) continue;
			//Теперь надо пройти по каждому пикселю окна чтобы составить массив
			//и сортировкой получить медиану. Наверное самый неэффективны способ.
			medianArrPos = 0;
			for (windowY=destY; windowY<(sourceY+marginSize); windowY++)
			{
				for (windowX=destX; windowX<(sourceX+marginSize); windowX++)
				{
					medianArr[medianArrPos]=sourceMatrix_.getMatrixElem(windowY,windowX);
					medianArrPos++;
				}
			}
			//Сортируем, берём медиану из середины. Точностью поиска середины не
			//заморачиваемся т.к. окна будут большие, в десятки пикселов.
			std::nth_element(medianArr,medianPos,medianArrEnd);
			if (GetDelta(*medianPos,sourceMatrix_.getMatrixElem(sourceY,sourceX))
				< getOwnerObj().getThreshold())
			{
				//Отличие от медианы меньше порогового. Просто копируем пиксел.
				destMatrix_.setMatrixElem(destY,destX,
					sourceMatrix_.getMatrixElem(sourceY,sourceX));
			}
			else
			{
				//Отличие больше порогового - записываем в dest-пиксель медиану.
				destMatrix_.setMatrixElem(destY,destX,*medianPos);
			};
			//Сообщить вызвавшему коду прогресс выполнения.
			if (callBackObj) callBackObj->CallBack(progressPosition);
		};
	};
	//Не забыть delete! ))
	delete[] medianArr;
}

template <typename CellType>
void RealMedianFilterTemplBase<CellType>::ApplyStubFilter(CallBackBase *callBackObj)
//Обрабатывает матрицу sourceMatrix_ "никаким" фильтром. По сути просто копирование.
//Для отладки. Результат записывает в destMatrix_.
{
	int destX, destY, sourceX, sourceY, marginSize;
	marginSize = (getOwnerObj().getAperture() - 1) / 2;
	destMatrix_.CreateDestMatrix(sourceMatrix_, marginSize);
	if (callBackObj)
		callBackObj->setMaxProgress(destMatrix_.getXSize() * destMatrix_.getYSize());
	unsigned long progressPosition = 0;

	//Поехали.
	for (destY = 0; destY < destMatrix_.getYSize(); destY++)
	{
		sourceY = destY + marginSize;
		for (destX = 0; destX < destMatrix_.getXSize(); destX++)
		{
			sourceX = destX + marginSize;
			progressPosition++;
			//Незначимые пиксели не трогаем.
			if (sourceMatrix_.getSignMatrixElem(sourceY,sourceX) != 1) continue;

			//Просто копируем пиксел.
			destMatrix_.setMatrixElem(destY,destX,
				sourceMatrix_.getMatrixElem(sourceY,sourceX));

			//Сообщить вызвавшему коду прогресс выполнения.
			if (callBackObj) callBackObj->CallBack(progressPosition);
		};
	};
}

template <typename CellType>
void RealMedianFilterTemplBase<CellType>::SourcePrintStupidVisToCout()
//"Тупая" визуализация матрицы, отправляется прямо в cout.
{
	//Просто проброс вызова в объект матрицы.
	sourceMatrix_.PrintStupidVisToCout();
}

template <typename CellType>
bool RealMedianFilterTemplBase<CellType>::SourceSaveToCSVFile(const std::string &fileName, ErrorInfo *errObj)
//Вывод исходной матрицы в csv-файл, который должны понимать всякие картографические
//программы. Это значит что каждый пиксел - это одна строка в файле.
//Это "тупой" вариант вывода - метаданные нормально не сохраняются.
{
	//Просто проброс вызова в объект матрицы.
	return sourceMatrix_.SaveToCSVFile(fileName, errObj);
}

template <typename CellType>
bool RealMedianFilterTemplBase<CellType>::DestSaveToCSVFile(const std::string &fileName, ErrorInfo *errObj)
//Аналогично SourceSaveToCSVFile, но для матрицы с результатом.
{
	//Просто проброс вызова в объект матрицы.
	return destMatrix_.SaveToCSVFile(fileName, errObj);
}

////////////////////////////////////
//          MedianFilter          //
////////////////////////////////////

MedianFilter::MedianFilter() : aperture_(DEFAULT_APERTURE), threshold_(DEFAULT_THRESHOLD),
marginType_(DEFAULT_MARGIN_TYPE), useMemChunks_(false), maxDataSize_(0), sourceFileName_(""),
destFileName_(""), imageSizeX_(0), imageSizeY_(0), imageIsLoaded_(false), sourceIsAttached_(false),
destIsAttached_(false), dataType_(PIXEL_UNKNOWN), pFilterObj(NULL)
{

}

MedianFilter::~MedianFilter()
{
	//Возможно создавался объект реального фильтра. Надо удалить.
	delete pFilterObj;
}

bool MedianFilter::SelectInputFile(const std::string &fileName, ErrorInfo *errObj)
//Выбрать исходный файл для дальнейшего чтения и обработки. Получает информацию о параметрах изображения,
//запоминает её в полях объекта.
{
	//TODO: когда избавлюсь от Load и Save - тут должны будут создаваться объекты GDAL, существующие до
	//завершения работы с файлом. Чтобы не создавать их постоянно в процессе по-кусочечной обработки
	//картинки.

	//TODO:вот эта проверка временная! Нужна только пока используется load и save!
	if ((sourceIsAttached_) && (fileName == sourceFileName_))
		return true;

	//А был ли файл?
	filesystem::path filePath = STB.Utf8ToWstring(fileName);
	if (!filesystem::is_regular_file(filePath))
	{
		if (errObj)	errObj->SetError(CMNERR_FILE_NOT_EXISTS, ": " + fileName);
		return false;
	}

	//Открываем картинку, определяем тип и размер данных.
	GDALDataset *inputDataset = (GDALDataset*)GDALOpen(fileName.c_str(), GA_ReadOnly);
	if (!inputDataset)
	{
		if (errObj) errObj->SetError(CMNERR_READ_ERROR, ": " + fileName);
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
	dataType_ = GDALToGIC_PixelType(inputRaster->GetRasterDataType());
	imageSizeX_ = inputRaster->GetXSize();
	imageSizeY_ = inputRaster->GetYSize();
	if (dataType_ == PIXEL_UNKNOWN)
	{
		GDALClose(inputDataset);
		if (errObj) errObj->SetError(CMNERR_UNSUPPORTED_FILE_FORMAT, ": " + fileName);
		return false;
	}

	//Если GDAL говорит что тип пикселя - байт - надо посмотреть метаданные какой именно там
	//тип байтов.
	if (dataType_ == PIXEL_INT8)
	{
		const char *tempStr = inputRaster->GetMetadataItem("PIXELTYPE", "IMAGE_STRUCTURE");
		if ((tempStr == NULL) || strcmp(tempStr, "SIGNEDBYTE"))
		{
			//Это явно не signed-байт
			dataType_ = PIXEL_UINT8;
		}
	}

	//Тип данных определён, надо закрыть картинку, создать вложенный объект нужного типа
	//и дальше все вызовы транслировать уже в него. Параметры фильтра также важно
	//не забыть пробросить.
	GDALClose(inputDataset);
	inputRaster = NULL;
	delete pFilterObj;
	switch (dataType_)
	{
		case PIXEL_INT8: pFilterObj = new RealMedianFilterInt8(this); break;
		case PIXEL_UINT8: pFilterObj = new RealMedianFilterUInt8(this); break;
		case PIXEL_INT16: pFilterObj = new RealMedianFilterInt16(this); break;
		case PIXEL_UINT16: pFilterObj = new RealMedianFilterUInt16(this); break;
		case PIXEL_INT32: pFilterObj = new RealMedianFilterInt32(this); break;
		case PIXEL_UINT32: pFilterObj = new RealMedianFilterUInt32(this); break;
		case PIXEL_FLOAT32: pFilterObj = new RealMedianFilterFloat32(this); break;
		case PIXEL_FLOAT64: pFilterObj = new RealMedianFilterFloat64(this); break;
		default: pFilterObj = NULL;
	}
	if (pFilterObj)
	{
		sourceIsAttached_ = true;
		sourceFileName_ = fileName;
	}
	else
	{
		if (errObj) errObj->SetError(CMNERR_UNKNOWN_ERROR, "MedianFilter::SelectInputFile() error creating pFilterObj!",true);
		sourceIsAttached_ = false;
		return false;
	}
	return true;
}


bool MedianFilter::SelectOutputFile(const std::string &fileName, const bool &forceRewrite, ErrorInfo *errObj)
//Подготовить целевой файл к записи в него результата. Если forceRewrite==false - вернёт ошибку в виде
//false и кода ошибки в errObj.
{
	//Заглушка.
	if (errObj) errObj->SetError(CMNERR_FEATURE_NOT_READY);
	return false;
}

bool MedianFilter::LoadImage(const std::string &fileName, ErrorInfo *errObj,
	CallBackBase *callBackObj)
	//Читает изображение в матрицу так чтобы по краям оставалось место для создания граничных
	//пикселей.
{
	FixAperture();
	if (SelectInputFile(fileName, errObj))
	{
		imageIsLoaded_ = pFilterObj->LoadImage(fileName, errObj, callBackObj);
	}
	
	return imageIsLoaded_;
}

bool MedianFilter::SaveImage(const std::string &fileName, ErrorInfo *errObj)
//Сохраняет матрицу в изображение. За основу берётся ранее загруженная через LoadImage
//картинка - файл копируется под новым именем и затем в него вносятся изменённые пиксели.
///В первую очередь это нужно чтобы оставить метаданные в неизменном оригинальном виде.
{
	//Тупой проброс вызова.
	if (imageIsLoaded_)
	{
		return pFilterObj->SaveImage(fileName, errObj);
	}
	else
	{
		if (errObj) errObj->SetError(CMNERR_FILE_NOT_LOADED);
		return false;
	}
}

void MedianFilter::FillMargins(CallBackBase *callBackObj)
//Заполняет граничные (пустые пиксели) области вокруг значимых пикселей в соответствии с
//выбранным алгоритмом.
{
	//Проброс вызова.
	if (imageIsLoaded_)
	{
		pFilterObj->FillMargins(callBackObj);
	}
}

void MedianFilter::ApplyStupidFilter(CallBackBase *callBackObj)
//Обрабатывает матрицу sourceMatrix_ "тупым" фильтром. Результат записывает в destMatrix_.
{
	//Проброс вызова
	if (imageIsLoaded_)
	{
		pFilterObj->ApplyStupidFilter(callBackObj);
	}
}

//Приводит апертуру (длина стороны окна фильтра) к имеющему смысл значению.
void MedianFilter::FixAperture()
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

void MedianFilter::ApplyStubFilter(CallBackBase *callBackObj)
//Обрабатывает матрицу sourceMatrix_ "никаким" фильтром. По сути просто копирование.
//Для отладки. Результат записывает в destMatrix_.
{
	//Проброс вызова
	if (imageIsLoaded_)
	{
		pFilterObj->ApplyStubFilter(callBackObj);
	}
}

void MedianFilter::SourcePrintStupidVisToCout()
//"Тупая" визуализация матрицы, отправляется прямо в cout.
{
	//Просто проброс вызова в объект матрицы.
	pFilterObj->SourcePrintStupidVisToCout();
}

bool MedianFilter::SourceSaveToCSVFile(const std::string &fileName, ErrorInfo *errObj)
//Вывод исходной матрицы в csv-файл, который должны понимать всякие картографические
//программы. Это значит что каждый пиксел - это одна строка в файле.
//Это "тупой" вариант вывода - метаданные нормально не сохраняются.
{
	//Просто проброс вызова в объект матрицы.
	return pFilterObj->SourceSaveToCSVFile(fileName, errObj);
}

bool MedianFilter::DestSaveToCSVFile(const std::string &fileName, ErrorInfo *errObj)
//Аналогично SourceSaveToCSVFile, но для матрицы с результатом.
{
	//Просто проброс вызова в объект матрицы.
	return pFilterObj->DestSaveToCSVFile(fileName, errObj);
}

unsigned long long MedianFilter::CalcMinSourceBlockSize() const
//Вычислить минимальный размер исходного блока (в байтах), которыми можно обработать данную картинку.
//Исходный блок - т.е. содержащий данные исходной картинки.
//Файл картинки должен уже был быть выбран. В случае проблем вернёт 0.
{
	//Смысл в том, что фильтр обрабатывает картинку, в которой помимо самой картинки содержатся
	//граничные пиксели - сверху, снизу, слева, справа. Они либо генерируются одним из алгоритмов,
	//либо это часть самой картинки, но в данный момент эта часть считается незначимой, т.е. не
	//обрабатывается. Картинки удобно обрабатывать построчно. Ширина и высота этих незначимых "полей"
	//известна из апертуры. Получается что нам надо держать в памяти одновременно 3 блока: строки
	//уже незначимых пикселей сверху значимого блока, затем строки значимого блока (в начале и в конце
	//этих строк тоже будут лежать незначимые пиксели), а затем строки ещё незначимого блока (то что ниже).
	//Таким образом, минимально для обработки картинки в памяти должно быть доступно утроенное количество
	//байт от значения, возвращаемого данной функцией.

	//ВысотаБлока = (апертура-1)/2 но не больше высоты изображения!
	//ШиринаБлока = 2*ВысотаБлока+ШиринаКартинки
	//РазмерБлока = 3*ВысотаБлока*ШиринаБлока*(размер элемента+1 байт на элемент вспомогательной матрицы)
	//В общем, комментариев много, кода мало:
	unsigned long long blockHeight = (aperture_ - 1) / 2;
	if (blockHeight > imageSizeY_)
		blockHeight = imageSizeY_;
	return 3 * blockHeight*((2 * blockHeight) + imageSizeX_)/* * (pFilterObj->)*/;
}


unsigned long long MedianFilter::CalcMinDestBlockSize() const
//Вычислить минимальный размер блока назначения (в байтах). Блок назначения - куда записываются пиксели
//уже обработанного изображения и хранятся до их записи в файл. Количество этих блоков равно количеству
//исходных блоков - 2 штуки (т.к. 1 блок - верхние граничные пиксели и 1 блок - нижние), размер самого
//блока - немного меньше чем размер исходного блока.
//Файл картинки должен уже был быть выбран. В случае проблем вернёт 0.
{
	//Заглушка
	return 0;
}


unsigned long long MedianFilter::CalcMinMemSize() const
//Вычислить минимальное количество памяти, с которым вообще сможет работать медианный фильтр.
{
	//Заглушка
	return 0;
}

} //namespace geoimgconv