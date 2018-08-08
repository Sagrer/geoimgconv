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

//Наследник FilterBase, реализующий "тупой" фильтр. Алгоритм медианной
//фильтрации работает "в лоб". Результат записывается в выбранный destFile

#include "median_filter_stupid.h"
#include "real_median_filter_stupid.h"

using namespace std;

namespace geoimgconv
{
	
//////////////////////////////////////////
//          MedianFilterStupid          //
//////////////////////////////////////////

//Создать объект класса, который работает с изображениями с типом пикселей dataType_
//и соответствующим алгоритмом фильтрации и поместить указатель на него в pFilterObj_.
//Если в dataType_ лежит что-то неправильное то туда попадёт nullptr. Кроме того, метод
//должен установить правильный dataTypeSize_.
void MedianFilterStupid::NewFilterObj()
{
	switch (getDataType())
	{
	case PixelType::Int8:
		setFilterObj(new RealMedianFilterStupidInt8(this));
		setDataTypeSize(sizeof(int8_t));
		break;
	case PixelType::UInt8:
		setFilterObj(new RealMedianFilterStupidUInt8(this));
		setDataTypeSize(sizeof(uint8_t));
		break;
	case PixelType::Int16:
		setFilterObj(new RealMedianFilterStupidInt16(this));
		setDataTypeSize(sizeof(int16_t));
		break;
	case PixelType::UInt16:
		setFilterObj(new RealMedianFilterStupidUInt16(this));
		setDataTypeSize(sizeof(uint16_t));
		break;
	case PixelType::Int32:
		setFilterObj(new RealMedianFilterStupidInt32(this));
		setDataTypeSize(sizeof(int32_t));
		break;
	case PixelType::UInt32:
		setFilterObj(new RealMedianFilterStupidUInt32(this));
		setDataTypeSize(sizeof(uint32_t));
		break;
	case PixelType::Float32:
		setFilterObj(new RealMedianFilterStupidFloat32(this));
		setDataTypeSize(sizeof(float));
		break;
	case PixelType::Float64:
		setFilterObj(new RealMedianFilterStupidFloat64(this));
		setDataTypeSize(sizeof(double));
		break;
	default:
		setFilterObj(nullptr);
		setDataTypeSize(0);
		break;
	}
}

} //namespace geoimgconv