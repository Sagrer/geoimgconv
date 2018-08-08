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

//Наследник работающий с произвольным типом пикселя и реализующий "заглушечный"
//фильтр, который ничего реально не делает.

#include "pixel_type_speciefic_filter.h"

namespace geoimgconv
{

template <typename CellType> class RealMedianFilterStub : public PixelTypeSpecieficFilter<CellType>
{
public:
	//Запретим конструктор по умолчанию, копирующий и переносящие конструкторы и аналогичные
	//им операторы присваивания.
	RealMedianFilterStub() = delete;
	RealMedianFilterStub(const RealMedianFilterStub&) = delete;
	RealMedianFilterStub(RealMedianFilterStub&&) = delete;
	RealMedianFilterStub& operator=(const RealMedianFilterStub&) = delete;
	RealMedianFilterStub& operator=(RealMedianFilterStub&&) = delete;
	//Cоздать объект можно только передав ссылку на FilterBase
	RealMedianFilterStub(FilterBase *ownerObj) : PixelTypeSpecieficFilter<CellType>(ownerObj) {}

	//Этот полиморфный метод вызывается ApplyFilter-ом и обрабатывает изображение правильным алгоритмом.
	//В данном случае применяется "никакой" алгоритм, который ничего кроме копирования не делает.
	//Первый аргумент указывает количество строк матрицы для реальной обработки.
	void FilterMethod(const int &currYToProcess, CallBackBase *callBackObj) override;
};

//Алиасы для классов, работающих с реально использующимися в GeoTIFF типами пикселей.
using RealMedianFilterStubFloat64 = RealMedianFilterStub<double>;
using RealMedianFilterStubFloat32 = RealMedianFilterStub<float>;
using RealMedianFilterStubInt8 = RealMedianFilterStub<int8_t>;
using RealMedianFilterStubUInt8 = RealMedianFilterStub<uint8_t>;
using RealMedianFilterStubInt16 = RealMedianFilterStub<int16_t>;
using RealMedianFilterStubUInt16 = RealMedianFilterStub<uint16_t>;
using RealMedianFilterStubInt32 = RealMedianFilterStub<int32_t>;
using RealMedianFilterStubUInt32 = RealMedianFilterStub<uint32_t>;

}	//namespace geoimgconv