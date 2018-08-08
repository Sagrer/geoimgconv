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

//Шаблонный класс для произвольного типа пикселя. На базе этой версии создаются
//специализации наследников с уже конкретными алгоритмами, тоже шаблонные.

#include "pixel_type_speciefic_filter.h"
#include <algorithm>
#include <memory>

using namespace std;

namespace geoimgconv
{

//Константы со значениями по умолчанию см. в common.cpp

//Инстанциация по шаблону для тех типов, которые реально будут использоваться. Таким образом,
//код для них будет генерироваться только в одной единице компилляции.
template class PixelTypeSpecieficFilter<double>;
template class PixelTypeSpecieficFilter<float>;
template class PixelTypeSpecieficFilter<int8_t>;
template class PixelTypeSpecieficFilter<uint8_t>;
template class PixelTypeSpecieficFilter<int16_t>;
template class PixelTypeSpecieficFilter<uint16_t>;
template class PixelTypeSpecieficFilter<int32_t>;
template class PixelTypeSpecieficFilter<uint32_t>;

////////////////////////////////////////////
//           PixelTypeSpecieficFilter             //
////////////////////////////////////////////

//--------------------------------//
//       Приватные методы         //
//--------------------------------//

//Сделать шаг по пиксельным координатам в указанном направлении.
//Вернёт false если координаты ушли за границы изображения, иначе true.
template <typename CellType>
bool PixelTypeSpecieficFilter<CellType>::PixelStep(int &x, int &y, const PixelDirection direction)
{
	if (direction == PixelDirection::Up)
	{
		y--;
	}
	else if (direction == PixelDirection::Down)
	{
		y++;
	}
	else if (direction == PixelDirection::Right)
	{
		x++;
	}
	else if (direction == PixelDirection::Left)
	{
		x--;
	}
	else if (direction == PixelDirection::UpRight)
	{
		x++;
		y--;
	}
	else if (direction == PixelDirection::UpLeft)
	{
		x--;
		y--;
	}
	else if (direction == PixelDirection::DownRight)
	{
		x++;
		y++;
	}
	else if (direction == PixelDirection::DownLeft)
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
void PixelTypeSpecieficFilter<CellType>::GetMirrorPixel(const int &x, const int &y, const int &currX, const int &currY, int &outX, int &outY, const int recurseDepth)
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
			delta = abs(x-currX)-shift;
			if ((x-currX)<0) delta = -delta;
			outX=x+delta;
		};
		if (y != currY)
		{
			delta = abs(y-currY)-shift;
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
void PixelTypeSpecieficFilter<CellType>::SimpleFiller(const int &x, const int &y,
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
void PixelTypeSpecieficFilter<CellType>::MirrorFiller(const int &x, const int &y,
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
void PixelTypeSpecieficFilter<CellType>::FillMargins_PixelBasedAlgo(const PixFillerMethod FillerMethod,
	const int yStart, const int yToProcess, CallBackBase *callBackObj)
{
	//Двигаемся построчно, пока не найдём значимый пиксель. В границы не лезем,
	//т.к. они значимыми быть не могут.
	int x, y, marginSize;
	marginSize = (getOwnerObj().getAperture() - 1) / 2;
	/*//Размер "окна" заполнения увеличен в полтора раза чтобы уменьшить вероятность того что
	//внутри окна вокруг значимого пикселя всё-же окажутся незаполненные незначимые.
	//Полностью избежать этого в данном алгоритме нельзя, но алгоритм гарантирующий это на 100%
	//работает ощутимо медленнее. Небольшие неточности в прочем для медианного фильтра будут
	//практически непринципиальны.
	int fillerWindowSize = (int)(marginSize * 1.5);*/
	//Но при сильном разбиении изображения на куски это похоже создаёт больше проблем чем пользы :(
	//Поэтому вот так. В этом варианте деформаций всё же меньше.
	int fillerWindowSize = marginSize;
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

				(this->*FillerMethod)(x, y, PixelDirection::Up, fillerWindowSize, 2);
				(this->*FillerMethod)(x, y, PixelDirection::Down, fillerWindowSize, 2);
				(this->*FillerMethod)(x, y, PixelDirection::Right, fillerWindowSize, 2);
				(this->*FillerMethod)(x, y, PixelDirection::Left, fillerWindowSize, 2);
				(this->*FillerMethod)(x, y, PixelDirection::UpRight, fillerWindowSize, 3);
				(this->*FillerMethod)(x, y, PixelDirection::UpLeft, fillerWindowSize, 3);
				(this->*FillerMethod)(x, y, PixelDirection::DownRight, fillerWindowSize, 3);
				(this->*FillerMethod)(x, y, PixelDirection::DownLeft, fillerWindowSize, 3);
				//Сообщение о прогрессе.
				if (callBackObj) callBackObj->CallBack(progressPosition);
			}
		}
	}
}

//Алгоритм заполнения граничных пикселей по незначимым пикселям, общий для Simple и Mirror
template <typename CellType>
void PixelTypeSpecieficFilter<CellType>::FillMargins_EmptyPixelBasedAlgo(const PixFillerMethod FillerMethod,
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
			actualPixelY, actualPixelX, actualPixelDistance, actualPixelDirection, PixelDirection::Down,
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
				actualPixelY, actualPixelX, actualPixelDistance, actualPixelDirection, PixelDirection::Down,
				windowWasEmpty, tempPixelValue, FillerMethod);
		}
	}
}

//Вспомогательный метод для прохода по пикселам строки вправо.
template<typename CellType>
inline void PixelTypeSpecieficFilter<CellType>::FillMargins_EmptyPixelBasedAlgo_ProcessToRight(int &y, int &x,
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
			actualPixelY, actualPixelX, actualPixelDistance, actualPixelDirection, PixelDirection::Right,
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
inline void PixelTypeSpecieficFilter<CellType>::FillMargins_EmptyPixelBasedAlgo_ProcessToLeft(int &y, int &x,
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
			actualPixelY, actualPixelX, actualPixelDistance, actualPixelDirection, PixelDirection::Left,
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
inline void PixelTypeSpecieficFilter<CellType>::FillMargins_EmptyPixelBasedAlgo_ProcessNextPixel(int &y, int &x,
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
				if ((actualPixelDirection == PixelDirection::Up) ||
					(actualPixelDirection == PixelDirection::Down) ||
					(actualPixelDirection == PixelDirection::Right) ||
					(actualPixelDirection == PixelDirection::Left))
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
inline bool PixelTypeSpecieficFilter<CellType>::FillMargins_WindowIsEmpty(const int &windowY,
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
		if (direction == PixelDirection::Right)
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
		else if (direction == PixelDirection::Left)
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
		else if (direction == PixelDirection::Down)
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
inline bool PixelTypeSpecieficFilter<CellType>::FillMargins_FindNearestActualPixel(const int &startPixelY,
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
			currDirection = PixelDirection::Up;
		}
		else if (currDirection == PixelDirection::Up)
		{
			currDirection = PixelDirection::Down;
		}
		else if (currDirection == PixelDirection::Down)
		{
			currDirection = PixelDirection::Right;
		}
		else if (currDirection == PixelDirection::Right)
		{
			currDirection = PixelDirection::Left;
		}
		else if (currDirection == PixelDirection::Left)
		{
			currDirection = PixelDirection::UpRight;
		}
		else if (currDirection == PixelDirection::UpRight)
		{
			currDirection = PixelDirection::UpLeft;
		}
		else if (currDirection == PixelDirection::UpLeft)
		{
			currDirection = PixelDirection::DownRight;
		}
		else if (currDirection == PixelDirection::DownRight)
		{
			currDirection = PixelDirection::DownLeft;
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
		if (((currDirection == PixelDirection::Left) && pixelFinded) ||
			(currDirection == PixelDirection::DownLeft))
		{
			//Пиксель был найден по не-диагоналям. Проверять диагонали смысла нет.
			//Также продолжать поиск нет смысла если проверены уже вообще все направления.
			searchDone = true;
		}
	}
	//В любом случае - вернуть что получилось.
	return pixelFinded;
}

//Применит указанный (ссылкой на метод) фильтр к изображению. Входящий и исходящий файлы
//уже должны быть подключены. Вернёт false и инфу об ошибке если что-то пойдёт не так.
template <typename CellType>
bool PixelTypeSpecieficFilter<CellType>::ApplyFilter(CallBackBase *callBackObj, ErrorInfo *errObj)
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
			currMM = TopMarginMode::File1;
			currMatrix = NULL;
		}
		else
		{
			currMM = TopMarginMode::Matr;
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
		int fillerYStart, fillerYToProcess;
		if (currMM == TopMarginMode::File2)
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
		FillMargins(fillerYStart, fillerYToProcess, NULL);
		/*//Для переделанного заполнителя обработка граничных пикселей делается тупо целиком.
		FillMargins(0, filterYToProcess + 2*marginSize);*/
		//Также если используется квантованная матрица - нужно её обновить.
		if (sourceMatrix_.getUseQuantedData())
		{
			//Заполняется целиком. Можно оптимизировать и делать полную копию только в первый проход,
			//но пока - так.
			FillQuantedMatrix(0, filterYToProcess + 2*marginSize);
		}

		//Надо применить фильтр
		FilterMethod(filterYToProcess, callBackObj);

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
			0, marginSize, TopMarginMode::Matr, &sourceMatrix_, NULL);
		destMatrix_.CreateDestMatrix(sourceMatrix_, marginSize);
		//Всё ещё надо обработать граничные пиксели
		FillMargins(marginSize, filterYToProcess, NULL);
		//Также если используется квантованная матрица - нужно её обновить.
		if (sourceMatrix_.getUseQuantedData())
		{
			FillQuantedMatrix(marginSize, filterYToProcess);
		}
		//Всё ещё надо применить фильтр.
		FilterMethod(filterYToProcess, callBackObj);

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

//Метод вычисляет минимальную и максимальную высоту в открытом изображении если это вообще нужно.
//Также вычисляет дельту (шаг между уровнями).
template<typename CellType>
void PixelTypeSpecieficFilter<CellType>::CalcMinMaxPixelValues()
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
void PixelTypeSpecieficFilter<CellType>::FillMargins(const int yStart, const int yToProcess,
	CallBackBase *callBackObj)
{
	//Просто пробросим вызов в реально работающий метод, правильно указав метод-заполнитель.
	switch (getOwnerObj().getMarginType())
	{
	case MarginType::SimpleFilling:
		FillMargins_PixelBasedAlgo(&PixelTypeSpecieficFilter<CellType>::SimpleFiller, yStart, yToProcess,
		callBackObj);
		/*FillMargins_EmptyPixelBasedAlgo(&PixelTypeSpecieficFilter<CellType>::SimpleFiller, yStart, yToProcess,
			callBackObj);*/
		break;
	case MarginType::MirrorFilling:
		FillMargins_PixelBasedAlgo(&PixelTypeSpecieficFilter<CellType>::MirrorFiller, yStart, yToProcess,
			callBackObj);
		/*FillMargins_EmptyPixelBasedAlgo(&PixelTypeSpecieficFilter<CellType>::MirrorFiller, yStart, yToProcess,
			callBackObj);*/
	}
}

//Заполняет матрицу квантованных пикселей в указанном промежутке, получая их из значений
//оригинальных пикселей в исходной матрице. Нужно для алгоритма Хуанга.
template <typename CellType>
void PixelTypeSpecieficFilter<CellType>::FillQuantedMatrix(const int yStart, const int yToProcess)
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

//"Тупая" визуализация матрицы, отправляется прямо в cout.
template <typename CellType>
void PixelTypeSpecieficFilter<CellType>::SourcePrintStupidVisToCout()
{
	//Просто проброс вызова в объект матрицы.
	sourceMatrix_.PrintStupidVisToCout();
}

//Вывод исходной матрицы в csv-файл, который должны понимать всякие картографические
//программы. Это значит что каждый пиксел - это одна строка в файле.
//Это "тупой" вариант вывода - метаданные нормально не сохраняются.
template <typename CellType>
bool PixelTypeSpecieficFilter<CellType>::SourceSaveToCSVFile(const string &fileName, ErrorInfo *errObj)
{
	//Просто проброс вызова в объект матрицы.
	return sourceMatrix_.SaveToCSVFile(fileName, errObj);
}

//Вывод исходной квантованной матрицы в csv-файл.
template<typename CellType>
bool PixelTypeSpecieficFilter<CellType>::QuantedSaveToCSVFile(const string & fileName, ErrorInfo * errObj)
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
bool PixelTypeSpecieficFilter<CellType>::DestSaveToCSVFile(const string &fileName, ErrorInfo *errObj)
{
	//Просто проброс вызова в объект матрицы.
	return destMatrix_.SaveToCSVFile(fileName, errObj);
}

} //namespace geoimgconv