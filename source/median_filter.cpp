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

//Сделать шаг по пиксельным координатам в указанном направлении.
//Вернёт false если координаты ушли за границы изображения, иначе true.
template <typename CellType>
bool RealMedianFilterTemplBase<CellType>::PixelStep(int &x, int &y, const PixelDirection direction)
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

//Заполнять пиксели простым алгоритмом в указанном направлении
template <typename CellType>
void RealMedianFilterTemplBase<CellType>::SimpleFiller(const int &x, const int &y,
	const PixelDirection direction, const int &marginSize)
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

//Заполнять пиксели зеркальным алгоритмом в указанном направлении
template <typename CellType>
void RealMedianFilterTemplBase<CellType>::MirrorFiller(const int &x, const int &y,
	const PixelDirection direction, const int &marginSize)
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

//Костяк алгоритма, общий для Simple и Mirror
template <typename CellType>
void RealMedianFilterTemplBase<CellType>::FillMargins_PixelBasedAlgo(const PixFillerMethod FillerMethod,
	CallBackBase *callBackObj)
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

//Заполнить пустые пиксели source-матрицы простым алгоритмом (сплошной цвет).
template <typename CellType>
void RealMedianFilterTemplBase<CellType>::FillMargins_Simple(CallBackBase *callBackObj)
{
	FillMargins_PixelBasedAlgo(&RealMedianFilterTemplBase<CellType>::SimpleFiller,
		callBackObj);
}

//Заполнить пустые пиксели source-матрицы зеркальным алгоритмом.
template <typename CellType>
void RealMedianFilterTemplBase<CellType>::FillMargins_Mirror(CallBackBase *callBackObj)
{
	FillMargins_PixelBasedAlgo(&RealMedianFilterTemplBase<CellType>::MirrorFiller,
		callBackObj);
}

//Применит указанный (ссылкой на метод) фильтр к изображению. Входящий и исходящий файлы
//уже должны быть подключены. Вернёт false и инфу об ошибке если что-то пойдёт не так.
template <typename CellType>
bool RealMedianFilterTemplBase<CellType>::ApplyFilter(FilterMethod CurrFilter,
	CallBackBase *callBackObj, ErrorInfo *errObj)
{
	//Задаём размер матриц.
	getOwnerObj().FixAperture();
	int marginSize = (getOwnerObj().getAperture() - 1) / 2;
	int sourceYSize, destYSize;
	if (getOwnerObj().getUseMemChunks())
	{
		//Размер по Y зависит от режима использования памяти.
		sourceYSize = getOwnerObj().getBlocksInMem() * marginSize;
		destYSize = (getOwnerObj().getBlocksInMem() - 2) * marginSize;
	}
	else
	{
		sourceYSize = getOwnerObj().getImageSizeY() + (2 * marginSize);
		destYSize = getOwnerObj().getImageSizeY();
	}
	sourceMatrix_.CreateEmpty(getOwnerObj().getImageSizeX() + (2 * marginSize), sourceYSize);
	//destMatrix будет создаваться и обнуляться уже в процессе, перед запуском очередного прохода фильтра.	

	//Настроим прогрессбар.
	if (callBackObj)
		callBackObj->setMaxProgress(getOwnerObj().getImageSizeX() * getOwnerObj().getImageSizeY());
	
	//Теперь в цикле надо пройти по всем строкам исходного файла.
	int currYToProcess, filterYToProcess;
	bool needRecalc = true;
	int currBlocksToProcess = getOwnerObj().getBlocksInMem() - 1;
	//int debugFileNum = 1;	//Для отладочного сохранения.
	int writeYPosition = 0;
	for (getOwnerObj().setCurrPositionY(0); getOwnerObj().getCurrPositionY()
		< getOwnerObj().getImageSizeY(); getOwnerObj().setCurrPositionY(
			getOwnerObj().getCurrPositionY() + currYToProcess))
	{
		//Если надо - вычислим число строк для обработки.
		if (needRecalc)
		{
			if (getOwnerObj().getCurrPositionY() == 0)
			{
				//Это первая итерация.
				if (getOwnerObj().getUseMemChunks() == false)
				{
					//И единственная итерация. Всё обрабатывается одним куском.
					currYToProcess = getOwnerObj().getImageSizeY();
					filterYToProcess = currYToProcess;
				}
				else
				{
					//Вероятно будут ещё итерации. В первый раз читаем на 1 блок больше.
					currYToProcess = currBlocksToProcess * marginSize;
					//Но возможно мы сейчас прочитаем всё и итерация всё же последняя.
					int nextY = getOwnerObj().getCurrPositionY() + currYToProcess;
					if (nextY >= getOwnerObj().getImageSizeY())
						currYToProcess = getOwnerObj().getImageSizeY();
					//В любом случае читать дальше на 1 блок меньше.
					currBlocksToProcess--;
					filterYToProcess = currBlocksToProcess * marginSize;;
				}
			}
			else
			{
				//Не первая итерация.
				currYToProcess = currBlocksToProcess * marginSize;
				filterYToProcess = currYToProcess;
				int nextY = getOwnerObj().getCurrPositionY() + currYToProcess;
				if (nextY > getOwnerObj().getImageSizeY())
				{
					//Последняя итерация в которой строк будет меньше.
					currYToProcess -= nextY - getOwnerObj().getImageSizeY();
					//Количество строк для обработки возможно тоже надо уменьшить если разница
					//превышает 1 блок - именно на эту разницу.
					if ((filterYToProcess - currYToProcess) > marginSize)
						filterYToProcess -= (filterYToProcess - currYToProcess) - marginSize;
				}
				//Проверим нужен ли в следующей итерации пересчёт.
				nextY += currYToProcess;
				if (nextY > getOwnerObj().getImageSizeY())
					needRecalc = true;
				else
					needRecalc = false;	//Пересчёт в следующей итерации точно не потребуется.
			}
		}
		else
		{
			//В этой итерации пересчёт не нужен, но он может потребоваться в следующей.
			int nextY = getOwnerObj().getCurrPositionY() + currYToProcess*2;
			if (nextY > getOwnerObj().getImageSizeY())
				needRecalc = true;
		}

		//Теперь надо загрузить новые пиксели в исходную матрицу и обнулить целевую матрицу.
		TopMarginMode currMM;
		AltMatrix<CellType> *currMatrix;
		if (getOwnerObj().getCurrPositionY() == 0)
		{
			currMM = TOP_MM_FILE1;
			currMatrix = NULL;
		}
		else
		{
			currMM = TOP_MM_MATR;
			currMatrix = &sourceMatrix_;
		}
		if (!sourceMatrix_.LoadFromGDALRaster(getOwnerObj().getGdalSourceRaster(),
			getOwnerObj().getCurrPositionY(), currYToProcess, marginSize, currMM,
			currMatrix, errObj))
		{
			return false;
		}
		destMatrix_.CreateDestMatrix(sourceMatrix_, marginSize);

		//Надо обработать граничные пиксели
		//TODO.

		//Надо применить фильтр
		//TODO.
		(this->*CurrFilter)(filterYToProcess,callBackObj);

		//Сохраняем то что получилось.
		if (!destMatrix_.SaveToGDALRaster(getOwnerObj().getGdalDestRaster(),
			writeYPosition, filterYToProcess, errObj))
		{
			return false;
		}

		//Для отладки - сохраним содержимое матриц.
		//sourceMatrix_.SaveToCSVFile("source" + STB.IntToString(debugFileNum, 5) + ".csv");
		//destMatrix_.SaveToCSVFile("dest" + STB.IntToString(debugFileNum, 5) + ".csv");
		//debugFileNum++;

		//Вроде всё, итерация завершена.
		writeYPosition += filterYToProcess;
	}

	//Есть вероятность того что в последнем блоке в последней итерации было что-то
	//значимое. Если это так - надо выполнить ещё одну итерацию, без фактического чтения.
	if ((getOwnerObj().getCurrPositionY() - currYToProcess) == 0)
	{
		//Если была единственная итерация - надо скорректировать число блоков.
		currBlocksToProcess++;
	}
	if ((getOwnerObj().getUseMemChunks()) && (currYToProcess > ((currBlocksToProcess - 1)*marginSize)))
	{
		filterYToProcess = currYToProcess - ((currBlocksToProcess - 1)*marginSize);
		//"читаем" данные в матрицу, на самом деле просто копируем 2 нижних блока наверх.
		sourceMatrix_.LoadFromGDALRaster(getOwnerObj().getGdalSourceRaster(), 0,
			0, marginSize, TOP_MM_MATR, &sourceMatrix_, NULL);
		destMatrix_.CreateDestMatrix(sourceMatrix_, marginSize);
		//Всё ещё надо обработать граничные пиксели
		//TODO.
		//Всё ещё надо применить фильтр.
		//TODO.
		(this->*CurrFilter)(filterYToProcess,callBackObj);

		//Сохраняем то что получилось.
		if (!destMatrix_.SaveToGDALRaster(getOwnerObj().getGdalDestRaster(),
			writeYPosition, filterYToProcess, errObj))
		{
			return false;
		}

		//Для отладки - сохраним содержимое матриц.
		//sourceMatrix_.SaveToCSVFile("source" + STB.IntToString(debugFileNum, 5) + ".LASTBLOCK.csv");
		//destMatrix_.SaveToCSVFile("dest" + STB.IntToString(debugFileNum, 5) + ".LASTBLOCK.csv");
	}

	//Вроде всё ок.
	return true;
}

//Метод "тупого" фильтра, который тупо копирует входящую матрицу в исходящую. Нужен для тестирования
//и отладки. Первый аргумент указывает количество строк матрицы для реальной обработки.
template <typename CellType>
void RealMedianFilterTemplBase<CellType>::StubFilter(const int &currYToProcess,
	CallBackBase *callBackObj)
{
	int destX, destY, sourceX, sourceY, marginSize;
	marginSize = (getOwnerObj().getAperture() - 1) / 2;
	unsigned long progressPosition = getOwnerObj().getCurrPositionY()*destMatrix_.getXSize();

	//Поехали.
	for (destY = 0; destY < currYToProcess; destY++)
	{
		sourceY = destY + marginSize;
		for (destX = 0; destX < destMatrix_.getXSize(); destX++)
		{
			sourceX = destX + marginSize;
			progressPosition++;
			//Незначимые пиксели не трогаем.
			if (sourceMatrix_.getSignMatrixElem(sourceY, sourceX) != 1) continue;

			//Просто копируем пиксел.
			destMatrix_.setMatrixElem(destY, destX,
				sourceMatrix_.getMatrixElem(sourceY, sourceX));

			//Сообщить вызвавшему коду прогресс выполнения.
			if (callBackObj) callBackObj->CallBack(progressPosition);
		};
	};
}

//Метод для обработки матрицы "тупым" фильтром, котороый действует практически в лоб.
//Первый аргумент указывает количество строк матрицы для реальной обработки.
template <typename CellType>
void RealMedianFilterTemplBase<CellType>::StupiudFilter(const int &currYToProcess,
	CallBackBase *callBackObj)
{
	//Заглушка.
}

//--------------------------------//
//       Прочий функционал        //
//--------------------------------//

//Читает изображение в матрицу так чтобы по краям оставалось место для создания граничных
//пикселей.
template <typename CellType>
bool RealMedianFilterTemplBase<CellType>::LoadImage(const std::string &fileName, ErrorInfo *errObj,
	CallBackBase *callBackObj)
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

//Сохраняет матрицу в изображение. За основу берётся ранее загруженная через LoadImage
//картинка - файл копируется под новым именем и затем в него вносятся изменённые пиксели.
//В первую очередь это нужно чтобы оставить метаданные в неизменном оригинальном виде.
template <typename CellType>
bool  RealMedianFilterTemplBase<CellType>::SaveImage(const std::string &fileName, ErrorInfo *errObj)
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

//Заполняет граничные (пустые пиксели) области вокруг значимых пикселей в соответствии с
//выбранным алгоритмом.
template <typename CellType>
void RealMedianFilterTemplBase<CellType>::FillMargins(CallBackBase *callBackObj)
{
	switch (getOwnerObj().getMarginType())
	{
	case MARGIN_SIMPLE_FILLING: FillMargins_Simple(callBackObj); break;
	case MARGIN_MIRROR_FILLING: FillMargins_Mirror(callBackObj);
	}		
}

//Обрабатывает матрицу sourceMatrix_ "тупым" фильтром. Результат записывает в destMatrix_.
template <typename CellType>
void RealMedianFilterTemplBase<CellType>::ApplyStupidFilter_old(CallBackBase *callBackObj)
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

//Обрабатывает матрицу sourceMatrix_ "никаким" фильтром. По сути просто копирование.
//Для отладки. Результат записывает в destMatrix_.
template <typename CellType>
void RealMedianFilterTemplBase<CellType>::ApplyStubFilter_old(CallBackBase *callBackObj)
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

//Обрабатывает выбранный исходный файл "никаким" фильтром. По сути это просто копирование.
//Для отладки. Результат записывается в выбранный destFile
template <typename CellType>
bool RealMedianFilterTemplBase<CellType>::ApplyStubFilter(CallBackBase *callBackObj, ErrorInfo *errObj)
{
	//Новый вариант "никакого" фильтра. Работающий по кускам. Просто вызываем уже готовый
	//метод, передав ему нужный фильтрующий метод.
	return ApplyFilter(&RealMedianFilterTemplBase<CellType>::StubFilter, callBackObj, errObj);
}

//"Тупая" визуализация матрицы, отправляется прямо в cout.
template <typename CellType>
void RealMedianFilterTemplBase<CellType>::SourcePrintStupidVisToCout()
{
	//Просто проброс вызова в объект матрицы.
	sourceMatrix_.PrintStupidVisToCout();
}

//Вывод исходной матрицы в csv-файл, который должны понимать всякие картографические
//программы. Это значит что каждый пиксел - это одна строка в файле.
//Это "тупой" вариант вывода - метаданные нормально не сохраняются.
template <typename CellType>
bool RealMedianFilterTemplBase<CellType>::SourceSaveToCSVFile(const std::string &fileName, ErrorInfo *errObj)
{
	//Просто проброс вызова в объект матрицы.
	return sourceMatrix_.SaveToCSVFile(fileName, errObj);
}

//Аналогично SourceSaveToCSVFile, но для матрицы с результатом.
template <typename CellType>
bool RealMedianFilterTemplBase<CellType>::DestSaveToCSVFile(const std::string &fileName, ErrorInfo *errObj)
{
	//Просто проброс вызова в объект матрицы.
	return destMatrix_.SaveToCSVFile(fileName, errObj);
}

////////////////////////////////////
//          MedianFilter          //
////////////////////////////////////

MedianFilter::MedianFilter() : aperture_(DEFAULT_APERTURE), threshold_(DEFAULT_THRESHOLD),
marginType_(DEFAULT_MARGIN_TYPE), useMemChunks_(false), blocksInMem_(0), sourceFileName_(""),
destFileName_(""), imageSizeX_(0), imageSizeY_(0), imageIsLoaded_(false), sourceIsAttached_(false),
destIsAttached_(false), dataType_(PIXEL_UNKNOWN), dataTypeSize_(0), pFilterObj_(NULL),
minBlockSize_(0), minMemSize_(0), gdalSourceDataset_(NULL), gdalDestDataset_(NULL), gdalSourceRaster_(NULL),
gdalDestRaster_(NULL), currPositionY_(0)
{
	
}

MedianFilter::~MedianFilter()
{
	//Возможно создавался объект реального фильтра. Надо удалить.
	delete pFilterObj_;
	//Возможно нужно закрыть подключённые файлы и уничтожить объекты GDAL
	CloseAllFiles();
}

//--------------------------//
//     Приватные методы     //
//--------------------------//

//Вычислить минимальные размеры блоков памяти, которые нужны для принятия решения о
//том сколько памяти разрешено использовать медианному фильтру в процессе своей работы.
void MedianFilter::CalcMemSizes()
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
	//Размер пиксела в исходного блока в байтах у нас складывается из размера типа
	//элементов в матрице и размера элемента вспомогательной матрицы (это 1 байт).
	//
	//Размер исходного блока.
	/*unsigned long long minSourceBlockSize = (2 * firstLastBlockHeight*blockWidth +
		blockHeight * blockWidth) * (dataTypeSize_ + 1);*/
	unsigned long long minSourceBlockSize = blockHeight * blockWidth * (dataTypeSize_ + 1);
	//Размер блока с результатом
	unsigned long long minDestBlockSize = imageSizeX_ * firstLastBlockHeight * dataTypeSize_;
	//Минимальное допустимое количество памяти.
	minMemSize_ = (3 * minSourceBlockSize) + minDestBlockSize;
	//Обобщённый размер "блока", содержащего и исходные данные и результат.
	minBlockSize_ = minSourceBlockSize + minDestBlockSize;
	//Общее количество памяти, которое может потребоваться для для работы над изображением.
	/*maxMemSize_ = (((2 * minBlockSize_) + (imageSizeX_ * imageSizeY_) + (firstLastBlockHeight * imageSizeY_)) *
		(dataTypeSize_ + 1)) + (imageSizeX_ * imageSizeY_ * dataTypeSize_);*/
	maxMemSize_ = (2 * minSourceBlockSize) +		//2 граничных source-блока
		(imageSizeX_ * imageSizeY_ * dataTypeSize_) +	//dest-матрица
		(blockWidth * imageSizeY_ * (dataTypeSize_ + 1));	//остальная source-матрица.

}

//--------------------------//
//      Прочие методы       //
//--------------------------//

//Выбрать исходный файл для дальнейшего чтения и обработки. Получает информацию о параметрах изображения,
//запоминает её в полях объекта.
bool MedianFilter::OpenInputFile(const std::string &fileName, ErrorInfo *errObj)
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
		if (errObj) errObj->SetError(CMNERR_INTERNAL_ERROR, ": MedianFilter::OpenInputFile() попытка открыть файл при не закрытом старом." );
		return false;
	}

	//А был ли файл?
	filesystem::path filePath = STB.Utf8ToWstring(fileName);
	if (!filesystem::is_regular_file(filePath))
	{
		if (errObj)	errObj->SetError(CMNERR_FILE_NOT_EXISTS, ": " + fileName);
		return false;
	}

	//Открываем картинку, определяем тип и размер данных.
	gdalSourceDataset_ = (GDALDataset*)GDALOpen(fileName.c_str(), GA_ReadOnly);
	if (!gdalSourceDataset_)
	{
		if (errObj) errObj->SetError(CMNERR_READ_ERROR, ": " + fileName);
		return false;
	}
	if (gdalSourceDataset_->GetRasterCount() != 1)
	{
		//Если в картинке слоёв не строго 1 штука - это какая-то непонятная картинка,
		//т.е. не похожа на карту высот, возможно там RGB или ещё что подобное...
		//Так что облом и ругаемся.
		GDALClose(gdalSourceDataset_);
		gdalSourceDataset_ = NULL;
		if (errObj) errObj->SetError(CMNERR_UNSUPPORTED_FILE_FORMAT, ": " + fileName);
		return false;
	}
	gdalSourceRaster_ = gdalSourceDataset_->GetRasterBand(1);
	dataType_ = GDALToGIC_PixelType(gdalSourceRaster_->GetRasterDataType());
	imageSizeX_ = gdalSourceRaster_->GetXSize();
	imageSizeY_ = gdalSourceRaster_->GetYSize();
	if (dataType_ == PIXEL_UNKNOWN)
	{
		GDALClose(gdalSourceDataset_);
		gdalSourceDataset_ = NULL;
		gdalSourceRaster_ = NULL;
		if (errObj) errObj->SetError(CMNERR_UNSUPPORTED_FILE_FORMAT, ": " + fileName);
		return false;
	}

	//Если GDAL говорит что тип пикселя - байт - надо посмотреть метаданные какой именно там
	//тип байтов.
	if (dataType_ == PIXEL_INT8)
	{
		const char *tempStr = gdalSourceRaster_->GetMetadataItem("PIXELTYPE", "IMAGE_STRUCTURE");
		if ((tempStr == NULL) || strcmp(tempStr, "SIGNEDBYTE"))
		{
			//Это явно не signed-байт
			dataType_ = PIXEL_UINT8;
		}
	}

	//Тип данных определён, надо создать вложенный объект нужного типа и дальше все вызовы транслировать
	//уже в него.
	delete pFilterObj_;
	switch (dataType_)
	{
		case PIXEL_INT8:
			pFilterObj_ = new RealMedianFilterInt8(this);
			dataTypeSize_ = sizeof(boost::int8_t);
			break;
		case PIXEL_UINT8:
			pFilterObj_ = new RealMedianFilterUInt8(this);
			dataTypeSize_ = sizeof(boost::uint8_t);
			break;
		case PIXEL_INT16:
			pFilterObj_ = new RealMedianFilterInt16(this);
			dataTypeSize_ = sizeof(boost::int16_t);
			break;
		case PIXEL_UINT16:
			pFilterObj_ = new RealMedianFilterUInt16(this);
			dataTypeSize_ = sizeof(boost::uint16_t);
			break;
		case PIXEL_INT32:
			pFilterObj_ = new RealMedianFilterInt32(this);
			dataTypeSize_ = sizeof(boost::int32_t);
			break;
		case PIXEL_UINT32:
			pFilterObj_ = new RealMedianFilterUInt32(this);
			dataTypeSize_ = sizeof(boost::uint32_t);
			break;
		case PIXEL_FLOAT32:
			pFilterObj_ = new RealMedianFilterFloat32(this);
			dataTypeSize_ = sizeof(float);
			break;
		case PIXEL_FLOAT64:
			pFilterObj_ = new RealMedianFilterFloat64(this);
			dataTypeSize_ = sizeof(double);
			break;
		default: 
			pFilterObj_ = NULL;
			dataTypeSize_ = 0;
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
		if (errObj) errObj->SetError(CMNERR_UNKNOWN_ERROR, "MedianFilter::OpenInputFile() error creating pFilterObj_!",true);
		gdalSourceDataset_ = NULL;
		gdalSourceRaster_ = NULL;
		return false;
	}
}

//Подготовить целевой файл к записи в него результата. Если forceRewrite==true - перезапишет уже
//существующий файл. Иначе вернёт ошибку (false и инфу в errObj). Input-файл уже должен быть открыт.
bool MedianFilter::OpenOutputFile(const std::string &fileName, const bool &forceRewrite, ErrorInfo *errObj)
{
	//Этот метод можно вызывать только если исходный файл уже был открыт.
	if (!sourceIsAttached_)
	{
		if (errObj) errObj->SetError(CMNERR_INTERNAL_ERROR, ":  MedianFilter::OpenOutputFile исходный файл не был открыт.");
		return false;
	}
	//И только если файл назначение открыт наоборот ещё не был.
	if (destIsAttached_)
	{
		if (errObj) errObj->SetError(CMNERR_INTERNAL_ERROR, ":  MedianFilter::OpenOutputFile попытка открыть файл назначения при уже открытом файле назначения.");
		return false;
	}
	
	//Готовим пути.
	filesystem::path destFilePath, sourceFilePath;
	destFilePath = STB.Utf8ToWstring(fileName);
	sourceFilePath = STB.Utf8ToWstring(sourceFileName_);

	//Проверяем есть ли уже такой файл.
	system::error_code errCode;
	if (filesystem::exists(destFilePath, errCode))
	{
		if (!forceRewrite)
		{
			//Запрещено перезаписывать файл, а он существует. Это печально :(.
			if(errObj) errObj->SetError(CMNERR_FILE_EXISTS_ALREADY, ": "+ fileName);
			return false;
		}
		//Удаляем старый файл.
		if (!filesystem::remove(destFilePath, errCode))
		{
			if (errObj) errObj->SetError(CMNERR_WRITE_ERROR, ": " + errCode.message());
			return false;
		}
	}

	//Всё, файла быть не должно. Копируем исходный файл.
	filesystem::copy_file(sourceFilePath, destFilePath, errCode);
	if (errCode.value())
	{
		if (errObj) errObj->SetError(CMNERR_WRITE_ERROR, ": " + errCode.message());
		return false;
	}

	//Теперь открываем в GDAL то что получилось.
	gdalDestDataset_ = (GDALDataset*)GDALOpen(fileName.c_str(), GA_Update);
	if (!gdalDestDataset_)
	{
		if (errObj)	errObj->SetError(CMNERR_WRITE_ERROR, ": " + fileName);
		return false;
	}
	gdalDestRaster_ = gdalDestDataset_->GetRasterBand(1);

	//Готово.
	destIsAttached_ = true;
	return true;
}

//Закрыть исходный файл.
void MedianFilter::CloseInputFile()
{
	if (sourceIsAttached_)
	{
		GDALClose(gdalSourceDataset_);
		sourceIsAttached_ = false;
	}
	gdalSourceDataset_ = NULL;
	gdalSourceRaster_ = NULL;
}

//Закрыть файл назначения.
void MedianFilter::CloseOutputFile()
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
void MedianFilter::CloseAllFiles()
{
	CloseInputFile();
	CloseOutputFile();
}


bool MedianFilter::LoadImage(const std::string &fileName, ErrorInfo *errObj,
	CallBackBase *callBackObj)
	//Читает изображение в матрицу так чтобы по краям оставалось место для создания граничных
	//пикселей.
{
	FixAperture();
	if (OpenInputFile(fileName, errObj))
	{
		imageIsLoaded_ = pFilterObj_->LoadImage(fileName, errObj, callBackObj);
	}
	
	return imageIsLoaded_;
}

//Сохраняет матрицу в изображение. За основу берётся ранее загруженная через LoadImage
//картинка - файл копируется под новым именем и затем в него вносятся изменённые пиксели.
//В первую очередь это нужно чтобы оставить метаданные в неизменном оригинальном виде.
bool MedianFilter::SaveImage(const std::string &fileName, ErrorInfo *errObj)
{
	//Тупой проброс вызова.
	if (imageIsLoaded_)
	{
		return pFilterObj_->SaveImage(fileName, errObj);
	}
	else
	{
		if (errObj) errObj->SetError(CMNERR_FILE_NOT_LOADED);
		return false;
	}
}

//Заполняет граничные (пустые пиксели) области вокруг значимых пикселей в соответствии с
//выбранным алгоритмом.
void MedianFilter::FillMargins(CallBackBase *callBackObj)
{
	//Проброс вызова.
	if (imageIsLoaded_)
	{
		pFilterObj_->FillMargins(callBackObj);
	}
}

//Обрабатывает матрицу sourceMatrix_ "тупым" фильтром. Результат записывает в destMatrix_.
void MedianFilter::ApplyStupidFilter_old(CallBackBase *callBackObj)
{
	//Проброс вызова
	if (imageIsLoaded_)
	{
		pFilterObj_->ApplyStupidFilter_old(callBackObj);
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

//Обрабатывает матрицу sourceMatrix_ "никаким" фильтром. По сути просто копирование.
//Для отладки. Результат записывает в destMatrix_.
void MedianFilter::ApplyStubFilter_old(CallBackBase *callBackObj)
{
	//Проброс вызова
	if (imageIsLoaded_)
	{
		pFilterObj_->ApplyStubFilter_old(callBackObj);
	}
}

//Обрабатывает выбранный исходный файл "никаким" фильтром. По сути это просто копирование.
//Для отладки. Результат записывается в выбранный destFile
bool MedianFilter::ApplyStubFilter(CallBackBase *callBackObj, ErrorInfo *errObj)
{
	//Проброс вызова.
	if (sourceIsAttached_ || destIsAttached_)
	{
		return pFilterObj_->ApplyStubFilter(callBackObj, errObj);
	}
	else
	{
		if (errObj) errObj->SetError(CMNERR_INTERNAL_ERROR, ": MedianFilter::ApplyStubFilter no source and\\or dest \
file(s) were attached.");
		return false;
	}
}

//"Тупая" визуализация матрицы, отправляется прямо в cout.
void MedianFilter::SourcePrintStupidVisToCout()
{
	//Просто проброс вызова в объект матрицы.
	pFilterObj_->SourcePrintStupidVisToCout();
}

//Вывод исходной матрицы в csv-файл, который должны понимать всякие картографические
//программы. Это значит что каждый пиксел - это одна строка в файле.
//Это "тупой" вариант вывода - метаданные нормально не сохраняются.
bool MedianFilter::SourceSaveToCSVFile(const std::string &fileName, ErrorInfo *errObj)
{
	//Просто проброс вызова в объект матрицы.
	return pFilterObj_->SourceSaveToCSVFile(fileName, errObj);
}

//Аналогично SourceSaveToCSVFile, но для матрицы с результатом.
bool MedianFilter::DestSaveToCSVFile(const std::string &fileName, ErrorInfo *errObj)
{
	//Просто проброс вызова в объект матрицы.
	return pFilterObj_->DestSaveToCSVFile(fileName, errObj);
}

} //namespace geoimgconv