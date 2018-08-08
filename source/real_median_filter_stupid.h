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

//Наследник работающий с произвольным типом пикселя и реализующий "тупой" медианный
//фильтр, который работает нагло и в лоб, а поэтому медленно.

#include "pixel_type_speciefic_filter.h"

namespace geoimgconv
{

template <typename CellType> class RealMedianFilterStupid : public PixelTypeSpecieficFilter<CellType>
{
public:
	//Запретим конструктор по умолчанию, копирующий и переносящие конструкторы и аналогичные
	//им операторы присваивания.
	RealMedianFilterStupid() = delete;
	RealMedianFilterStupid(const RealMedianFilterStupid&) = delete;
	RealMedianFilterStupid(RealMedianFilterStupid&&) = delete;
	RealMedianFilterStupid& operator=(const RealMedianFilterStupid&) = delete;
	RealMedianFilterStupid& operator=(RealMedianFilterStupid&&) = delete;
	//Cоздать объект можно только передав ссылку на FilterBase
	RealMedianFilterStupid(FilterBase *ownerObj) : PixelTypeSpecieficFilter<CellType>(ownerObj) {}

	//Этот полиморфный метод вызывается ApplyFilter-ом и обрабатывает изображение правильным алгоритмом.
	//В данном случае применяется "тупой" алгоритм, работающий медленно и в лоб.
	//Первый аргумент указывает количество строк матрицы для реальной обработки.
	void FilterMethod(const int &currYToProcess, CallBackBase *callBackObj) override;
};

//Алиасы для классов, работающих с реально использующимися в GeoTIFF типами пикселей.
using RealMedianFilterStupidFloat64 = RealMedianFilterStupid<double>;
using RealMedianFilterStupidFloat32 = RealMedianFilterStupid<float>;
using RealMedianFilterStupidInt8 = RealMedianFilterStupid<int8_t>;
using RealMedianFilterStupidUInt8 = RealMedianFilterStupid<uint8_t>;
using RealMedianFilterStupidInt16 = RealMedianFilterStupid<int16_t>;
using RealMedianFilterStupidUInt16 = RealMedianFilterStupid<uint16_t>;
using RealMedianFilterStupidInt32 = RealMedianFilterStupid<int32_t>;
using RealMedianFilterStupidUInt32 = RealMedianFilterStupid<uint32_t>;

}	//namespace geoimgconv