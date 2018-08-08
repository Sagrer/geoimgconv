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

//Наследник работающий с произвольным типом пикселя и реализующий "тупой" медианный
//фильтр, который работает нагло и в лоб, а поэтому медленно.

#include "real_median_filter_stupid.h"
#include <memory>

using namespace std;

namespace geoimgconv
{

//Инстанциация по шаблону для тех типов, которые реально будут использоваться. Таким образом,
//код для них будет генерироваться только в одной единице компилляции.
template class RealMedianFilterStupid<double>;
template class RealMedianFilterStupid<float>;
template class RealMedianFilterStupid<int8_t>;
template class RealMedianFilterStupid<uint8_t>;
template class RealMedianFilterStupid<int16_t>;
template class RealMedianFilterStupid<uint16_t>;
template class RealMedianFilterStupid<int32_t>;
template class RealMedianFilterStupid<uint32_t>;

//Этот полиморфный метод вызывается ApplyFilter-ом и обрабатывает изображение правильным алгоритмом.
//В данном случае применяется "тупой" алгоритм, работающий медленно и в лоб.
//Первый аргумент указывает количество строк матрицы для реальной обработки.
template<typename CellType>
void RealMedianFilterStupid<CellType>::FilterMethod(const int &currYToProcess, CallBackBase *callBackObj)
{
	//Данный метод применяет медианный фильтр "в лоб", т.ч. тупо для каждого пикселя создаёт
	//массив из всех пикселей, входящих в окно, после чего сортирует массив и медиану подставляет
	//на место пикселя в dest-матрице.
	int destX, destY, windowX, windowY, sourceX, sourceY, marginSize, medianArrPos;
	//Тут будет this при обращении к элементам предков. Всё потому что С++ различает зависимые от шаблонов
	//и независимые от шаблонов элементы, парсит всё в 2 этапа и не может быть уверен что обращение
	//без this означает именно элемент, унаследованный от предка а не что-то в глобальном пространстве имён.
	marginSize = (this->getOwnerObj().getAperture() - 1) / 2;
	//Сразу вычислим индексы и указатели чтобы не считать в цикле.
	//И выделим память под массив для пикселей окна, в котором ищем медиану.
	int medianArrSize = this->getOwnerObj().getAperture() * this->getOwnerObj().getAperture();
	unique_ptr<CellType[]> medianArr(new CellType[medianArrSize]);
	//Целочисленное деление, неточностью пренебрегаем ибо ожидаем окно со
	//стороной в десятки пикселей.
	CellType *medianPos = medianArr.get() + (medianArrSize / 2);
	CellType *medianArrEnd = medianArr.get() + medianArrSize;	//Элемент за последним в массиве.
	unsigned long progressPosition = this->getOwnerObj().getCurrPositionY() * this->destMatrix_.getXSize();

	//Поехали.
	//TODO: тут явно есть что поправить в плане оптимизации, есть лишние сложения.
	for (destY = 0; destY < currYToProcess; ++destY)
	{
		sourceY = destY + marginSize;
		for (destX = 0; destX < this->destMatrix_.getXSize(); ++destX)
		{
			sourceX = destX + marginSize;
			++progressPosition;
			//Незначимые пиксели не трогаем.
			if (this->sourceMatrix_.getSignMatrixElem(sourceY, sourceX) != 1) continue;
			//Теперь надо пройти по каждому пикселю окна чтобы составить массив
			//и сортировкой получить медиану. Наверное самый неэффективны способ.
			medianArrPos = 0;
			for (windowY = destY; windowY<(sourceY + marginSize + 1); ++windowY)
			{
				for (windowX = destX; windowX<(sourceX + marginSize + 1); ++windowX)
				{
					medianArr[medianArrPos] = this->sourceMatrix_.getMatrixElem(windowY, windowX);
					++medianArrPos;
				}
			}
			//Сортируем, берём медиану из середины. Точностью поиска середины не
			//заморачиваемся т.к. окна будут большие, в десятки пикселов.
			nth_element(medianArr.get(), medianPos, medianArrEnd);
			//Записываем в матрицу назначения либо медиану либо исходный пиксель, смотря
			//что там с порогом, с разницей между пикселем и медианой и включён ли режим
			//заполнения ям.
			if (this->UseMedian(*medianPos, this->sourceMatrix_.getMatrixElem(sourceY, sourceX)))
			{
				//Медиана.
				this->destMatrix_.setMatrixElem(destY, destX, *medianPos);
			}
			else
			{
				//Просто копируем пиксел.
				this->destMatrix_.setMatrixElem(destY, destX,
					this->sourceMatrix_.getMatrixElem(sourceY, sourceX));
			};
			//Сообщить вызвавшему коду прогресс выполнения.
			if (callBackObj) callBackObj->CallBack(progressPosition);
		};
	};
}

}	//namespace geoimgconv