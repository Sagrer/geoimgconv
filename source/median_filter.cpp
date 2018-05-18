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
#include "common.h"

using namespace boost;
using namespace std;

namespace geoimgconv
{

//Константы со значениями по умолчанию см. в common.cpp

////////////////////////////////////////////
//           RealMedianFilter             //
////////////////////////////////////////////

//Нельзя создать объект не дав ссылку на MedianFilterBase
template <typename CellType>
RealMedianFilter<CellType>::RealMedianFilter(MedianFilterBase *ownerObj) :
	RealMedianFilterBase(ownerObj),	sourceMatrix_(true, ownerObj->getUseHuangAlgo()),
	destMatrix_(false,false), minMaxCalculated_(false), noDataPixelValue_(0),
	minPixelValue_(0), maxPixelValue_(0), levelsDelta_(0.0)
{

};

//--------------------------------//
//       Приватные методы         //
//--------------------------------//

//Сделать шаг по пиксельным координатам в указанном направлении.
//Вернёт false если координаты ушли за границы изображения, иначе true.
template <typename CellType>
bool RealMedianFilter<CellType>::PixelStep(int &x, int &y, const PixelDirection direction)
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
void RealMedianFilter<CellType>::GetMirrorPixel(const int &x, const int &y, const int &currX, const int &currY, int &outX, int &outY, const int recurseDepth)
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
void RealMedianFilter<CellType>::SimpleFiller(const int &x, const int &y,
	const PixelDirection direction, const int &marginSize, const char &signMatrixValue)
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
		else if ((sourceMatrix_.getSignMatrixElem(currY, currX) == 0) ||
			(sourceMatrix_.getSignMatrixElem(currY, currX) > signMatrixValue))
		{
			//Заполняем либо вообще не заполненные пиксели, либо любые незначимые
			//с более низким приоритетом.
			sourceMatrix_.setMatrixElem(currY,currX,sourceMatrix_.getMatrixElem(y,x));
			sourceMatrix_.setSignMatrixElem(currY,currX,signMatrixValue); //Заполненный незначимый пиксель.
		}
	}
}

//Заполнять пиксели зеркальным алгоритмом в указанном направлении
template <typename CellType>
void RealMedianFilter<CellType>::MirrorFiller(const int &x, const int &y,
	const PixelDirection direction, const int &marginSize, const char &signMatrixValue)
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
			(sourceMatrix_.getSignMatrixElem(currY, currX) > signMatrixValue))
		{
			//Заполняем либо вообще не заполненные пиксели, либо любые незначимые
			//с более низким приоритетом.
			GetMirrorPixel(x,y,currX,currY,mirrorX,mirrorY);	//Координаты зеркального пикселя.
			sourceMatrix_.setMatrixElem(currY,currX,sourceMatrix_.getMatrixElem(mirrorY,mirrorX));
			sourceMatrix_.setSignMatrixElem(currY,currX,signMatrixValue); //Заполненный незначимый пиксель.
		}
	}
}

//Костяк алгоритма, общий для Simple и Mirror
template <typename CellType>
void RealMedianFilter<CellType>::FillMargins_PixelBasedAlgo(const PixFillerMethod FillerMethod,
	const int yStart, const int yToProcess, CallBackBase *callBackObj)
{
	//Двигаемся построчно, пока не найдём значимый пиксель. В границы не лезем,
	//т.к. они значимыми быть не могут.
	int x, y, marginSize;
	marginSize = (getOwnerObj().getAperture() - 1) / 2;
	//Настроим объект, выводящий информацию о прогрессе обработки.
	if (callBackObj)
		callBackObj->setMaxProgress((sourceMatrix_.getXSize() - marginSize * 2) *
			(sourceMatrix_.getYSize() - marginSize*2));
	unsigned long progressPosition = 0;
	//Поехали.
	for (y = yStart; y < (yStart + yToProcess); y++)
	{
		for (x = marginSize; x < (sourceMatrix_.getXSize() - marginSize); x++)
		{
			progressPosition++;
			if (sourceMatrix_.getSignMatrixElem(y, x) == 1)
			{
				//Теперь будем проверять значимы ли пикселы вверху, внизу, справа, слева и
				//по всем диагоналям. Если незначимы (независимо от фактической заполненности)
				//- запускается процесс заполнения, который будет двигаться и заполнять пиксели
				//в данном направлении пока не достигнет края картинки либо пока не встретит
				//значимый пиксель. При этом вертикальное и горизонтальное заполнение имеет
				//приоритет над диагональным, т.е. диагональные пикселы будут перезаписаны если
				//встретятся на пути. Поехали.

				(this->*FillerMethod)(x, y, PIXEL_DIR_UP, marginSize, 2);
				(this->*FillerMethod)(x, y, PIXEL_DIR_DOWN, marginSize, 2);
				(this->*FillerMethod)(x, y, PIXEL_DIR_RIGHT, marginSize, 2);
				(this->*FillerMethod)(x, y, PIXEL_DIR_LEFT, marginSize, 2);
				(this->*FillerMethod)(x, y, PIXEL_DIR_UP_RIGHT, marginSize, 3);
				(this->*FillerMethod)(x, y, PIXEL_DIR_UP_LEFT, marginSize, 3);
				(this->*FillerMethod)(x, y, PIXEL_DIR_DOWN_RIGHT, marginSize, 3);
				(this->*FillerMethod)(x, y, PIXEL_DIR_DOWN_LEFT, marginSize, 3);
				//Сообщение о прогрессе.
				if (callBackObj) callBackObj->CallBack(progressPosition);
			}
		}
	}
}

//Алгоритм заполнения граничных пикселей по незначимым пикселям, общий для Simple и Mirror
template <typename CellType>
void RealMedianFilter<CellType>::FillMargins_EmptyPixelBasedAlgo(const PixFillerMethod FillerMethod,
	const int yStart, const int yToProcess, CallBackBase *callBackObj)
{
	//Двигаемся построчно слева направо затем справа налево и так далее. Для каждого
	//незначимого и ещё ничем не заполненного пикселя проверяем наличие на расстоянии
	//размера окна значимых пикселей. Если таковой был найден - значит данный незначимый пиксель
	//входит в окно того значимого пикселя и обязательно должен быть заполнен. Для этого
	//во-первых запоминаем значение найденного пикселя, во вторых ищем сначала по
	//вертикалям и горизонталям, затем по диагоналям ближайший значимый пиксель и от него
	//"ведём дорожку", заданную выбранным алгоритмом (т.е. заполняющим методом).
	//Если же по этим направлениям пиксель найти не удалось - просто заполняем его запомненным
	//ранее значимым, он в любом случае относительно близко. Пиксель, заполненные вертикальным
	//или горизонтальным способом имеют приоритет выше, чем заполненные диагональным и выше,
	//чем заполненные просто из запомненных. Если при "ведении дорожки" приоритет дорожки выше чем
	//приоритет незначимого пикселя на дорожке - то он перезаписывается.
	//При движении по строкам используется знание о том были ли значимые пиксели в предыдущем окне.
	//Если их не было то на очередном шаге достаточно проверить только 1 столбик или строку в
	//направлении движения.

	int x, y, yEnd, marginSize, windowX, windowY, windowXEnd, windowYEnd;
	int actualPixelY, actualPixelX, actualPixelDistance;
	CellType tempPixelValue;
	PixelDirection actualPixelDirection;
	marginSize = (getOwnerObj().getAperture() - 1) / 2;
	//Настроим объект, выводящий информацию о прогрессе обработки.
	if (callBackObj)
		callBackObj->setMaxProgress((sourceMatrix_.getXSize() - marginSize * 2) *
			(sourceMatrix_.getYSize() - marginSize*2));
	//Цикл, в каждой итерации которого обрабатывается до 2 строк:
	y = yStart;
	yEnd = yStart+yToProcess;
	//Если true то можно проверять только следующий столбик или строку.
	bool windowWasEmpty = false;
	//Координата начала проверяемого на значимость окна.
	windowY = y;
	//Координата следующая за последней для проверяемого окна.
	windowYEnd = y+marginSize+1;
	if (windowYEnd > yEnd) windowYEnd = yEnd;
	while (y < yEnd)
	{
		//По строке вправо.
		FillMargins_EmptyPixelBasedAlgo_ProcessToRight(y, x, windowY, windowX, windowYEnd, windowXEnd,
			actualPixelY, actualPixelX, actualPixelDirection, actualPixelDistance, tempPixelValue,
			windowWasEmpty, marginSize, FillerMethod);
		//Теперь смещаемся на пиксель вниз.
		++y;
		if ((y) > marginSize)
		{
			++windowY;
		}
		if (windowYEnd < sourceMatrix_.getYSize())
		{
			++windowYEnd;
		}
		//Вываливаемся из цикла если все строки обработаны.
		if (!(y < yEnd)) break;
		//Обрабатываем первый пиксел новой строки
		FillMargins_EmptyPixelBasedAlgo_ProcessNextPixel(y, x, windowY, windowX, windowYEnd, windowXEnd,
			actualPixelY, actualPixelX, actualPixelDistance, actualPixelDirection, PIXEL_DIR_DOWN,
			windowWasEmpty, tempPixelValue, FillerMethod);
		//Обрабатываем строку влево.
		FillMargins_EmptyPixelBasedAlgo_ProcessToLeft(y, x, windowY, windowX, windowYEnd, windowXEnd,
			actualPixelY, actualPixelX, actualPixelDirection, actualPixelDistance, tempPixelValue,
			windowWasEmpty, marginSize, FillerMethod);
		//И снова на пиксель вниз.
		++y;
		if ((y) > marginSize)
		{
			++windowY;
		}
		if (windowYEnd < sourceMatrix_.getYSize())
		{
			++windowYEnd;
		}
		//Обрабатываем первый пиксел новой строки
		if (y < yEnd)
		{
			FillMargins_EmptyPixelBasedAlgo_ProcessNextPixel(y, x, windowY, windowX, windowYEnd, windowXEnd,
				actualPixelY, actualPixelX, actualPixelDistance, actualPixelDirection, PIXEL_DIR_DOWN,
				windowWasEmpty, tempPixelValue, FillerMethod);
		}		
	}
}

//Вспомогательный метод для прохода по пикселам строки вправо.
template<typename CellType>
inline void RealMedianFilter<CellType>::FillMargins_EmptyPixelBasedAlgo_ProcessToRight(int &y, int &x,
	int &windowY, int &windowX,	int &windowYEnd, int &windowXEnd, int &actualPixelY, int &actualPixelX,
	PixelDirection &actualPixelDirection, int &actualPixelDistance, CellType &tempPixelValue,
	bool &windowWasEmpty, int marginSize, const PixFillerMethod FillerMethod)
{
	windowX = 0;
	windowXEnd = marginSize+1;
	if (windowXEnd > sourceMatrix_.getXSize())
	{
		windowXEnd = sourceMatrix_.getXSize();
	}
	for (x = 0; x < sourceMatrix_.getXSize(); ++x)
	{
		//Обработаем очередной пиксел.
		FillMargins_EmptyPixelBasedAlgo_ProcessNextPixel(y, x, windowY, windowX, windowYEnd, windowXEnd,
			actualPixelY, actualPixelX, actualPixelDistance, actualPixelDirection, PIXEL_DIR_RIGHT,
			windowWasEmpty, tempPixelValue, FillerMethod);

		//Конец итерации по очередному пикселу в строке. Теперь надо подкорректировать счётчики окна.
		if ((x+1) > marginSize)
		{
			++windowX;
		}
		if (windowXEnd < sourceMatrix_.getXSize())
		{
			++windowXEnd;
		}
	}
}

//Вспомогательный метод для прохода по пикселам строки влево.
template<typename CellType>
inline void RealMedianFilter<CellType>::FillMargins_EmptyPixelBasedAlgo_ProcessToLeft(int &y, int &x,
	int &windowY, int &windowX, int &windowYEnd, int &windowXEnd, int &actualPixelY, int &actualPixelX,
	PixelDirection &actualPixelDirection, int &actualPixelDistance, CellType &tempPixelValue,
	bool &windowWasEmpty, int marginSize, const PixFillerMethod FillerMethod)
{
	windowX = sourceMatrix_.getXSize()-1-marginSize;
	windowXEnd = sourceMatrix_.getXSize();
	if (windowX < 0)
	{
		windowX = 0;
	}
	for (x = sourceMatrix_.getXSize()-1; x >= 0; --x)
	{
		//Обработаем очередной пиксел.
		FillMargins_EmptyPixelBasedAlgo_ProcessNextPixel(y, x, windowY, windowX, windowYEnd, windowXEnd,
			actualPixelY, actualPixelX, actualPixelDistance, actualPixelDirection, PIXEL_DIR_LEFT,
			windowWasEmpty, tempPixelValue, FillerMethod);

		//Конец итерации по очередному пикселу в строке. Теперь надо подкорректировать счётчики окна.
		if ((sourceMatrix_.getXSize()-1-x) > marginSize)
		{
			--windowXEnd;
		}
		if (windowX > 0)
		{
			--windowX;
		}
	}
}

//Вспомогательный метод для обработки очередного незначимого пикселя при проходе
//в указанном направлении (currentDirection).
template<typename CellType>
inline void RealMedianFilter<CellType>::FillMargins_EmptyPixelBasedAlgo_ProcessNextPixel(int &y, int &x,
	int &windowY, int &windowX, int& windowYEnd, int &windowXEnd, int &actualPixelY, int &actualPixelX,
	int &actualPixelDistance, PixelDirection &actualPixelDirection,	const PixelDirection &currentDirection,
	bool &windowWasEmpty, CellType &tempPixelValue,	const PixFillerMethod FillerMethod)
{
	if (sourceMatrix_.getSignMatrixElem(y, x) != 1)
	{
		//Пиксел не значим. Обработаем его.
		//Проверим есть ли в окне вокруг значимые пиксели.
		windowWasEmpty = FillMargins_WindowIsEmpty(windowY, windowX, windowYEnd, windowXEnd,
			actualPixelY, actualPixelX, tempPixelValue, windowWasEmpty, currentDirection);
		if (!windowWasEmpty)
		{
			//В окне есть значимый пиксел - найдём подходящий значимый и нарисуем дорожку.
			if (FillMargins_FindNearestActualPixel(y, x, actualPixelY, actualPixelX,
				actualPixelDirection, actualPixelDistance))
			{
				//Пиксель найден. Заполним дорожку от него до текущего.
				char signValue;
				if ((actualPixelDirection == PIXEL_DIR_UP) ||
					(actualPixelDirection == PIXEL_DIR_DOWN) ||
					(actualPixelDirection == PIXEL_DIR_RIGHT) ||
					(actualPixelDirection == PIXEL_DIR_LEFT))
				{
					//Приоритет вертикальных и горизонтальных дорожек выше чем приоритет
					//диагональных дорожек.
					signValue = 2;
				}
				else
				{
					signValue = 3;
				}
				//Собственно, вызываем заполняющий метод. Направление должно быть противоположным
				//найденному, т.к. дорожку рисуем не к найденному пикселю а от него!
				(this->*FillerMethod)(actualPixelX, actualPixelY, RevertPixelDirection(actualPixelDirection),
					actualPixelDistance, signValue);
			}
			else
			{
				//Пиксел не найден, но он был в окне. Заполним это значение в матрицу значимости
				//с самым низким приоритетом.
				sourceMatrix_.setSignMatrixElem(y, x, 4);
			}
		}
	}
}

//Вспомогательный метод для проверки наличия в окне значимых пикселей. Вернёт true если
//значимых пикселей не нашлось. Если нашлось - то false и координаты со значением первого
//найденного пикселя (через не-константные ссылочные параметры). Если windowWasEmpty
//то воспользуется direction чтобы проверить только одну строчку или колонку, считая что
//остальное уже было проверено при предыдущем вызове этого метода.
template <typename CellType>
inline bool RealMedianFilter<CellType>::FillMargins_WindowIsEmpty(const int &windowY,
	const int &windowX, const int &windowYEnd, const int &windowXEnd, int &pixelY,
	int &pixelX, CellType &pixelValue, const bool &windowWasEmpty,
	const PixelDirection &direction) const
{
	//Счётчики.
	int y, x;

	//Смотрим, можно ли сэкономить.
	if (windowWasEmpty)
	{
		//Можно. Проверим только 1 новый столбик или строку.
		if (direction == PIXEL_DIR_RIGHT)
		{
			//Проверяем только правый столбик окна.
			x = windowXEnd-1;
			for (y = windowY; y < windowYEnd; ++y)
			{
				//Пробуем найти значимый пиксел.
				if (sourceMatrix_.getSignMatrixElem(y,x) == 1)
				{
					//Нашлось.
					pixelY = y;
					pixelX = x;
					pixelValue = sourceMatrix_.getMatrixElem(y,x);
					return false;
				};
			};
			//Не нашлось.
			return true;
		}
		else if (direction == PIXEL_DIR_LEFT)
		{
			//Проверяем только левый столбик окна.
			for (y = windowY; y < windowYEnd; ++y)
			{
				//Пробуем найти значимый пиксел.
				if (sourceMatrix_.getSignMatrixElem(y,windowX) == 1)
				{
					//Нашлось.
					pixelY = y;
					pixelX = windowX;
					pixelValue = sourceMatrix_.getMatrixElem(y,windowX);
					return false;
				};
			};
			//Не нашлось.
			return true;
		}
		else if (direction == PIXEL_DIR_DOWN)
		{
			//Проверяем только нижнюю строку окна.
			y = windowYEnd-1;
			for (x = windowX; x < windowXEnd; ++x)
			{
				//Пробуем найти значимый пиксел.
				if (sourceMatrix_.getSignMatrixElem(y,x) == 1)
				{
					//Нашлось.
					pixelY = y;
					pixelX = x;
					pixelValue = sourceMatrix_.getMatrixElem(y,x);
					return false;
				};
			};
			//Не нашлось.
			return true;
		};
	};

	//Нельзя сэкономить, окно придётся проверять целиком.
	for(y = windowY; y < windowYEnd; ++y)
	{
		for(x = windowX; x < windowXEnd; ++x)
		{
			//Пробуем найти значимый пиксел.
			if (sourceMatrix_.getSignMatrixElem(y,x) == 1)
			{
				//Нашлось.
				pixelY = y;
				pixelX = x;
				pixelValue = sourceMatrix_.getMatrixElem(y,x);
				return false;
			};
		};
	};

	//Не нашлось.
	return true;
}

//Вспомогательный метод для поиска ближайшего к указанному незначимому значимого пикселя. Сначала
//поиск выполняется по горизонтали и вертикали, только затем по диагоналям. Если найти пиксел не
//удалось - вернёт false. Если удалось вернёт true, координаты пикселя, направление и дистанцию
//на него.
template <typename CellType>
inline bool RealMedianFilter<CellType>::FillMargins_FindNearestActualPixel(const int &startPixelY,
	const int &startPixelX, int &resultPixelY, int &resultPixelX, PixelDirection &resultDirection,
	int &resultDistance)
{
	//Имеем 8 возможных направлений. Так что первый цикл будет по самим направлениям.
	bool searchDone = false;
	bool currPixelFinded = false;	//Найден ли пиксель в итерации.
	bool pixelFinded = false;	//Найден ли пиксель вообще.
	bool justStarted = true;	//<-- это для правильного выставления счётчиков в 1ю итерацию.
	PixelDirection currDirection;
	//Начальная найденная дистанция - за пределами картинки. Любая реальная будет меньше.
	if (sourceMatrix_.getXSize() < sourceMatrix_.getYSize())
	{
		resultDistance = sourceMatrix_.getYSize()+1;
	}
	else
	{
		resultDistance = sourceMatrix_.getXSize()+1;
	}
	int currResultDistance, currResultX, currResultY;

	while (!searchDone)
	{
		//Выберем направление. Можно было бы инкрементировать enum, но мало ли, вдруг
		//там порядок изменится. Так что вот так:
		if (justStarted)
		{
			justStarted = false;
			currDirection = PIXEL_DIR_UP;
		}
		else if (currDirection == PIXEL_DIR_UP)
		{
			currDirection = PIXEL_DIR_DOWN;
		}
		else if (currDirection == PIXEL_DIR_DOWN)
		{
			currDirection = PIXEL_DIR_RIGHT;
		}
		else if (currDirection == PIXEL_DIR_RIGHT)
		{
			currDirection = PIXEL_DIR_LEFT;
		}
		else if (currDirection == PIXEL_DIR_LEFT)
		{
			currDirection = PIXEL_DIR_UP_RIGHT;
		}
		else if (currDirection == PIXEL_DIR_UP_RIGHT)
		{
			currDirection = PIXEL_DIR_UP_LEFT;
		}
		else if (currDirection == PIXEL_DIR_UP_LEFT)
		{
			currDirection = PIXEL_DIR_DOWN_RIGHT;
		}
		else if (currDirection == PIXEL_DIR_DOWN_RIGHT)
		{
			currDirection = PIXEL_DIR_DOWN_LEFT;
		}

		//Ищем.
		currResultY = startPixelY;
		currResultX = startPixelX;
		currResultDistance = 0;
		while ((PixelStep(currResultX, currResultY, currDirection)) && 
			(currResultDistance < (resultDistance-1)))
		{
			++currResultDistance;
			if (sourceMatrix_.getSignMatrixElem(currResultY, currResultX) == 1)
			{
				//Нашёлся значимый пиксель.
				currPixelFinded = true;
				break;
			}
		}
		//Если пиксель найден - он точно расположен ближе чем найденный предыдущий, запомним его.
		if (currPixelFinded)
		{
			resultPixelY = currResultY;
			resultPixelX = currResultX;
			resultDistance = currResultDistance;
			resultDirection = currDirection;
		}
		//Готовимся к следующей итерации.
		if (currPixelFinded)
		{
			pixelFinded = true;
			currPixelFinded = false;
		}
		if (((currDirection == PIXEL_DIR_LEFT) && pixelFinded) ||
			(currDirection == PIXEL_DIR_DOWN_LEFT))
		{
			//Пиксель был найден по не-диагоналям. Проверять диагонали смысла нет.
			//Также продолжать поиск нет смысла если проверены уже вообще все направления.
			searchDone = true;
		}
	}
	//В любом случае - вернуть что получилось.
	return pixelFinded;
}

//Заполнить пустые пиксели source-матрицы простым алгоритмом (сплошной цвет).
template <typename CellType>
void RealMedianFilter<CellType>::FillMargins_Simple(CallBackBase *callBackObj)
{
	/*FillMargins_PixelBasedAlgo(&RealMedianFilter<CellType>::SimpleFiller,
		callBackObj);*/
	FillMargins_EmptyPixelBasedAlgo(&RealMedianFilter<CellType>::SimpleFiller,
		callBackObj);
}

//Заполнить пустые пиксели source-матрицы зеркальным алгоритмом.
template <typename CellType>
void RealMedianFilter<CellType>::FillMargins_Mirror(CallBackBase *callBackObj)
{
	/*FillMargins_PixelBasedAlgo(&RealMedianFilter<CellType>::MirrorFiller,
		callBackObj);*/
	FillMargins_EmptyPixelBasedAlgo(&RealMedianFilter<CellType>::MirrorFiller,
		callBackObj);
}

//Применит указанный (ссылкой на метод) фильтр к изображению. Входящий и исходящий файлы
//уже должны быть подключены. Вернёт false и инфу об ошибке если что-то пойдёт не так.
template <typename CellType>
bool RealMedianFilter<CellType>::ApplyFilter(FilterMethod CurrFilter,
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
	int debugFileNum = 1;	//Для отладочного сохранения.
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

		//Надо обработать граничные пиксели. Начальная позиция для обработки может быть разной в зависимости
		//от того как обрабатывается текущий кусок.
		/*int fillerYStart, fillerYToProcess;
		if (currMM == TOP_MM_FILE2)
		{
			fillerYStart = 0;
			fillerYToProcess = filterYToProcess + 2*marginSize;
		}
		else
		{
			fillerYStart = marginSize;
			fillerYToProcess = filterYToProcess + marginSize;
		}
		//Прогрессбар не используем пока там не будет реализована обработка нескольких баров в одном.
		//FillMargins(fillerYStart, fillerYToProcess, NULL);*/
		//Для переделанного заполнителя обработка граничных пикселей делается тупо целиком.
		FillMargins(0, filterYToProcess + 2*marginSize);
		//Также если используется алгоритм Хуанга - нужно обновить квантованную матрицу.
		if (CurrFilter == &RealMedianFilter<CellType>::HuangFilter)
		{
			//Копируется целиком. Можно оптимизировать и делать полную копию только в первый проход,
			//но пока - так.
			FillQuantedMatrix(0, filterYToProcess + 2*marginSize);
		}

		//Надо применить фильтр
		(this->*CurrFilter)(filterYToProcess,callBackObj);

		//Сохраняем то что получилось.
		if (!destMatrix_.SaveToGDALRaster(getOwnerObj().getGdalDestRaster(),
			writeYPosition, filterYToProcess, errObj))
		{
			return false;
		}

		//Для отладки - сохраним содержимое матриц.
		//sourceMatrix_.SaveToCSVFile("source" + STB.IntToString(debugFileNum, 5) + ".csv");
		//QuantedSaveToCSVFile("quanted" + STB.IntToString(debugFileNum, 5) + ".csv");
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
		FillMargins(marginSize, filterYToProcess, NULL);
		//Также если используется алгоритм Хуанга - нужно обновить квантованную матрицу.
		if (CurrFilter == &RealMedianFilter<CellType>::HuangFilter)
		{
			FillQuantedMatrix(marginSize, filterYToProcess);
		}
		//Всё ещё надо применить фильтр.
		(this->*CurrFilter)(filterYToProcess,callBackObj);

		//Сохраняем то что получилось.
		if (!destMatrix_.SaveToGDALRaster(getOwnerObj().getGdalDestRaster(),
			writeYPosition, filterYToProcess, errObj))
		{
			return false;
		}

		//Для отладки - сохраним содержимое матриц.
		//sourceMatrix_.SaveToCSVFile("source" + STB.IntToString(debugFileNum, 5) + ".LASTBLOCK.csv");
		//QuantedSaveToCSVFile("quanted" + STB.IntToString(debugFileNum, 5) + ".LASTBLOCK.csv");
		//destMatrix_.SaveToCSVFile("dest" + STB.IntToString(debugFileNum, 5) + ".LASTBLOCK.csv");
	}

	//Вроде всё ок.
	return true;
}

//Метод "никакого" фильтра, который тупо копирует входящую матрицу в исходящую. Нужен для тестирования
//и отладки. Первый аргумент указывает количество строк матрицы для реальной обработки.
template <typename CellType>
void RealMedianFilter<CellType>::StubFilter(const int &currYToProcess,
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
void RealMedianFilter<CellType>::StupidFilter(const int &currYToProcess,
	CallBackBase *callBackObj)
{
	//Данный метод применяет медианный фильтр "в лоб", т.ч. тупо для каждого пикселя создаёт
	//массив из всех пикселей, входящих в окно, после чего сортирует массив и медиану подставляет
	//на место пикселя в dest-матрице.
	int destX, destY, windowX, windowY, sourceX, sourceY, marginSize, medianArrPos;
	marginSize = (getOwnerObj().getAperture() - 1) / 2;
	//Сразу вычислим индексы и указатели чтобы не считать в цикле.
	//И выделим память под массив для пикселей окна, в котором ищем медиану.
	int medianArrSize = getOwnerObj().getAperture() * getOwnerObj().getAperture();
	CellType *medianArr = new CellType[medianArrSize];
	//Целочисленное деление, неточностью пренебрегаем ибо ожидаем окно со
	//стороной в десятки пикселей.
	CellType *medianPos = medianArr + (medianArrSize / 2);
	CellType *medianArrEnd = medianArr + medianArrSize;	//Элемент за последним в массиве.
	unsigned long progressPosition = getOwnerObj().getCurrPositionY()*destMatrix_.getXSize();

	//Поехали.
	//TODO: тут явно есть что поправить в плане оптимизации, есть лишние сложения и инкремент неправильный.
	for (destY = 0; destY < currYToProcess; destY++)
	{
		sourceY = destY + marginSize;
		for (destX = 0; destX < destMatrix_.getXSize(); destX++)
		{
			sourceX = destX + marginSize;
			progressPosition++;
			//Незначимые пиксели не трогаем.
			if (sourceMatrix_.getSignMatrixElem(sourceY, sourceX) != 1) continue;
			//Теперь надо пройти по каждому пикселю окна чтобы составить массив
			//и сортировкой получить медиану. Наверное самый неэффективны способ.
			medianArrPos = 0;
			for (windowY = destY; windowY<(sourceY + marginSize + 1); windowY++)
			{
				for (windowX = destX; windowX<(sourceX + marginSize + 1); windowX++)
				{
					medianArr[medianArrPos] = sourceMatrix_.getMatrixElem(windowY, windowX);
					medianArrPos++;
				}
			}
			//Сортируем, берём медиану из середины. Точностью поиска середины не
			//заморачиваемся т.к. окна будут большие, в десятки пикселов.
			std::nth_element(medianArr, medianPos, medianArrEnd);
			if (GetDelta(*medianPos, sourceMatrix_.getMatrixElem(sourceY, sourceX))
				< getOwnerObj().getThreshold())
			{
				//Отличие от медианы меньше порогового. Просто копируем пиксел.
				destMatrix_.setMatrixElem(destY, destX,
					sourceMatrix_.getMatrixElem(sourceY, sourceX));
			}
			else
			{
				//Отличие больше порогового - записываем в dest-пиксель медиану.
				destMatrix_.setMatrixElem(destY, destX, *medianPos);
			};
			//Сообщить вызвавшему коду прогресс выполнения.
			if (callBackObj) callBackObj->CallBack(progressPosition);
		};
	};
	//Не забыть delete! ))
	delete[] medianArr;
}

//Метод для обработки матрицы алгоритмом Хуанга. Теоретически на больших окнах это очень быстрый
//алгоритм, единственный недостаток которого - некоторая потеря точности из за квантования.
template<typename CellType>
void RealMedianFilter<CellType>::HuangFilter(const int &currYToProcess, CallBackBase *callBackObj)
{
	//Создаём массив-гистограмму.
	unsigned long *gist = new unsigned long[getOwnerObj().getHuangLevelsNum()];
	//Текущее значение медианы
	boost::uint16_t median;
	//Количество элементов в гистограмме, которые меньше медианы (т.е. левее её).
	boost::uint16_t elemsLeftMed;
	//Позиция, в которой по идее должна быть расположена медиана.
	boost::uint16_t halfMedPos = ((getOwnerObj().getAperture() * getOwnerObj().getAperture()) - 1) / 2;
	//Признак значимости гистограммы. Изначально гистограмма незначима и её нужно заполнить.
	bool gistIsActual = false;
	//Признак вообще наличия гистограммы. Второй признак нужен т.к. гистограмма может быть незначимой,
	//но при этом всё же существовать в устаревшем виде, что в некоторых случаях может быть использовано
	//для ускорения обработки (когда быстрее сделать гистограмму актуальной чем составить её с нуля).
	bool gistIsEmpty = true;
	//Счётчики и прочее что может понадобиться.
	int sourceX, sourceY, destX, destY, marginSize, oldY, oldX;
	marginSize = (getOwnerObj().getAperture() - 1) / 2;
	//Предыдущие значения координат меньше нуля - в процессе работы такие невозможны что обозначает
	//никакой пиксель.
	oldY = -1;
	oldX = -1;
	//Позиция прогрессбара.
	unsigned long progressPosition = getOwnerObj().getCurrPositionY()*destMatrix_.getXSize();

	//Основной цикл по строкам пикселей. Каждая итерация обрабатывает до 2 строк, сначала справа налево,
	//потом слева направо.
	destY = 0;
	sourceY = destY + marginSize;
	while (destY < currYToProcess)
	{
		//Тут первый подцикл.
		HuangFilter_ProcessStringToRight(destX, destY, sourceX, sourceY, marginSize, progressPosition,
			gistIsActual, gistIsEmpty, gist, median, elemsLeftMed, oldY, oldX, halfMedPos, callBackObj);

		//На следующую строку.
		++destY;
		++sourceY;
		//Вываливаемся из цикла если вышли за границы.
		if (!(destY < currYToProcess)) break;

		//Тут второй подцикл.
		HuangFilter_ProcessStringToLeft(destX, destY, sourceX, sourceY, marginSize, progressPosition,
			gistIsActual, gistIsEmpty, gist, median, elemsLeftMed, oldY, oldX, halfMedPos, callBackObj);

		//На следующую строку.
		++destY;
		++sourceY;
	}

	//Не забыть delete!
	delete[] gist;
}

//Вспомогательный метод для алгоритма Хуанга. Цикл по строке вправо.
template<typename CellType>
inline void RealMedianFilter<CellType>::HuangFilter_ProcessStringToRight(int &destX, int &destY,
	int &sourceX, int &sourceY, const int &marginSize, unsigned long &progressPosition,
	bool &gistIsActual, bool &gistIsEmpty, unsigned long *gist, boost::uint16_t &median,
	boost::uint16_t &elemsLeftMed, int &oldY, int &oldX, const boost::uint16_t &halfMedPos,
	CallBackBase *callBackObj)
{
	for (destX = 0, sourceX = destX + marginSize; destX < destMatrix_.getXSize(); ++destX, ++sourceX)
	{
		//Прогрессбар.
		progressPosition++;
		//Незначимые пиксели не трогаем.
		if (sourceMatrix_.getSignMatrixElem(sourceY, sourceX) != 1)
		{

			gistIsActual = false;
			continue;
		};
		//Пиксель значим - обработаем его тем или иным образом.
		if (gistIsActual)
		{
			//Гистограмма актуальна - шаг будет или вправо или вниз, смотря какой это пиксель в строке.
			if (destX == 0)
			{
				//Это был шаг вниз.
				HuangFilter_DoStepDown(destY, destX, gist, median, elemsLeftMed);
				++oldY;
			}
			else
			{
				//Это был шаг вправо.
				HuangFilter_DoStepRight(destY, destX, gist, median, elemsLeftMed);
				++oldX;
			}
			//Корректируем медиану.
			HuangFilter_DoMedianCorrection(median, elemsLeftMed, halfMedPos, gist);
			//При необходимости записываем новое значение пикселя.
			HuangFilter_WriteDestPixel(destY, destX, sourceY, sourceX, median);
		}
		else if ((!gistIsEmpty) && (destY == oldY) && ((destX-oldX) <= halfMedPos))
		{
			//Гистограмма неактуальна, но была актуальна на этой же строке и не так далеко.
			//Сымитируем нужное количество "шагов вправо" чтобы сделать гистограмму актуальной.
			while (oldX < destX)
			{
				++oldX;
				HuangFilter_DoStepRight(destY, oldX, gist, median, elemsLeftMed);
			}
			//Корректируем медиану.
			HuangFilter_DoMedianCorrection(median, elemsLeftMed, halfMedPos, gist);
			gistIsActual = true;
			//При необходимости записываем новое значение пикселя.
			HuangFilter_WriteDestPixel(destY, destX, sourceY, sourceX, median);
		}
		else if ((!gistIsEmpty) && (destX == oldX) && (destY == oldY+1))
		{
			//Гистограмма неактуальна, но была актуальна на предыдущей строке и в том же столбце.
			//Таким образом, неважно как именно мы сюда попали, но фактически нам достаточно просто
			//сделать тут шаг вниз.
			HuangFilter_DoStepDown(destY, destX, gist, median, elemsLeftMed);
			++oldY;
			//Корректируем медиану.
			HuangFilter_DoMedianCorrection(median, elemsLeftMed, halfMedPos, gist);
			gistIsActual = true;
			//При необходимости записываем новое значение пикселя.
			HuangFilter_WriteDestPixel(destY, destX, sourceY, sourceX, median);
		}
		else
		{
			//Старая гистограмма бесполезна. Придётся заполнить её с нуля.
			HuangFilter_FillGist(destY, destX, gist, median, elemsLeftMed, halfMedPos);
			//При необходимости записываем новое значение пикселя.
			HuangFilter_WriteDestPixel(destY, destX, sourceY, sourceX, median);
			//Запомнить где мы есть и состояние гистограммы.
			oldY = destY;
			oldX = destX;
			gistIsEmpty = false;
			gistIsActual = true;
		};

		//Сообщить вызвавшему коду прогресс выполнения.
		if (callBackObj) callBackObj->CallBack(progressPosition);
	}
}

//Вспомогательный метод для алгоритма Хуанга. Цикл по строке влево.
//Два почти одинаковых метода здесь чтобы внутри не делать проверок направления
//за счёт чего оно может быть будет работать немного быстрее.
template<typename CellType>
inline void RealMedianFilter<CellType>::HuangFilter_ProcessStringToLeft(int &destX, int &destY,
	int &sourceX, int &sourceY, const int &marginSize, unsigned long &progressPosition,
	bool &gistIsActual, bool &gistIsEmpty, unsigned long *gist, boost::uint16_t &median,
	boost::uint16_t &elemsLeftMed, int &oldY, int &oldX, const boost::uint16_t &halfMedPos,
	CallBackBase *callBackObj)
{
	int lastPixelX = destMatrix_.getXSize()-1;
	for (destX = lastPixelX, sourceX = destX + marginSize; destX >= 0;
		--destX, --sourceX)
	{
		//Прогрессбар.
		progressPosition++;
		//Незначимые пиксели не трогаем.
		if (sourceMatrix_.getSignMatrixElem(sourceY, sourceX) != 1)
		{

			gistIsActual = false;
			continue;
		};
		//Пиксель значим - обработаем его тем или иным образом.
		if (gistIsActual)
		{
			//Гистограмма актуальна - шаг будет или влево или вниз, смотря какой это пиксель в строке.
			if (destX == lastPixelX)
			{
				//Это был шаг вниз.
				HuangFilter_DoStepDown(destY, destX, gist, median, elemsLeftMed);
				++oldY;
			}
			else
			{
				//Это был шаг влево.
				HuangFilter_DoStepLeft(destY, destX, gist, median, elemsLeftMed);
				--oldX;
			}
			//Корректируем медиану.
			HuangFilter_DoMedianCorrection(median, elemsLeftMed, halfMedPos, gist);
			//При необходимости записываем новое значение пикселя.
			HuangFilter_WriteDestPixel(destY, destX, sourceY, sourceX, median);
		}
		else if ((!gistIsEmpty) && (destY == oldY) && ((oldX-destX) <= halfMedPos))
		{
			//Гистограмма неактуальна, но была актуальна на этой же строке и не так далеко.
			//Сымитируем нужное количество "шагов вправо" чтобы сделать гистограмму актуальной.
			while (oldX > destX)
			{
				--oldX;
				HuangFilter_DoStepLeft(destY, oldX, gist, median, elemsLeftMed);
			}
			//Корректируем медиану.
			HuangFilter_DoMedianCorrection(median, elemsLeftMed, halfMedPos, gist);
			gistIsActual = true;
			//При необходимости записываем новое значение пикселя.
			HuangFilter_WriteDestPixel(destY, destX, sourceY, sourceX, median);
		}
		else if ((!gistIsEmpty) && (destX == oldX) && (destY == oldY+1))
		{
			//Гистограмма неактуальна, но была актуальна на предыдущей строке и в том же столбце.
			//Таким образом, неважно как именно мы сюда попали, но фактически нам достаточно просто
			//сделать тут шаг вниз.
			HuangFilter_DoStepDown(destY, destX, gist, median, elemsLeftMed);
			++oldY;
			//Корректируем медиану.
			HuangFilter_DoMedianCorrection(median, elemsLeftMed, halfMedPos, gist);
			gistIsActual = true;
			//При необходимости записываем новое значение пикселя.
			HuangFilter_WriteDestPixel(destY, destX, sourceY, sourceX, median);
		}
		else
		{
			//Старая гистограмма бесполезна. Придётся заполнить её с нуля.
			HuangFilter_FillGist(destY, destX, gist, median, elemsLeftMed, halfMedPos);
			//При необходимости записываем новое значение пикселя.
			HuangFilter_WriteDestPixel(destY, destX, sourceY, sourceX, median);
			//Запомнить где мы есть и состояние гистограммы.
			oldY = destY;
			oldX = destX;
			gistIsEmpty = false;
			gistIsActual = true;
		};

		//Сообщить вызвавшему коду прогресс выполнения.
		if (callBackObj) callBackObj->CallBack(progressPosition);
	}
}

//Вспомогательный метод для алгоритма Хуанга. Заполняет гистограмму с нуля. В параметрах координаты
//верхнего левого угла апертуры.
template<typename CellType>
inline void RealMedianFilter<CellType>::HuangFilter_FillGist(const int &leftUpY, const int &leftUpX,
	unsigned long *gist, boost::uint16_t &median, boost::uint16_t &elemsLeftMed,
	const boost::uint16_t & halfMedPos)
{
	//Счётчики и границы.
	int windowY, windowX, windowYEnd, windowXEnd;
	windowYEnd = leftUpY + getOwnerObj().getAperture();
	windowXEnd = leftUpX + getOwnerObj().getAperture();
	//Обнуляем гистограмму.
	std::fill(gist, gist+getOwnerObj().getHuangLevelsNum(), 0);
	//Проходим по пикселям апертуры и заполняем гистограмму.
	for (windowY = leftUpY; windowY < windowYEnd; ++windowY)
	{
		for (windowX = leftUpX; windowX < windowXEnd; ++windowX)
		{
			//Инкрементируем счётчик пикселей этого уровня в гистограмме.
			boost::uint16_t temp = sourceMatrix_.getQuantedMatrixElem(windowY, windowX);
			++(gist[sourceMatrix_.getQuantedMatrixElem(windowY, windowX)]);
		}
	}
	//Теперь найдём актуальное значение медианы и количество пикселей левее медианы.
	elemsLeftMed = 0;
	median = 0;
	while ((elemsLeftMed + gist[median]) <= halfMedPos)
	{
		elemsLeftMed += gist[median];
		++median;
	}
}

//Вспомогательный метод для алгоритма Хуанга. Выполняет шаг вправо.
template<typename CellType>
inline void RealMedianFilter<CellType>::HuangFilter_DoStepRight(const int &leftUpY,
	const int &leftUpX, unsigned long *gist, const boost::uint16_t &median,
	boost::uint16_t &elemsLeftMed)
{
	//Счётчики и границы.
	int windowY, windowXLeft, windowXRight , windowYEnd;
	windowYEnd = leftUpY + getOwnerObj().getAperture();
	windowXLeft = leftUpX - 1;
	windowXRight = windowXLeft + getOwnerObj().getAperture();
	//Тут будет текущее квантованное значение пикселя.
	boost::uint16_t currQuantedValue;

	//Удаляем из гистограммы колонку левее апертуры, добавляем последнюю (самую правую) колонку.
	for (windowY = leftUpY; windowY < windowYEnd; ++windowY)
	{
		//Левая колонка.
		currQuantedValue = sourceMatrix_.getQuantedMatrixElem(windowY, windowXLeft);
		--(gist[currQuantedValue]);
		if (currQuantedValue < median)
		{
			--elemsLeftMed;
		}
		//Правая колонка.
		currQuantedValue = sourceMatrix_.getQuantedMatrixElem(windowY, windowXRight);
		++(gist[currQuantedValue]);
		if (currQuantedValue < median)
		{
			++elemsLeftMed;
		}
	}
}

//Вспомогательный метод для алгоритма Хуанга. Выполняет шаг влево.
template<typename CellType>
inline void RealMedianFilter<CellType>::HuangFilter_DoStepLeft(const int &leftUpY,
	const int &leftUpX, unsigned long *gist, const boost::uint16_t &median,
	boost::uint16_t &elemsLeftMed)
{
	//Счётчики и границы.
	int windowY, windowXRight, windowYEnd;
	windowYEnd = leftUpY + getOwnerObj().getAperture();
	windowXRight = leftUpX + getOwnerObj().getAperture();
	//Тут будет текущее квантованное значение пикселя.
	boost::uint16_t currQuantedValue;

	//Удаляем из гистограммы колонку правее апертуры, добавляем первую (самую левую) колонку.
	for (windowY = leftUpY; windowY < windowYEnd; ++windowY)
	{
		//Левая колонка.
		currQuantedValue = sourceMatrix_.getQuantedMatrixElem(windowY, leftUpX);
		++(gist[currQuantedValue]);
		if (currQuantedValue < median)
		{
			++elemsLeftMed;
		}
		//Правая колонка.
		currQuantedValue = sourceMatrix_.getQuantedMatrixElem(windowY, windowXRight);
		--(gist[currQuantedValue]);
		if (currQuantedValue < median)
		{
			--elemsLeftMed;
		}
	}
}

//Вспомогательный метод для алгоритма Хуанга. Выполняет шаг вниз.
template<typename CellType>
inline void RealMedianFilter<CellType>::HuangFilter_DoStepDown(const int &leftUpY,
	const int &leftUpX, unsigned long *gist, const boost::uint16_t &median,
	boost::uint16_t &elemsLeftMed)
{
	//Счётчики и границы.
	int windowX, windowYUp, windowYDown, windowXEnd;
	windowXEnd = leftUpX + getOwnerObj().getAperture();
	windowYUp = leftUpY - 1;
	windowYDown = windowYUp + getOwnerObj().getAperture();
	//Тут будет текущее квантованное значение пикселя.
	boost::uint16_t currQuantedValue;

	//Удаляем из гистограммы строку выше апертуры, добавляем последнюю (самую нижнюю)
	//строку апертуры.
	for (windowX = leftUpX; windowX < windowXEnd; ++windowX)
	{
		//Верхняя строка.
		currQuantedValue = sourceMatrix_.getQuantedMatrixElem(windowYUp, windowX);
		--(gist[currQuantedValue]);
		if (currQuantedValue < median)
		{
			--elemsLeftMed;
		}
		//Нижняя строка.
		currQuantedValue = sourceMatrix_.getQuantedMatrixElem(windowYDown, windowX);
		++(gist[currQuantedValue]);
		if (currQuantedValue < median)
		{
			++elemsLeftMed;
		}
	}
}

//Вспомогательный метод для алгоритма Хуанга. Корректирует медиану.
template<typename CellType>
inline void RealMedianFilter<CellType>::HuangFilter_DoMedianCorrection(boost::uint16_t &median,
	boost::uint16_t &elemsLeftMed, const boost::uint16_t &halfMedPos, unsigned long *gist)
{
	if (elemsLeftMed > halfMedPos)
	{
		//Медиану надо двигать вправо, т.е. уменьшать.
		do
		{
			--median;
			elemsLeftMed -= gist[median];
		}
		while (elemsLeftMed > halfMedPos);
	}
	else
	{
		//Медиана либо уже актуальна, либо её нужно двигать влево т.е. увеличивать.
		while (elemsLeftMed + gist[median] <= halfMedPos)
		{
			elemsLeftMed += gist[median];
			++median;
		}
	}
	/*//В любом случае теперь имеем корректную медиану.
	//Проверим правильность корректировки.
	boost::uint16_t elemsLeftMed_test = 0;
	boost::uint16_t median_test = 0;
	while ((elemsLeftMed_test + gist[median_test]) <= halfMedPos)
	{
		elemsLeftMed_test += gist[median_test];
		++median_test;
	}
	if (median != median_test)
	{
		median = median_test;
	}*/
}

//Вспомогательный метод для алгоритма Хуанга. Запись нового значения пикселя в матрицу
//назначения.
template<typename CellType>
inline void RealMedianFilter<CellType>::HuangFilter_WriteDestPixel(const int &destY, const int &destX,
	const int &sourceY,	const int &sourceX,	const boost::uint16_t &median)
{
	CellType newValue = QuantedValueToPixelValue(median);
	if (GetDelta(newValue, sourceMatrix_.getMatrixElem(sourceY, sourceX))
		< getOwnerObj().getThreshold())
	{
		//Отличие от медианы меньше порогового. Просто копируем пиксел.
		destMatrix_.setMatrixElem(destY, destX,
			sourceMatrix_.getMatrixElem(sourceY, sourceX));
	}
	else
	{
		//Отличие больше порогового - записываем в dest-пиксель медиану.
		destMatrix_.setMatrixElem(destY, destX, newValue);
	};
	//TEST - пишем просто квантованный пиксел.
	/*destMatrix_.setMatrixElem(destY, destX,
		QuantedValueToPixelValue(sourceMatrix_.getQuantedMatrixElem(sourceY, sourceX)));*/
}

//Метод вычисляет минимальную и максимальную высоту в открытом изображении если это вообще нужно.
//Также вычисляет дельту (шаг между уровнями).
template<typename CellType>
void RealMedianFilter<CellType>::CalcMinMaxPixelValues()
{
	if (!minMaxCalculated_)
	{
		//Заставим сработать событие, сигнализирующее о начале вычисления.
		if (getOwnerObj().onMinMaxDetectionStart != NULL) getOwnerObj().onMinMaxDetectionStart();
		//Запишем новое NoDataValue и вычислим min и max средствами GDAL.
		getOwnerObj().getGdalSourceRaster()->SetNoDataValue((double)noDataPixelValue_);
		double minMaxArr[2];
		getOwnerObj().getGdalSourceRaster()->ComputeRasterMinMax(false, &(minMaxArr[0]));
		minPixelValue_ = (CellType)minMaxArr[0];
		maxPixelValue_ = (CellType)minMaxArr[1];
		minMaxCalculated_ = true;
		//Дельта (шаг между уровнями) тоже вычисляется именно тут.
		levelsDelta_ = (double)(maxPixelValue_ - minPixelValue_) / (double)(getOwnerObj().getHuangLevelsNum()-1);
		//И сообщим что всё готово.
		if (getOwnerObj().onMinMaxDetectionEnd != NULL) getOwnerObj().onMinMaxDetectionEnd();
	}
}

//--------------------------------//
//       Прочий функционал        //
//--------------------------------//

//Заполняет граничные (пустые пиксели) области вокруг значимых пикселей в соответствии с
//выбранным алгоритмом.
template <typename CellType>
void RealMedianFilter<CellType>::FillMargins(const int yStart, const int yToProcess,
	CallBackBase *callBackObj)
{
	//Просто пробросим вызов в реально работающий метод, правильно указав метод-заполнитель.
	switch (getOwnerObj().getMarginType())
	{
	case MARGIN_SIMPLE_FILLING:
		/*FillMargins_PixelBasedAlgo(&RealMedianFilter<CellType>::SimpleFiller, yStart, yToProcess,
		callBackObj);*/
		FillMargins_EmptyPixelBasedAlgo(&RealMedianFilter<CellType>::SimpleFiller, yStart, yToProcess,
			callBackObj);
		break;
	case MARGIN_MIRROR_FILLING:
		/*FillMargins_PixelBasedAlgo(&RealMedianFilter<CellType>::MirrorFiller, yStart, yToProcess,
			callBackObj);*/
		FillMargins_EmptyPixelBasedAlgo(&RealMedianFilter<CellType>::MirrorFiller, yStart, yToProcess,
			callBackObj);
	}
}

//Заполняет матрицу квантованных пикселей в указанном промежутке, получая их из значений
//оригинальных пикселей в исходной матрице. Нужно для алгоритма Хуанга.
template <typename CellType>
void RealMedianFilter<CellType>::FillQuantedMatrix(const int yStart, const int yToProcess)
{
	//unsigned long progressPosition = 0;
	//Поехали.
	for (int y = yStart; y < (yStart + yToProcess); ++y)
	{
		for (int x = 0; x < sourceMatrix_.getXSize(); ++x)
		{
			//++progressPosition;

			//Для всех пикселей (даже незначимых!) - преобразовать к одному из уровней в
			//гистограмме. Нужно знать минимальную и максимальную высоту в исходной
			//картинке. При этом также вычислится и дельта. Нулевые незначимые пиксели тут
			//так и останутся нулевыми.
			CalcMinMaxPixelValues();
			//Заполняем значение
			sourceMatrix_.setQuantedMatrixElem(y, x, PixelValueToQuantedValue(
				sourceMatrix_.getMatrixElem(y, x)));
		}
	}
}

//Обрабатывает выбранный исходный файл "тупым" фильтром. Результат записывается в выбранный destFile.
template <typename CellType>
bool RealMedianFilter<CellType>::ApplyStupidFilter(CallBackBase *callBackObj,
	ErrorInfo *errObj)
{
	//Просто вызываем уже готовый метод, передав ему нужный фильтрующий метод.
	return ApplyFilter(&RealMedianFilter<CellType>::StupidFilter, callBackObj, errObj);
}

//Обрабатывает выбранный исходный файл алгоритмом Хуанга. Результат записывается в выбранный destFile.
template <typename CellType>
bool RealMedianFilter<CellType>::ApplyHuangFilter(CallBackBase *callBackObj,
	ErrorInfo *errObj)
{
	//Просто вызываем уже готовый метод, передав ему нужный фильтрующий метод.
	return ApplyFilter(&RealMedianFilter<CellType>::HuangFilter, callBackObj, errObj);
}

//Обрабатывает выбранный исходный файл "никаким" фильтром. По сути это просто копирование.
//Для отладки. Результат записывается в выбранный destFile
template <typename CellType>
bool RealMedianFilter<CellType>::ApplyStubFilter(CallBackBase *callBackObj, ErrorInfo *errObj)
{
	//Новый вариант "никакого" фильтра. Работающий по кускам. Просто вызываем уже готовый
	//метод, передав ему нужный фильтрующий метод.
	return ApplyFilter(&RealMedianFilter<CellType>::StubFilter, callBackObj, errObj);
}

//"Тупая" визуализация матрицы, отправляется прямо в cout.
template <typename CellType>
void RealMedianFilter<CellType>::SourcePrintStupidVisToCout()
{
	//Просто проброс вызова в объект матрицы.
	sourceMatrix_.PrintStupidVisToCout();
}

//Вывод исходной матрицы в csv-файл, который должны понимать всякие картографические
//программы. Это значит что каждый пиксел - это одна строка в файле.
//Это "тупой" вариант вывода - метаданные нормально не сохраняются.
template <typename CellType>
bool RealMedianFilter<CellType>::SourceSaveToCSVFile(const std::string &fileName, ErrorInfo *errObj)
{
	//Просто проброс вызова в объект матрицы.
	return sourceMatrix_.SaveToCSVFile(fileName, errObj);
}

//Вывод исходной квантованной матрицы в csv-файл.
template<typename CellType>
bool RealMedianFilter<CellType>::QuantedSaveToCSVFile(const std::string & fileName, ErrorInfo * errObj)
{
	//Создаём временную матрицу.
	AltMatrix<CellType> tempMatr;
	tempMatr.CreateEmpty(sourceMatrix_.getXSize(), sourceMatrix_.getYSize());
	//Копируем квантованную матрицу в новосозданную с преобразованием пикселей в обычные.
	for (int currY = 0; currY < tempMatr.getYSize(); ++currY)
	{
		for (int currX = 0; currX < tempMatr.getXSize(); ++currX)
		{
			tempMatr.setMatrixElem(currY, currX, QuantedValueToPixelValue(
				sourceMatrix_.getQuantedMatrixElem(currY, currX)));
		}
	}
	//Сохраняем.
	return tempMatr.SaveToCSVFile(fileName, errObj);
}

//Аналогично SourceSaveToCSVFile, но для матрицы с результатом.
template <typename CellType>
bool RealMedianFilter<CellType>::DestSaveToCSVFile(const std::string &fileName, ErrorInfo *errObj)
{
	//Просто проброс вызова в объект матрицы.
	return destMatrix_.SaveToCSVFile(fileName, errObj);
}

////////////////////////////////////////
//          MedianFilterBase          //
////////////////////////////////////////

MedianFilterBase::MedianFilterBase(bool useHuangAlgo, boost::uint16_t huangLevelsNum) :
aperture_(DEFAULT_MEDFILTER_APERTURE), threshold_(DEFAULT_MEDFILTER_THRESHOLD),
marginType_(DEFAULT_MEDFILTER_MARGIN_TYPE), useMemChunks_(false), blocksInMem_(0), sourceFileName_(""),
destFileName_(""), imageSizeX_(0), imageSizeY_(0), imageIsLoaded_(false), sourceIsAttached_(false),
destIsAttached_(false), dataType_(PIXEL_UNKNOWN), dataTypeSize_(0), pFilterObj_(NULL), minBlockSize_(0),
minMemSize_(0), gdalSourceDataset_(NULL), gdalDestDataset_(NULL), gdalSourceRaster_(NULL),
gdalDestRaster_(NULL), currPositionY_(0), useHuangAlgo_(useHuangAlgo), huangLevelsNum_(huangLevelsNum)
{

}

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
	//Размер пиксела в исходного блока в байтах у нас складывается из размера типа
	//элементов в матрице и размера элемента вспомогательной матрицы (это 1 байт).
	//
	//Размер исходного блока - зависит от того используем ли алгоритм Хуанга.
	unsigned long long minSourceBlockSize;
	if (useHuangAlgo_)
	{
		minSourceBlockSize = blockHeight * blockWidth * (dataTypeSize_ + 3);
	}
	else
	{
		minSourceBlockSize = blockHeight * blockWidth * (dataTypeSize_ + 1);
	}
	//Размер блока с результатом
	unsigned long long minDestBlockSize = imageSizeX_ * firstLastBlockHeight * dataTypeSize_;
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
		(imageSizeX_ * imageSizeY_ * dataTypeSize_) +	//dest-матрица
		(blockWidth * imageSizeY_ * (dataTypeSize_ + 1));	//остальная source-матрица.

}

//--------------------------//
//      Прочие методы       //
//--------------------------//

//Выбрать исходный файл для дальнейшего чтения и обработки. Получает информацию о параметрах изображения,
//запоминает её в полях объекта.
bool MedianFilterBase::OpenInputFile(const std::string &fileName, ErrorInfo *errObj)
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
		if (errObj) errObj->SetError(CMNERR_INTERNAL_ERROR, ": MedianFilterBase::OpenInputFile() попытка открыть файл при не закрытом старом." );
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
		if (errObj) errObj->SetError(CMNERR_UNKNOWN_ERROR, "MedianFilterBase::OpenInputFile() error creating pFilterObj_!",true);
		gdalSourceDataset_ = NULL;
		gdalSourceRaster_ = NULL;
		return false;
	}
}

//Подготовить целевой файл к записи в него результата. Если forceRewrite==true - перезапишет уже
//существующий файл. Иначе вернёт ошибку (false и инфу в errObj). Input-файл уже должен быть открыт.
bool MedianFilterBase::OpenOutputFile(const std::string &fileName, const bool &forceRewrite, ErrorInfo *errObj)
{
	//Этот метод можно вызывать только если исходный файл уже был открыт.
	if (!sourceIsAttached_)
	{
		if (errObj) errObj->SetError(CMNERR_INTERNAL_ERROR, ":  MedianFilterBase::OpenOutputFile исходный файл не был открыт.");
		return false;
	}
	//И только если файл назначение открыт наоборот ещё не был.
	if (destIsAttached_)
	{
		if (errObj) errObj->SetError(CMNERR_INTERNAL_ERROR, ":  MedianFilterBase::OpenOutputFile попытка открыть файл назначения при уже открытом файле назначения.");
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
			if (errObj) errObj->SetError(CMNERR_WRITE_ERROR, ": " +
				STB.SystemCharsetToUtf8(errCode.message()));
			return false;
		}
	}

	//Всё, файла быть не должно. Копируем исходный файл.
	filesystem::copy_file(sourceFilePath, destFilePath, errCode);
	if (errCode.value())
	{
		if (errObj) errObj->SetError(CMNERR_WRITE_ERROR, ": " +
			STB.SystemCharsetToUtf8(errCode.message()));
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
bool MedianFilterBase::SourceSaveToCSVFile(const std::string &fileName, ErrorInfo *errObj)
{
	//Просто проброс вызова в объект матрицы.
	return pFilterObj_->SourceSaveToCSVFile(fileName, errObj);
}

//Аналогично SourceSaveToCSVFile, но для матрицы с результатом.
bool MedianFilterBase::DestSaveToCSVFile(const std::string &fileName, ErrorInfo *errObj)
{
	//Просто проброс вызова в объект матрицы.
	return pFilterObj_->DestSaveToCSVFile(fileName, errObj);
}

////////////////////////////////////////
//          MedianFilterStub          //
////////////////////////////////////////

//Применить "никакой" медианный фильтр.
bool MedianFilterStub::ApplyFilter(CallBackBase *callBackObj, ErrorInfo *errObj)
{
	//Проброс вызова.
	if (getSourceIsAttached() && getDestIsAttached())
	{
		return getFilterObj().ApplyStubFilter(callBackObj, errObj);
	}
	else
	{
		if (errObj) errObj->SetError(CMNERR_INTERNAL_ERROR, ": MedianFilterBase::ApplyStubFilter no source and\\or dest \
file(s) were attached.");
		return false;
	}
}

//////////////////////////////////////////
//          MedianFilterStupid          //
//////////////////////////////////////////

//Обрабатывает выбранный исходный файл "тупым" фильтром. Результат записывается в выбранный destFile.
bool MedianFilterStupid::ApplyFilter(CallBackBase *callBackObj, ErrorInfo *errObj)
{
	//Проброс вызова.
	if (getSourceIsAttached() && getDestIsAttached())
	{
		return getFilterObj().ApplyStupidFilter(callBackObj, errObj);
	}
	else
	{
		if (errObj) errObj->SetError(CMNERR_INTERNAL_ERROR, ": MedianFilterBase::ApplyStubFilter no source and\\or dest \
file(s) were attached.");
		return false;
	}
}

//////////////////////////////////////////
//          MedianFilterHuang           //
//////////////////////////////////////////

//Обработать изображение медианным фильтром по алгоритму Хуанга
bool MedianFilterHuang::ApplyFilter(CallBackBase *callBackObj, ErrorInfo *errObj)
{
	//Проброс вызова.
	if (getSourceIsAttached() && getDestIsAttached())
	{
		return getFilterObj().ApplyHuangFilter(callBackObj, errObj);
	}
	else
	{
		if (errObj) errObj->SetError(CMNERR_INTERNAL_ERROR, ": MedianFilterBase::ApplyHuangFilter no source and\\or dest \
file(s) were attached.");
		return false;
	}
}

} //namespace geoimgconv