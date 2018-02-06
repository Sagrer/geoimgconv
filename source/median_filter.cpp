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

////////////////////////////////////
//        MedianFilterBase        //
////////////////////////////////////

//Класс абстрактный, реализуем на всякий случай конструктор и деструктор.
MedianFilterBase::MedianFilterBase() : aperture_(DEFAULT_APERTURE),
threshold_(DEFAULT_THRESHOLD), marginType_(DEFAULT_MARGIN_TYPE)
{

}

MedianFilterBase::~MedianFilterBase()
{

}

//--------------------------------//
//     Не-абстрактные методы      //
//--------------------------------//

//Приводит апертуру (длина стороны окна фильтра) к имеющему смысл значению.
void MedianFilterBase::FixAperture()
{
	//Апертура меньше трёх невозможна и не имеет смысла.
	//Апертура больше трёх должна быть нечётной чтобы в любую сторону от текущего пикселя было
	//одинаковое количество пикселей до края окна. Округлять будем в бОльшую сторону.
	if (this->aperture_ < 3)
		this->aperture_ = 3;
	else
	{
		if ((this->aperture_ % 2) == 0)
			this->aperture_++;
	}
}

////////////////////////////////////
//         MedianFilter           //
////////////////////////////////////

//--------------------------------//
//   Конструкторы-деструкторы     //
//--------------------------------//

template <typename CellType> MedianFilterTemplBase<CellType>::MedianFilterTemplBase() : sourceFileName_("")
{
}

template <typename CellType> MedianFilterTemplBase<CellType>::~MedianFilterTemplBase()
{

}

//--------------------------------//
//       Приватные методы         //
//--------------------------------//

template <typename CellType>
bool MedianFilterTemplBase<CellType>::PixelStep(int &x, int &y, const PixelDirection direction)
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
	if ((x >= 0) && (y >= 0) && (x < this->sourceMatrix_.getXSize()) && (y < this->sourceMatrix_.getYSize()))
		return true;
	else
		return false;
}

//Получает координаты действительного пикселя, зеркального для
//currXY по отношению к центральному xy. Результат записывается
//в outXY.
template <typename CellType>
void MedianFilterTemplBase<CellType>::GetMirrorPixel(const int &x, const int &y, const int &currX, const int &currY, int &outX, int &outY, const int recurseDepth)
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
		if ((outX >= 0) && (outX < this->sourceMatrix_.getXSize()) &&
			(outY >= 0) && (outY < this->sourceMatrix_.getYSize()))
		{
			if (this->sourceMatrix_.getSignMatrixElem(outY,outX) == 1)
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
					this->GetMirrorPixel(tempX,tempY,firstX,firstY,outX,outY,recurseDepth+1);
				};
				found = true;
			}		
		};
		if (found==false) shift++; //Ничего не найдено - поищем ближе к центру.
	} while(found==false);
};

template <typename CellType>
void MedianFilterTemplBase<CellType>::SimpleFiller(const int &x, const int &y,
	const PixelDirection direction, const int &marginSize)
//Заполнять пиксели простым алгоритмом в указанном направлении
{
	int currX = x;
	int currY = y;
	int stepCounter = 0;

	while (this->PixelStep(currX, currY, direction))
	{
		stepCounter++;
		if((this->sourceMatrix_.getSignMatrixElem(currY,currX)==1) ||
			(stepCounter > marginSize) )
		{
			//Значимый пиксель, либо ушли за пределы окна. Прекращаем заполнение.
			return;
		}
		else if ((this->sourceMatrix_.getSignMatrixElem(currY,currX) == 0) ||
			(direction == PIXEL_DIR_UP) || (direction == PIXEL_DIR_DOWN) ||
			(direction == PIXEL_DIR_RIGHT) || (direction == PIXEL_DIR_LEFT))
		{
			//Заполняем либо вообще не заполненные пиксели, либо любые незначимые
			//если двигаемся не по диагонали чтобы возможно переписать те, что были
			//уже заполнены диагональным заполнителем.
			this->sourceMatrix_.setMatrixElem(currY,currX,this->sourceMatrix_.getMatrixElem(y,x));
			this->sourceMatrix_.setSignMatrixElem(currY,currX,2); //Заполненный незначимый пиксель.
		}
	}
}

template <typename CellType>
void MedianFilterTemplBase<CellType>::MirrorFiller(const int &x, const int &y,
	const PixelDirection direction, const int &marginSize)
//Заполнять пиксели зеркальным алгоритмом в указанном направлении
{
	int currX = x;
	int currY = y;
	int mirrorX, mirrorY;
	int stepCounter = 0;

	while (this->PixelStep(currX, currY, direction))
	{
		stepCounter++;
		if((this->sourceMatrix_.getSignMatrixElem(currY,currX) == 1) ||
			(stepCounter > marginSize) )
		{
			//Значимый пиксель, либо ушли за пределы окна. Прекращаем заполнение.
			return;
		}
		else if ((this->sourceMatrix_.getSignMatrixElem(currY,currX) == 0) ||
			(direction == PIXEL_DIR_UP) || (direction == PIXEL_DIR_DOWN) ||
			(direction == PIXEL_DIR_RIGHT) || (direction == PIXEL_DIR_LEFT))
		{
			//Заполняем либо вообще не заполненные пиксели, либо любые незначимые
			//если двигаемся не по диагонали чтобы возможно переписать те, что были
			//уже заполнены диагональным заполнителем.
			this->GetMirrorPixel(x,y,currX,currY,mirrorX,mirrorY);	//Координаты зеркального пикселя.
			this->sourceMatrix_.setMatrixElem(currY,currX,this->sourceMatrix_.getMatrixElem(mirrorY,mirrorX));
			this->sourceMatrix_.setSignMatrixElem(currY,currX,2); //Заполненный незначимый пиксель.
		}
	}
}

template <typename CellType>
void MedianFilterTemplBase<CellType>::FillMargins_PixelBasedAlgo(const TFillerMethod FillerMethod,
	CallBackBase *callBackObj)
//Костяк алгоритма, общий для Simple и Mirror
{
	//Двигаемся построчно, пока не найдём значимый пиксель. В границы не лезем,
	//т.к. они значимыми быть не могут.
	int x, y, marginSize;
	marginSize = (this->getAperture() - 1) / 2;
	//Настроим объект, выводящий информацию о прогрессе обработки.
	if (callBackObj)
		callBackObj->setMaxProgress((this->sourceMatrix_.getXSize() - marginSize*2) *
			(this->sourceMatrix_.getYSize() - marginSize*2));
	unsigned long progressPosition = 0;
	//Поехали.
	for (y = marginSize; y < (this->sourceMatrix_.getYSize() - marginSize); y++)
	{
		for (x = marginSize; x < (this->sourceMatrix_.getXSize() - marginSize); x++)
		{
			progressPosition++;
			if (this->sourceMatrix_.getSignMatrixElem(y,x) == 1)
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
void MedianFilterTemplBase<CellType>::FillMargins_Simple(CallBackBase *callBackObj)
//Заполнить пустые пиксели source-матрицы простым алгоритмом (сплошной цвет).
{
	this->FillMargins_PixelBasedAlgo(&MedianFilterTemplBase<CellType>::SimpleFiller,
		callBackObj);
}

template <typename CellType>
void MedianFilterTemplBase<CellType>::FillMargins_Mirror(CallBackBase *callBackObj)
//Заполнить пустые пиксели source-матрицы зеркальным алгоритмом.
{
	this->FillMargins_PixelBasedAlgo(&MedianFilterTemplBase<CellType>::MirrorFiller,
		callBackObj);
}

//--------------------------------//
//       Прочий функционал        //
//--------------------------------//

template <typename CellType>
bool MedianFilterTemplBase<CellType>::LoadImage(const std::string &fileName, ErrorInfo *errObj,
	CallBackBase *callBackObj)
//Читает изображение в матрицу так чтобы по краям оставалось место для создания граничных
//пикселей.
{
	//Чистим матрицу и грузим в неё файл.
	bool result;
	this->sourceMatrix_.Clear();
	this->FixAperture();
	result = this->sourceMatrix_.LoadFromGDALFile(fileName, (this->getAperture() - 1) / 2, errObj);
	if (result)
		this->sourceFileName_ = fileName;
	return result;
}

template <typename CellType>
bool  MedianFilterTemplBase<CellType>::SaveImage(const std::string &fileName, ErrorInfo *errObj)
//Сохраняет матрицу в изображение. За основу берётся ранее загруженная через LoadImage
//картинка - файл копируется под новым именем и затем в него вносятся изменённые пиксели.
///В первую очередь это нужно чтобы оставить метаданные в неизменном оригинальном виде.
{
	//Недопилено.
	filesystem::path destFilePath, sourceFilePath, tempFilePath, basePath;
	system::error_code errCode;
	destFilePath = STB.Utf8ToWstring(fileName);
	sourceFilePath = STB.Utf8ToWstring(this->sourceFileName_);
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
	if (!this->destMatrix_.SaveToGDALFile(tempFileName, 0, 0, errObj))
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
void MedianFilterTemplBase<CellType>::FillMargins(CallBackBase *callBackObj)
//Заполняет граничные (пустые пиксели) области вокруг значимых пикселей в соответствии с
//выбранным алгоритмом.
{
	switch (this->getMarginType())
	{
	case MARGIN_SIMPLE_FILLING: this->FillMargins_Simple(callBackObj); break;
	case MARGIN_MIRROR_FILLING: this->FillMargins_Mirror(callBackObj);
	}		
}

template <typename CellType>
void MedianFilterTemplBase<CellType>::ApplyStupidFilter(CallBackBase *callBackObj)
//Обрабатывает матрицу sourceMatrix_ "тупым" фильтром. Результат записывает в destMatrix_.
{
	//Данный метод применяет медианный фильтр "в лоб", т.ч. тупо для каждого пикселя создаёт
	//массив из всех пикселей, входящих в окно, после чего сортирует массив и медиану подставляет
	//на место пикселя в dest-матрице. Применять имеет смысл разве что в тестовых целях и для
	//сравнения с оптимизированными алгоритмами.
	int destX, destY, windowX, windowY, sourceX, sourceY, marginSize, medianArrPos;
	marginSize = (this->getAperture() - 1) / 2;
	this->destMatrix_.CreateDestMatrix(this->sourceMatrix_,marginSize);
	//Сразу вычислим индексы и указатели чтобы не считать в цикле.
	//И выделим память под массив для пикселей окна, в котором ищем медиану.
	int medianArrSize = this->getAperture() * this->getAperture();	
	CellType *medianArr = new CellType[medianArrSize];
	//Целочисленное деление, неточностью пренебрегаем ибо ожидаем окно со
	//стороной в десятки пикселей.
	CellType *medianPos = medianArr+(medianArrSize/2);
	CellType *medianArrEnd = medianArr+medianArrSize; //Элемент за последним в массиве!
	//Настроим объект, выводящий информацию о прогрессе обработки.
	if (callBackObj)
		callBackObj->setMaxProgress(this->destMatrix_.getXSize() * this->destMatrix_.getYSize());
	unsigned long progressPosition = 0;
	
	//Поехали.
	for (destY = 0; destY < this->destMatrix_.getYSize(); destY++)
	{
		sourceY=destY+marginSize;
		for (destX = 0; destX < this->destMatrix_.getXSize(); destX++)
		{
			sourceX=destX+marginSize;
			progressPosition++;
			//Незначимые пиксели не трогаем.
			if (this->sourceMatrix_.getSignMatrixElem(sourceY,sourceX) !=1) continue;
			//Теперь надо пройти по каждому пикселю окна чтобы составить массив
			//и сортировкой получить медиану. Наверное самый неэффективны способ.
			medianArrPos = 0;
			for (windowY=destY; windowY<(sourceY+marginSize); windowY++)
			{
				for (windowX=destX; windowX<(sourceX+marginSize); windowX++)
				{
					medianArr[medianArrPos]=this->sourceMatrix_.getMatrixElem(windowY,windowX);
					medianArrPos++;
				}
			}
			//Сортируем, берём медиану из середины. Точностью поиска середины не
			//заморачиваемся т.к. окна будут большие, в десятки пикселов.
			std::nth_element(medianArr,medianPos,medianArrEnd);
			if (this->GetDelta(*medianPos,this->sourceMatrix_.getMatrixElem(sourceY,sourceX))
				< this->getThreshold())
			{
				//Отличие от медианы меньше порогового. Просто копируем пиксел.
				this->destMatrix_.setMatrixElem(destY,destX,
					this->sourceMatrix_.getMatrixElem(sourceY,sourceX));
			}
			else
			{
				//Отличие больше порогового - записываем в dest-пиксель медиану.
				this->destMatrix_.setMatrixElem(destY,destX,*medianPos);
			};
			//Сообщить вызвавшему коду прогресс выполнения.
			if (callBackObj) callBackObj->CallBack(progressPosition);
		};
	};
	//Не забыть delete! ))
	delete[] medianArr;
}

template <typename CellType>
void MedianFilterTemplBase<CellType>::ApplyStubFilter(CallBackBase *callBackObj)
//Обрабатывает матрицу sourceMatrix_ "никаким" фильтром. По сути просто копирование.
//Для отладки. Результат записывает в destMatrix_.
{
	int destX, destY, sourceX, sourceY, marginSize;
	marginSize = (this->getAperture() - 1) / 2;
	this->destMatrix_.CreateDestMatrix(this->sourceMatrix_, marginSize);
	if (callBackObj)
		callBackObj->setMaxProgress(this->destMatrix_.getXSize() * this->destMatrix_.getYSize());
	unsigned long progressPosition = 0;

	//Поехали.
	for (destY = 0; destY < this->destMatrix_.getYSize(); destY++)
	{
		sourceY = destY + marginSize;
		for (destX = 0; destX < this->destMatrix_.getXSize(); destX++)
		{
			sourceX = destX + marginSize;
			progressPosition++;
			//Незначимые пиксели не трогаем.
			if (this->sourceMatrix_.getSignMatrixElem(sourceY,sourceX) != 1) continue;

			//Просто копируем пиксел.
			this->destMatrix_.setMatrixElem(destY,destX,
				this->sourceMatrix_.getMatrixElem(sourceY,sourceX));

			//Сообщить вызвавшему коду прогресс выполнения.
			if (callBackObj) callBackObj->CallBack(progressPosition);
		};
	};
}

template <typename CellType>
void MedianFilterTemplBase<CellType>::SourcePrintStupidVisToCout()
//"Тупая" визуализация матрицы, отправляется прямо в cout.
{
	//Просто проброс вызова в объект матрицы.
	this->sourceMatrix_.PrintStupidVisToCout();
}

template <typename CellType>
bool MedianFilterTemplBase<CellType>::SourceSaveToCSVFile(const std::string &fileName, ErrorInfo *errObj)
//Вывод исходной матрицы в csv-файл, который должны понимать всякие картографические
//программы. Это значит что каждый пиксел - это одна строка в файле.
//Это "тупой" вариант вывода - метаданные нормально не сохраняются.
{
	//Просто проброс вызова в объект матрицы.
	return this->sourceMatrix_.SaveToCSVFile(fileName, errObj);
}

template <typename CellType>
bool MedianFilterTemplBase<CellType>::DestSaveToCSVFile(const std::string &fileName, ErrorInfo *errObj)
//Аналогично SourceSaveToCSVFile, но для матрицы с результатом.
{
	//Просто проброс вызова в объект матрицы.
	return this->destMatrix_.SaveToCSVFile(fileName, errObj);
}

////////////////////////////////////
//     MedianFilterUniversal      //
////////////////////////////////////

//--------------------------------//
//   Конструкторы-деструкторы     //
//--------------------------------//

MedianFilterUniversal::MedianFilterUniversal() : pFilterObj(NULL),
imageIsLoaded(false), dataType(PIXEL_UNKNOWN)
{

}

MedianFilterUniversal::~MedianFilterUniversal()
{

}

//--------------------------------//
//       Приватные методы         //
//--------------------------------//

//Обновить параметры вложенного объекта из параметров данного объекта.
//Можно вызывать только если точно известно что this->pFilterObj существует.
void MedianFilterUniversal::UpdateSettings()
{
	this->pFilterObj->setAperture(this->getAperture());
	this->pFilterObj->setMarginType(this->getMarginType());
	this->pFilterObj->setThreshold(this->getThreshold());
}

//--------------------------------//
//       Прочий функционал        //
//--------------------------------//

bool MedianFilterUniversal::LoadImage(const std::string &fileName, ErrorInfo *errObj,
	CallBackBase *callBackObj)
	//Читает изображение в матрицу так чтобы по краям оставалось место для создания граничных
	//пикселей.
{
	//А был ли файл?
	filesystem::path filePath = STB.Utf8ToWstring(fileName);
	if (!filesystem::is_regular_file(filePath))
	{
		if (errObj)	errObj->SetError(CMNERR_FILE_NOT_EXISTS, ": " + fileName);
		return false;
	}

	//Открываем картинку, определяем тип данных.
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
	this->dataType = GDALToGIC_PixelType(inputRaster->GetRasterDataType());
	if (this->dataType == PIXEL_UNKNOWN)
	{
		GDALClose(inputDataset);
		if (errObj) errObj->SetError(CMNERR_UNSUPPORTED_FILE_FORMAT, ": " + fileName);
		return false;
	}

	//Если GDAL говорит что тип пикселя - байт - надо посмотреть метаданные какой именно там
	//тип байтов.
	if (this->dataType == PIXEL_INT8)
	{
		const char *tempStr = inputRaster->GetMetadataItem("PIXELTYPE", "IMAGE_STRUCTURE");
		if ((tempStr == NULL) || strcmp(tempStr, "SIGNEDBYTE"))
		{
			//Это явно не signed-байт
			this->dataType = PIXEL_UINT8;
		}
	}

	//Тип данных определён, надо закрыть картинку, создать вложенный объект нужного типа
	//и дальше все вызовы транслировать уже в него. Параметры фильтра также важно
	//не забыть пробросить.
	GDALClose(inputDataset);
	inputRaster = NULL;
	switch (this->dataType)
	{
		case PIXEL_INT8: this->pFilterObj = new MedianFilterInt8(); break;
		case PIXEL_UINT8: this->pFilterObj = new MedianFilterUInt8(); break;
		case PIXEL_INT16: this->pFilterObj = new MedianFilterInt16(); break;
		case PIXEL_UINT16: this->pFilterObj = new MedianFilterUInt16(); break;
		case PIXEL_INT32: this->pFilterObj = new MedianFilterInt32(); break;
		case PIXEL_UINT32: this->pFilterObj = new MedianFilterUInt32(); break;
		case PIXEL_FLOAT32: this->pFilterObj = new MedianFilterFloat32(); break;
		case PIXEL_FLOAT64: this->pFilterObj = new MedianFilterFloat64(); break;
	}
	this->FixAperture();
	this->UpdateSettings();
	this->imageIsLoaded = this->pFilterObj->LoadImage(fileName, errObj, callBackObj);
	return this->imageIsLoaded;
}

bool MedianFilterUniversal::SaveImage(const std::string &fileName, ErrorInfo *errObj)
//Сохраняет матрицу в изображение. За основу берётся ранее загруженная через LoadImage
//картинка - файл копируется под новым именем и затем в него вносятся изменённые пиксели.
///В первую очередь это нужно чтобы оставить метаданные в неизменном оригинальном виде.
{
	//Тупой проброс вызова.
	if (this->imageIsLoaded)
	{
		this->UpdateSettings();
		return this->pFilterObj->SaveImage(fileName, errObj);
	}
	else
	{
		if (errObj) errObj->SetError(CMNERR_FILE_NOT_LOADED);
		return false;
	}
}

void MedianFilterUniversal::FillMargins(CallBackBase *callBackObj)
//Заполняет граничные (пустые пиксели) области вокруг значимых пикселей в соответствии с
//выбранным алгоритмом.
{
	//Проброс вызова.
	if (this->imageIsLoaded)
	{
		this->UpdateSettings();
		this->pFilterObj->FillMargins(callBackObj);
	}		
}

void MedianFilterUniversal::ApplyStupidFilter(CallBackBase *callBackObj)
//Обрабатывает матрицу sourceMatrix_ "тупым" фильтром. Результат записывает в destMatrix_.
{
	//Проброс вызова
	if (this->imageIsLoaded)
	{
		this->UpdateSettings();
		this->pFilterObj->ApplyStupidFilter(callBackObj);
	}		
}


void MedianFilterUniversal::ApplyStubFilter(CallBackBase *callBackObj)
//Обрабатывает матрицу sourceMatrix_ "никаким" фильтром. По сути просто копирование.
//Для отладки. Результат записывает в destMatrix_.
{
	//Проброс вызова
	if (this->imageIsLoaded)
	{
		this->UpdateSettings();
		this->pFilterObj->ApplyStubFilter(callBackObj);
	}
}

void MedianFilterUniversal::SourcePrintStupidVisToCout()
//"Тупая" визуализация матрицы, отправляется прямо в cout.
{
	//Просто проброс вызова в объект матрицы.
	this->pFilterObj->SourcePrintStupidVisToCout();
}

bool MedianFilterUniversal::SourceSaveToCSVFile(const std::string &fileName, ErrorInfo *errObj)
//Вывод исходной матрицы в csv-файл, который должны понимать всякие картографические
//программы. Это значит что каждый пиксел - это одна строка в файле.
//Это "тупой" вариант вывода - метаданные нормально не сохраняются.
{
	//Просто проброс вызова в объект матрицы.
	return this->pFilterObj->SourceSaveToCSVFile(fileName, errObj);
}

bool MedianFilterUniversal::DestSaveToCSVFile(const std::string &fileName, ErrorInfo *errObj)
//Аналогично SourceSaveToCSVFile, но для матрицы с результатом.
{
	//Просто проброс вызова в объект матрицы.
	return this->pFilterObj->DestSaveToCSVFile(fileName, errObj);
}

} //namespace geoimgconv