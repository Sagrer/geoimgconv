///////////////////////////////////////////////////////////
//                                                       //
//                  GeoImageConverter                    //
//       Преобразователь изображений с геоданными        //
//       Copyright © 2017 Александр (Sagrer) Гриднев     //
//              Распространяется на условиях             //
//                 GNU GPL v3 или выше                   //
//                  см. файл gpl.txt                     //
//                                                       //
///////////////////////////////////////////////////////////

//Место для списка авторов данного конкретного файла (если изменения
//вносили несколько человек).

////////////////////////////////////////////////////////////////////////

//Всякие разные штуки, общие для всего проекта.

#include "common.h"
#include <string>
#include "small_tools_box.h"

namespace geoimgconv
{

//Текстовое представление для AppMode
const std::string AppModeTexts[] = {"median",	//0
			"mediancurses",		//1
			"interactivecurses",	//2
			"mediangui",	//3
			"interactivegui",	//4
			"unknown"	//5
};

//Текстовое представление элементов MarginType
const std::string MarginTypesTexts[] = {"simple",	//0
			"mirror",	//1
			"unknown"	//2
};

PixelType GDALToGIC_PixelType(const GDALDataType &GDALType)
//Преобразование типа пикселя из enum-а GDALDataType в PixelType.
//Важный момент - если тип был PIXEL_INT8 - невозможно просто так понять
//signed там или unsigned. Для того чтобы отличить одно от другого - надо
//в MAGE_STRUCTURE Domain посмотреть значение PIXELTYPE
{
	switch (GDALType)
	{
	case GDT_Byte: return PIXEL_INT8;
	case GDT_UInt16: return PIXEL_UINT16;
	case GDT_Int16: return PIXEL_INT16;
	case GDT_UInt32: return PIXEL_UINT32;
	case GDT_Int32: return PIXEL_INT32;
	case GDT_Float32: return PIXEL_FLOAT32;
	case GDT_Float64: return PIXEL_FLOAT64;
	default: return PIXEL_UNKNOWN;
	}
}

GDALDataType GICToGDAL_PixelType(const PixelType GICType)
//Преобразование обратно из PixelType в GDALDataType
{
	switch (GICType)
	{
	case PIXEL_INT8: return GDT_Byte;
	case PIXEL_UINT8: return GDT_Byte;
	case PIXEL_UINT16: return GDT_UInt16;
	case PIXEL_INT16: return GDT_Int16;
	case PIXEL_UINT32: return GDT_UInt32;
	case PIXEL_INT32: return GDT_Int32;
	case PIXEL_FLOAT32: return GDT_Float32;
	case PIXEL_FLOAT64: return GDT_Float64;
	default: return GDT_Unknown;
	}
}

void CPL_STDCALL GDALThreadErrorHandler(CPLErr eErrClass, int err_no, const char *msg)
//Обработчик ошибок GDAL. Указатель на него должен быть подсунут из каждого
//потока приложения через CPLPushErrorHandlerEx. Дополнительные данные
//- указатель на std::string под текст сообщения об ошибке. Строка должна
//быть для каждого потока своя.
{
	//Тупо кладём в настроенную для хендлера строку текст сообщения об ошибке.
	*((std::string*)(CPLGetErrorHandlerUserData())) = msg;
}

std::string &GetLastGDALError()
//Вернуть строку с текстом последней ошибки GDAL для данного потока. Работает только
//если был подключен обработчик GDALThreadErrorHandler
{
	return *((std::string*)(CPLGetErrorHandlerUserData()));
}

AppMode AppModeStrToEnum(const std::string &inputStr)
//Получить AppMode из строки, совпадающей без учёта регистра с одним из
//элементов AppModeTexts
{
	std::string inpStr;
	STB.Utf8ToLower(inputStr, inpStr);
	for (unsigned char i = 0; i <= APPMODE_UNKNOWN; i++)
	{
		if (inpStr == AppModeTexts[i])
			return AppMode(i);
	}
	return APPMODE_UNKNOWN;
}

MarginType MarginTypeStrToEnum(const std::string &inputStr)
//Получить MarginType из строки, совпадающей без учёта регистра с одним из
//элементов GIC_MarginTypeTexts
{
	std::string inpStr;
	STB.Utf8ToLower(inputStr, inpStr);
	for (unsigned char i = 0; i <= MARGIN_UNKNOWN_FILLING; i++)
	{
		if (inpStr == MarginTypesTexts[i])
			return MarginType(i);
	}
	return MARGIN_UNKNOWN_FILLING;
}

}		//namespace geoimgconv