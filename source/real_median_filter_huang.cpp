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

//Наследник работающий с произвольным типом пикселя и реализующий медианную фильтрацию
//по алгоритму Хуанга. Теоретически на больших окнах это очень быстрый
//алгоритм, единственный недостаток которого - потеря точности из за квантования.

#include "real_median_filter_huang.h"
#include <memory>

using namespace std;

namespace geoimgconv
{

//Инстанциация по шаблону для тех типов, которые реально будут использоваться. Таким образом,
//код для них будет генерироваться только в одной единице компилляции.
template class RealMedianFilterHuang<double>;
template class RealMedianFilterHuang<float>;
template class RealMedianFilterHuang<int8_t>;
template class RealMedianFilterHuang<uint8_t>;
template class RealMedianFilterHuang<int16_t>;
template class RealMedianFilterHuang<uint16_t>;
template class RealMedianFilterHuang<int32_t>;
template class RealMedianFilterHuang<uint32_t>;

//Тут будет this при обращении к элементам предков. Всё потому что С++ различает зависимые от шаблонов
//и независимые от шаблонов элементы, парсит всё в 2 этапа и не может быть уверен что обращение
//без this означает именно элемент, унаследованный от предка а не что-то в глобальном пространстве имён.

//Этот полиморфный метод вызывается ApplyFilter-ом и обрабатывает изображение правильным алгоритмом.
//В данном случае применяется
//Первый аргумент указывает количество строк матрицы для реальной обработки.
template<typename CellType>
void RealMedianFilterHuang<CellType>::FilterMethod(const int &currYToProcess, CallBackBase *callBackObj)
{
	//Создаём массив-гистограмму.
	unique_ptr<unsigned long[]> gist(new unsigned long[this->getOwnerObj().getHuangLevelsNum()]);
	//Текущее значение медианы
	uint16_t median;
	//Количество элементов в гистограмме, которые меньше медианы (т.е. левее её).
	unsigned long elemsLeftMed;
	//Позиция, в которой по идее должна быть расположена медиана.
	uint16_t halfMedPos = ((this->getOwnerObj().getAperture() * this->getOwnerObj().getAperture()) - 1) / 2;
	//Признак значимости гистограммы. Изначально гистограмма незначима и её нужно заполнить.
	bool gistIsActual = false;
	//Признак вообще наличия гистограммы. Второй признак нужен т.к. гистограмма может быть незначимой,
	//но при этом всё же существовать в устаревшем виде, что в некоторых случаях может быть использовано
	//для ускорения обработки (когда быстрее сделать гистограмму актуальной чем составить её с нуля).
	bool gistIsEmpty = true;
	//Счётчики и прочее что может понадобиться.
	int sourceX, sourceY, destX, destY, marginSize, oldY, oldX;
	marginSize = (this->getOwnerObj().getAperture() - 1) / 2;
	//Предыдущие значения координат меньше нуля - в процессе работы такие невозможны что обозначает
	//никакой пиксель.
	oldY = -1;
	oldX = -1;
	//Позиция прогрессбара.
	unsigned long progressPosition = this->getOwnerObj().getCurrPositionY() * this->destMatrix_.getXSize();

	//Основной цикл по строкам пикселей. Каждая итерация обрабатывает до 2 строк, сначала справа налево,
	//потом слева направо.
	destY = 0;
	sourceY = destY + marginSize;
	while (destY < currYToProcess)
	{
		//Тут первый подцикл.
		HuangFilter_ProcessStringToRight(destX, destY, sourceX, sourceY, marginSize, progressPosition,
			gistIsActual, gistIsEmpty, gist.get(), median, elemsLeftMed, oldY, oldX, halfMedPos, callBackObj);

		//На следующую строку.
		++destY;
		++sourceY;
		//Вываливаемся из цикла если вышли за границы.
		if (!(destY < currYToProcess)) break;

		//Тут второй подцикл.
		HuangFilter_ProcessStringToLeft(destX, destY, sourceX, sourceY, marginSize, progressPosition,
			gistIsActual, gistIsEmpty, gist.get(), median, elemsLeftMed, oldY, oldX, halfMedPos, callBackObj);

		//На следующую строку.
		++destY;
		++sourceY;
	}
}

//Вспомогательный метод для алгоритма Хуанга. Цикл по строке вправо.
template<typename CellType>
inline void RealMedianFilterHuang<CellType>::HuangFilter_ProcessStringToRight(int &destX, int &destY,
	int &sourceX, int &sourceY, const int &marginSize, unsigned long &progressPosition,
	bool &gistIsActual, bool &gistIsEmpty, unsigned long *gist, uint16_t &median,
	unsigned long &elemsLeftMed, int &oldY, int &oldX, const uint16_t &halfMedPos,
	CallBackBase *callBackObj)
{
	for (destX = 0, sourceX = destX + marginSize; destX < this->destMatrix_.getXSize(); ++destX, ++sourceX)
	{
		//Прогрессбар.
		progressPosition++;
		//Незначимые пиксели не трогаем.
		if (this->sourceMatrix_.getSignMatrixElem(sourceY, sourceX) != 1)
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
inline void RealMedianFilterHuang<CellType>::HuangFilter_ProcessStringToLeft(int &destX, int &destY,
	int &sourceX, int &sourceY, const int &marginSize, unsigned long &progressPosition,
	bool &gistIsActual, bool &gistIsEmpty, unsigned long *gist, uint16_t &median,
	unsigned long &elemsLeftMed, int &oldY, int &oldX, const uint16_t &halfMedPos,
	CallBackBase *callBackObj)
{
	int lastPixelX = this->destMatrix_.getXSize()-1;
	for (destX = lastPixelX, sourceX = destX + marginSize; destX >= 0;
		--destX, --sourceX)
	{
		//Прогрессбар.
		progressPosition++;
		//Незначимые пиксели не трогаем.
		if (this->sourceMatrix_.getSignMatrixElem(sourceY, sourceX) != 1)
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
inline void RealMedianFilterHuang<CellType>::HuangFilter_FillGist(const int &leftUpY, const int &leftUpX,
	unsigned long *gist, uint16_t &median, unsigned long &elemsLeftMed,
	const uint16_t & halfMedPos)
{
	//Счётчики и границы.
	int windowY, windowX, windowYEnd, windowXEnd;
	windowYEnd = leftUpY + this->getOwnerObj().getAperture();
	windowXEnd = leftUpX + this->getOwnerObj().getAperture();
	//Обнуляем гистограмму.
	fill(gist, gist + this->getOwnerObj().getHuangLevelsNum(), 0);
	//Проходим по пикселям апертуры и заполняем гистограмму.
	for (windowY = leftUpY; windowY < windowYEnd; ++windowY)
	{
		for (windowX = leftUpX; windowX < windowXEnd; ++windowX)
		{
			//Инкрементируем счётчик пикселей этого уровня в гистограмме.
			uint16_t temp = this->sourceMatrix_.getQuantedMatrixElem(windowY, windowX);
			++(gist[this->sourceMatrix_.getQuantedMatrixElem(windowY, windowX)]);
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
inline void RealMedianFilterHuang<CellType>::HuangFilter_DoStepRight(const int &leftUpY,
	const int &leftUpX, unsigned long *gist, const uint16_t &median,
	unsigned long &elemsLeftMed)
{
	//Счётчики и границы.
	int windowY, windowXLeft, windowXRight, windowYEnd;
	windowYEnd = leftUpY + this->getOwnerObj().getAperture();
	windowXLeft = leftUpX - 1;
	windowXRight = windowXLeft + this->getOwnerObj().getAperture();
	//Тут будет текущее квантованное значение пикселя.
	uint16_t currQuantedValue;

	//Удаляем из гистограммы колонку левее апертуры, добавляем последнюю (самую правую) колонку.
	for (windowY = leftUpY; windowY < windowYEnd; ++windowY)
	{
		//Левая колонка.
		currQuantedValue = this->sourceMatrix_.getQuantedMatrixElem(windowY, windowXLeft);
		--(gist[currQuantedValue]);
		if (currQuantedValue < median)
		{
			--elemsLeftMed;
		}
		//Правая колонка.
		currQuantedValue = this->sourceMatrix_.getQuantedMatrixElem(windowY, windowXRight);
		++(gist[currQuantedValue]);
		if (currQuantedValue < median)
		{
			++elemsLeftMed;
		}
	}
}

//Вспомогательный метод для алгоритма Хуанга. Выполняет шаг влево.
template<typename CellType>
inline void RealMedianFilterHuang<CellType>::HuangFilter_DoStepLeft(const int &leftUpY,
	const int &leftUpX, unsigned long *gist, const uint16_t &median,
	unsigned long &elemsLeftMed)
{
	//Счётчики и границы.
	int windowY, windowXRight, windowYEnd;
	windowYEnd = leftUpY + this->getOwnerObj().getAperture();
	windowXRight = leftUpX + this->getOwnerObj().getAperture();
	//Тут будет текущее квантованное значение пикселя.
	uint16_t currQuantedValue;

	//Удаляем из гистограммы колонку правее апертуры, добавляем первую (самую левую) колонку.
	for (windowY = leftUpY; windowY < windowYEnd; ++windowY)
	{
		//Левая колонка.
		currQuantedValue = this->sourceMatrix_.getQuantedMatrixElem(windowY, leftUpX);
		++(gist[currQuantedValue]);
		if (currQuantedValue < median)
		{
			++elemsLeftMed;
		}
		//Правая колонка.
		currQuantedValue = this->sourceMatrix_.getQuantedMatrixElem(windowY, windowXRight);
		--(gist[currQuantedValue]);
		if (currQuantedValue < median)
		{
			--elemsLeftMed;
		}
	}
}

//Вспомогательный метод для алгоритма Хуанга. Выполняет шаг вниз.
template<typename CellType>
inline void RealMedianFilterHuang<CellType>::HuangFilter_DoStepDown(const int &leftUpY,
	const int &leftUpX, unsigned long *gist, const uint16_t &median,
	unsigned long &elemsLeftMed)
{
	//Счётчики и границы.
	int windowX, windowYUp, windowYDown, windowXEnd;
	windowXEnd = leftUpX + this->getOwnerObj().getAperture();
	windowYUp = leftUpY - 1;
	windowYDown = windowYUp + this->getOwnerObj().getAperture();
	//Тут будет текущее квантованное значение пикселя.
	uint16_t currQuantedValue;

	//Удаляем из гистограммы строку выше апертуры, добавляем последнюю (самую нижнюю)
	//строку апертуры.
	for (windowX = leftUpX; windowX < windowXEnd; ++windowX)
	{
		//Верхняя строка.
		currQuantedValue = this->sourceMatrix_.getQuantedMatrixElem(windowYUp, windowX);
		--(gist[currQuantedValue]);
		if (currQuantedValue < median)
		{
			--elemsLeftMed;
		}
		//Нижняя строка.
		currQuantedValue = this->sourceMatrix_.getQuantedMatrixElem(windowYDown, windowX);
		++(gist[currQuantedValue]);
		if (currQuantedValue < median)
		{
			++elemsLeftMed;
		}
	}
}

//Вспомогательный метод для алгоритма Хуанга. Корректирует медиану.
template<typename CellType>
inline void RealMedianFilterHuang<CellType>::HuangFilter_DoMedianCorrection(uint16_t &median,
	unsigned long &elemsLeftMed, const uint16_t &halfMedPos, unsigned long *gist)
{
	if (elemsLeftMed > halfMedPos)
	{
		//Медиану надо двигать вправо, т.е. уменьшать.
		do
		{
			--median;
			elemsLeftMed -= gist[median];
		} while (elemsLeftMed > halfMedPos);
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
	uint16_t elemsLeftMed_test = 0;
	uint16_t median_test = 0;
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
inline void RealMedianFilterHuang<CellType>::HuangFilter_WriteDestPixel(const int &destY, const int &destX,
	const int &sourceY, const int &sourceX, const uint16_t &median)
{
	CellType newValue = this->QuantedValueToPixelValue(median);

	//Записываем в матрицу назначения либо медиану либо исходный пиксель, смотря
	//что там с порогом, с разницей между пикселем и медианой и включён ли режим
	//заполнения ям.
	if (this->UseMedian(newValue, this->sourceMatrix_.getMatrixElem(sourceY, sourceX)))
	{
		//Медиана.
		this->destMatrix_.setMatrixElem(destY, destX, newValue);
	}
	else
	{
		//Просто копируем пиксел.
		this->destMatrix_.setMatrixElem(destY, destX,
			this->sourceMatrix_.getMatrixElem(sourceY, sourceX));
	};
}

}	//namespace geoimgconv