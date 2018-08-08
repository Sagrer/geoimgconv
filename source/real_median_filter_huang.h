#pragma once

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

#include "pixel_type_speciefic_filter.h"

namespace geoimgconv
{

template <typename CellType> class RealMedianFilterHuang : public PixelTypeSpecieficFilter<CellType>
{
public:
	//Запретим конструктор по умолчанию, копирующий и переносящие конструкторы и аналогичные
	//им операторы присваивания.
	RealMedianFilterHuang() = delete;
	RealMedianFilterHuang(const RealMedianFilterHuang&) = delete;
	RealMedianFilterHuang(RealMedianFilterHuang&&) = delete;
	RealMedianFilterHuang& operator=(const RealMedianFilterHuang&) = delete;
	RealMedianFilterHuang& operator=(RealMedianFilterHuang&&) = delete;
	//Cоздать объект можно только передав ссылку на FilterBase
	RealMedianFilterHuang(FilterBase *ownerObj) : PixelTypeSpecieficFilter<CellType>(ownerObj) {}

	//Этот полиморфный метод вызывается ApplyFilter-ом и обрабатывает изображение правильным алгоритмом.
	//В данном случае применяется
	//Первый аргумент указывает количество строк матрицы для реальной обработки.
	void FilterMethod(const int &currYToProcess, CallBackBase *callBackObj) override;
private:
	//Вспомогательный метод для алгоритма Хуанга. Цикл по строке вправо.
	inline void HuangFilter_ProcessStringToRight(int &destX, int &destY, int &sourceX,
		int &sourceY, const int &marginSize, unsigned long &progressPosition, bool &gistIsActual,
		bool &gistIsEmpty, unsigned long *gist, uint16_t &median, unsigned long &elemsLeftMed,
		int &oldY, int &oldX, const uint16_t &halfMedPos, CallBackBase *callBackObj);

	//Вспомогательный метод для алгоритма Хуанга. Цикл по строке влево.
	//Два почти одинаковых метода здесь чтобы внутри не делать проверок направления
	//за счёт чего оно может быть будет работать немного быстрее.
	inline void HuangFilter_ProcessStringToLeft(int &destX, int &destY, int &sourceX,
		int &sourceY, const int &marginSize, unsigned long &progressPosition, bool &gistIsActual,
		bool &gistIsEmpty, unsigned long *gist, uint16_t &median, unsigned long &elemsLeftMed,
		int &oldY, int &oldX, const uint16_t &halfMedPos, CallBackBase *callBackObj);

	//Вспомогательный метод для алгоритма Хуанга. Заполняет гистограмму с нуля. В параметрах координаты
	//верхнего левого угла апертуры.
	inline void HuangFilter_FillGist(const int &leftUpY, const int &leftUpX, unsigned long *gist,
		uint16_t &median, unsigned long &elemsLeftMed,
		const uint16_t &halfMedPos);

	//Вспомогательный метод для алгоритма Хуанга. Выполняет шаг вправо.
	inline void HuangFilter_DoStepRight(const int &leftUpY, const int &leftUpX, unsigned long *gist,
		const uint16_t &median, unsigned long &elemsLeftMed);

	//Вспомогательный метод для алгоритма Хуанга. Выполняет шаг влево.
	inline void HuangFilter_DoStepLeft(const int &leftUpY, const int &leftUpX, unsigned long *gist,
		const uint16_t &median, unsigned long &elemsLeftMed);

	//Вспомогательный метод для алгоритма Хуанга. Выполняет шаг вниз.
	inline void HuangFilter_DoStepDown(const int &leftUpY, const int &leftUpX, unsigned long *gist,
		const uint16_t &median, unsigned long &elemsLeftMed);

	//Вспомогательный метод для алгоритма Хуанга. Корректирует медиану.
	inline void HuangFilter_DoMedianCorrection(uint16_t &median, unsigned long &elemsLeftMed,
		const uint16_t &halfMedPos, unsigned long *gist);

	//Вспомогательный метод для алгоритма Хуанга. Запись нового значения пикселя в матрицу
	//назначения.
	inline void HuangFilter_WriteDestPixel(const int &destY, const int &destX, const int &sourceY,
		const int &sourceX, const uint16_t &median);
};

//Алиасы для классов, работающих с реально использующимися в GeoTIFF типами пикселей.
using RealMedianFilterHuangFloat64 = RealMedianFilterHuang<double>;
using RealMedianFilterHuangFloat32 = RealMedianFilterHuang<float>;
using RealMedianFilterHuangInt8 = RealMedianFilterHuang<int8_t>;
using RealMedianFilterHuangUInt8 = RealMedianFilterHuang<uint8_t>;
using RealMedianFilterHuangInt16 = RealMedianFilterHuang<int16_t>;
using RealMedianFilterHuangUInt16 = RealMedianFilterHuang<uint16_t>;
using RealMedianFilterHuangInt32 = RealMedianFilterHuang<int32_t>;
using RealMedianFilterHuangUInt32 = RealMedianFilterHuang<uint32_t>;

}	//namespace geoimgconv