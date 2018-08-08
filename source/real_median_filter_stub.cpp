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

//Наследник работающий с произвольным типом пикселя и реализующий "заглушечный"
//фильтр, который ничего реально не делает.

#include "real_median_filter_stub.h"
#include "alt_matrix.h"

namespace geoimgconv
{

//Инстанциация по шаблону для тех типов, которые реально будут использоваться. Таким образом,
//код для них будет генерироваться только в одной единице компилляции.
template class RealMedianFilterStub<double>;
template class RealMedianFilterStub<float>;
template class RealMedianFilterStub<int8_t>;
template class RealMedianFilterStub<uint8_t>;
template class RealMedianFilterStub<int16_t>;
template class RealMedianFilterStub<uint16_t>;
template class RealMedianFilterStub<int32_t>;
template class RealMedianFilterStub<uint32_t>;

//Этот полиморфный метод вызывается ApplyFilter-ом и обрабатывает изображение правильным алгоритмом.
//В данном случае применяется "никакой" алгоритм, который ничего кроме копирования не делает.
//Первый аргумент указывает количество строк матрицы для реальной обработки.
template<typename CellType>
void RealMedianFilterStub<CellType>::FilterMethod(const int &currYToProcess, CallBackBase *callBackObj)
{
	int destX, destY, sourceX, sourceY, marginSize;
	//Тут будет this при обращении к элементам предков. Всё потому что С++ различает зависимые от шаблонов
	//и независимые от шаблонов элементы, парсит всё в 2 этапа и не может быть уверен что обращение
	//без this означает именно элемент, унаследованный от предка а не что-то в глобальном пространстве имён.
	marginSize = (this->getOwnerObj().getAperture() - 1) / 2;
	unsigned long progressPosition = this->getOwnerObj().getCurrPositionY() * this->destMatrix_.getXSize();

	//Поехали.
	for (destY = 0; destY < currYToProcess; destY++)
	{
		sourceY = destY + marginSize;
		for (destX = 0; destX < this->destMatrix_.getXSize(); destX++)
		{
			sourceX = destX + marginSize;
			progressPosition++;
			//Незначимые пиксели не трогаем.
			if (this->sourceMatrix_.getSignMatrixElem(sourceY, sourceX) != 1) continue;

			//Просто копируем пиксел.
			this->destMatrix_.setMatrixElem(destY, destX,
				this->sourceMatrix_.getMatrixElem(sourceY, sourceX));

			//Сообщить вызвавшему коду прогресс выполнения.
			if (callBackObj) callBackObj->CallBack(progressPosition);
		};
	};
}

}	//namespace geoimgconv