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

//Всякие разные штуки, общие для всего проекта.

#include "common.h"
#include <string>
#include "strings_tools_box.h"

using namespace std;

namespace geoimgconv
{

//Константы со всякими разными значениями по умолчанию.

//Временно версия будет вот так. Но нет ничего более постоянного, чем временное!
const string APP_VERSION = "0.4.1.0a";
//С какой периодичностью выводить прогресс в консоль во избежание тормозов от вывода. 
//В секундах.
const double DEFAULT_PROGRESS_UPDATE_PERIOD = 2.8;
//Имя входящего файла
const string DEFAULT_INPUT_FILE_NAME = "input.tif";
//Имя исходящего файла
const string DEFAULT_OUTPUT_FILE_NAME = "output.tif";
//Апертура медианного фильтра
const int DEFAULT_MEDFILTER_APERTURE = 101;
//Порог медианного фильтра
const double DEFAULT_MEDFILTER_THRESHOLD = 0.5;
//Какой тип заполнения граничных пикселей применяется по умолчанию
const MarginType DEFAULT_MEDFILTER_MARGIN_TYPE = MarginType::MirrorFilling;
//Режим работы программы
const AppMode DEFAULT_APP_MODE = AppMode::Median;
//Режим использования памяти
const MemoryMode DEFAULT_MEM_MODE = MemoryMode::Auto;
//Режим медианного фильтра по умолчанию.
const MedfilterAlgo DEFAULT_MEDFILTER_ALGO = MedfilterAlgo::Stupid;
//Количество уровней квантования для алгоритма Хуанга по умолчанию
const uint16_t DEFAULT_HUANG_LEVELS_NUM = 10000;
//Включён ли по умолчанию режим заполнения ям в медианном фильтре.
const bool DEFAULT_MEDFILTER_FILL_PITS = false;
//Максимально возможное количество уровней квантования для алгоритма Хуанга.
const uint16_t HUANG_MAX_LEVELS_NUM = -1; //65535

//Текстовое представление для AppMode
const string AppModeTexts[] = {"median",	//0
			"mediancurses",		//1
			"interactivecurses",	//2
			"mediangui",	//3
			"interactivegui",	//4
			"unknown"	//5
};

//Текстовое представление для MedfilterAlgo
const string MedfilterAlgoTexts[] = { "stub",	//0
			"stupid",		//1
			"huang",		//2
			"unknown"		//3
};

//Текстовое представление элементов MarginType
const string MarginTypesTexts[] = {"simple",	//0
			"mirror",	//1
			"unknown"	//2
};

//Текстовое представление для MemoryMode (без цифр)
const string MemoryModeTexts[] = { "auto",	//0
			"limit",				//1
			"limit_free_prc",		//2
			"limit_full_prc",		//3
			"stayfree",				//4
			"onechunk",				//5
			"unknown"				//6
};

//Преобразование типа пикселя из enum-а GDALDataType в PixelType.
//Важный момент - если тип был PixelType::Int8 - невозможно просто так понять
//signed там или unsigned. Для того чтобы отличить одно от другого - надо
//в MAGE_STRUCTURE Domain посмотреть значение PIXELTYPE
PixelType GDALToGIC_PixelType(const GDALDataType &GDALType)
{
	switch (GDALType)
	{
	case GDT_Byte: return PixelType::Int8;
	case GDT_UInt16: return PixelType::UInt16;
	case GDT_Int16: return PixelType::Int16;
	case GDT_UInt32: return PixelType::UInt32;
	case GDT_Int32: return PixelType::Int32;
	case GDT_Float32: return PixelType::Float32;
	case GDT_Float64: return PixelType::Float64;
	default: return PixelType::Unknown;
	}
}

//Преобразование обратно из PixelType в GDALDataType
GDALDataType GICToGDAL_PixelType(const PixelType GICType)
{
	switch (GICType)
	{
	case PixelType::Int8: return GDT_Byte;
	case PixelType::UInt8: return GDT_Byte;
	case PixelType::UInt16: return GDT_UInt16;
	case PixelType::Int16: return GDT_Int16;
	case PixelType::UInt32: return GDT_UInt32;
	case PixelType::Int32: return GDT_Int32;
	case PixelType::Float32: return GDT_Float32;
	case PixelType::Float64: return GDT_Float64;
	default: return GDT_Unknown;
	}
}

//Обработчик ошибок GDAL. Указатель на него должен быть подсунут из каждого
//потока приложения через CPLPushErrorHandlerEx. Дополнительные данные
//- указатель на string под текст сообщения об ошибке. Строка должна
//быть для каждого потока своя.
void CPL_STDCALL GDALThreadErrorHandler(CPLErr eErrClass, int err_no, const char *msg)
{
	//Тупо кладём в настроенную для хендлера строку текст сообщения об ошибке.
	*((string*)(CPLGetErrorHandlerUserData())) = msg;
}

//Вернуть строку с текстом последней ошибки GDAL для данного потока. Работает только
//если был подключен обработчик GDALThreadErrorHandler
string &GetLastGDALError()
{
	return *((string*)(CPLGetErrorHandlerUserData()));
}

//Получить AppMode из строки, совпадающей без учёта регистра с одним из
//элементов AppModeTexts
AppMode AppModeStrToEnum(const string &inputStr)
{
	string inpStr;
	StrTB::Utf8ToLower(inputStr, inpStr);
	for (unsigned char i = 0; i <= (unsigned char)AppMode::Unknown; i++)
	{
		if (inpStr == AppModeTexts[i])
			return AppMode(i);
	}
	return AppMode::Unknown;
}

//Получить MarginType из строки, совпадающей без учёта регистра с одним из
//элементов MarginTypeTexts
MarginType MarginTypeStrToEnum(const string &inputStr)
{
	string inpStr;
	StrTB::Utf8ToLower(inputStr, inpStr);
	for (unsigned char i = 0; i <= (unsigned char)MarginType::UnknownFilling; i++)
	{
		if (inpStr == MarginTypesTexts[i])
			return MarginType(i);
	}
	return MarginType::UnknownFilling;
}

//Получить MedfilterAlgo из строки, совпадающей без учёта регистра с одним из
//элементов MedfilterAlgoTexts
MedfilterAlgo MedfilterAlgoStrToEnum(const string &inputStr)
{
	string inpStr;
	StrTB::Utf8ToLower(inputStr, inpStr);
	for (unsigned char i = 0; i <= (unsigned char)MedfilterAlgo::Unknown; i++)
	{
		if (inpStr == MedfilterAlgoTexts[i])
			return MedfilterAlgo(i);
	}
	return MedfilterAlgo::Unknown;
}

//Получить противоположное направление движения по пикселям.
PixelDirection RevertPixelDirection(const PixelDirection & value)
{
	if (value == PixelDirection::Up)
	{
		return PixelDirection::Down;
	}
	else if (value == PixelDirection::Down)
	{
		return PixelDirection::Up;
	}
	else if (value == PixelDirection::Right)
	{
		return PixelDirection::Left;
	}
	else if (value == PixelDirection::Left)
	{
		return PixelDirection::Right;
	}
	else if (value == PixelDirection::UpRight)
	{
		return PixelDirection::DownLeft;
	}
	else if (value == PixelDirection::DownLeft)
	{
		return PixelDirection::UpRight;
	}
	else if (value == PixelDirection::UpLeft)
	{
		return PixelDirection::DownRight;
	}
	else if (value == PixelDirection::DownRight)
	{
		return PixelDirection::UpLeft;
	}
	else return PixelDirection::Unknown;
}

}		//namespace geoimgconv