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

//Всякие разные штуки, общие для всего проекта.

#pragma warning(push)
//Потому что библиотечный хидер даёт ворнинги :(
#pragma warning(disable:4251)
#include <gdal_priv.h>
#pragma warning(pop)

namespace geoimgconv
{

//Возможные типы заполнения "пустых" пикселей чтобы можно было обсчитывать краевые пиксели.
enum class MarginType : unsigned char
{
	SimpleFilling = 0,	//- заполняется просто копией краевых пикселей.
	MirrorFilling = 1,	//- заполняется зеркальным отображением пикселей за краевым на глубину в половину апертуры.
	UnknownFilling = 2	//- тип заполнения неизвестен. Элемент ДОЛЖЕН быть последним!
	//По нему определяется количество элементов!
};

//Текстовое представление элементов MarginType
extern const std::string MarginTypesTexts[];

//Типы пикселей в изображениях, поддерживаемые geoimgeconv.
enum class PixelType
{
	Unknown,
	Int8,
	UInt8,
	Int16,
	UInt16,
	Int32,
	UInt32,
	Float32,
	Float64
};

//Направление движения по пикселям
enum class PixelDirection
{
	Up,
	Down,
	Right,
	Left,
	UpRight,
	UpLeft,
	DownRight,
	DownLeft,
	Unknown
};

//Режим работы программы, задаваемый при запуске
enum class AppMode : unsigned char
{
	Median = 0,	//Просто медианный фильтр в консоли.
	MedianCurses = 1,	//Медианный фильтр в curses-интерфейсе.
	InteractiveCurses = 2,	//Интерактивный режим в curses-интерфейсе.
	MedianGui = 3,	//Медианный фильтр в графическом интерфейсе.
	InteractiveGui = 4,	//Интерактивный режим в графическом интерфейсе.
	DevTest = 5,		//Режим тестирования чего-нибудь в консоли. Только для разработки\отладки, не документировать в справке юзеру!
	Unknown = 6	//Неизвестно что. На случай если юзер напишет непонятную фигню.
	//Unknown ДОЛЖЕН быть последним, по нему определяется количество элементов!
};

//Текстовое представление для AppMode
extern const std::string AppModeTexts[];

//Режим работы медианного фильтра (используемый алгоритм)
enum class MedfilterAlgo : unsigned char
{
	Stub = 0,	//Фильтр делает ничего, просто копирует пиксели.
	Stupid = 1,	//Фильтр работает "в лоб", без особых заморочек и оптимизаций. Без потери точности.
	Huang = 2,	//Фильтр работает по алгоритму быстрой медианной фильтрации Хуанга. Возможна потеря точности в связи с квантованием изображения по уровням.
	Unknown = 3	//Неизвестно что. Должно быть последним по номеру в списке enum-а!
};

//Текстовое представление для MedfilterAlgo
extern const std::string MedfilterAlgoTexts[];

//Режим использования памяти программой
enum class MemoryMode : unsigned char
{
	Auto = 0,		//Выберать режим на своё усмотрение.
	Limit = 1,		//Занять под рабочее множество не более указанного количества памяти.
	LimitFreePrc = 2,	//Занять не более указанного процента свободной памяти.
	LimitFullPrc = 3,	//Занять не более указанного процента от всего ОЗУ.
	StayFree = 4,	//Занять столько памяти чтобы в ОЗУ осталось не менее указанного количества.
	OneChunk = 5,	//Попытаться загрузить изображение в память целиком и обработать одним куском.
	Unknown = 6		//Режим неизвестен. Это признак ошибки. Должен быть последним в enum-е.
};

//Текстовое представление для MemoryMode (без цифр)
extern const std::string MemoryModeTexts[];

//Режим чтения верхней граничной части изображения.
enum class TopMarginMode : unsigned char
{
	File1 = 0,	//Читать 1 блок из файла (только второй, т.е. ниже самого верхнего).
	File2 = 1,	//Читать 2 блока из файла.
	Matr = 3		//Скопировать 2 блока из конца другой (или той же) матрицы.
};

//Константы со всякими разными значениями по умолчанию. Значения см. в common.cpp

//Временно версия будет вот так. Но нет ничего более постоянного, чем временное!
extern const std::string APP_VERSION;
//С какой периодичностью выводить прогресс в консоль во избежание тормозов от вывода. 
//В секундах.
extern const double DEFAULT_PROGRESS_UPDATE_PERIOD;
//Имя входящего файла
extern const std::string DEFAULT_INPUT_FILE_NAME;
//Имя исходящего файла
extern const std::string DEFAULT_OUTPUT_FILE_NAME;
//Апертура медианного фильтра
extern const int DEFAULT_MEDFILTER_APERTURE;
//Порог медианного фильтра
extern const double DEFAULT_MEDFILTER_THRESHOLD;
//Какой тип заполнения граничных пикселей применяется по умолчанию
extern const MarginType DEFAULT_MEDFILTER_MARGIN_TYPE;
//Режим работы программы
extern const AppMode DEFAULT_APP_MODE;
//Режим использования памяти
extern const MemoryMode DEFAULT_MEM_MODE;
//Режим медианного фильтра по умолчанию.
extern const MedfilterAlgo DEFAULT_MEDFILTER_ALGO;
//Количество уровней квантования для алгоритма Хуанга по умолчанию
extern const uint16_t DEFAULT_HUANG_LEVELS_NUM;
//Включён ли по умолчанию режим заполнения ям в медианном фильтре.
extern const bool DEFAULT_MEDFILTER_FILL_PITS;
//Максимально возможное количество уровней квантования для алгоритма Хуанга.
extern const uint16_t HUANG_MAX_LEVELS_NUM;

//Функции всякие, но такие что нельзя засунуть в small_tools_box т.к. специфичны
//для данной конкретной программы.

//Преобразование типа пикселя из enum-а GDALDataType в PixelType.
//Важный момент - если тип был PIXEL_INT8 - невозможно просто так понять
//signed там или unsigned. Для того чтобы отличить одно от другого - надо
//в MAGE_STRUCTURE Domain посмотреть значение PIXELTYPE
PixelType GDALToGIC_PixelType(const GDALDataType &GDALType);

//Преобразование обратно из PixelType в GDALDataType
GDALDataType GICToGDAL_PixelType(const PixelType GICType);

//Обработчик ошибок GDAL. Указатель на него должен быть подсунут из каждого
//потока приложения через CPLPushErrorHandlerEx. Дополнительные данные
//- указатель на std::string под текст сообщения об ошибке. Строка должна
//быть для каждого потока своя.
void CPL_STDCALL GDALThreadErrorHandler(CPLErr eErrClass, int err_no, const char *msg);

//Вернуть строку с текстом последней ошибки GDAL для данного потока. Работает только
//если был подключен обработчик GDALThreadErrorHandler
std::string &GetLastGDALError();

//Получить AppMode из строки, совпадающей без учёта регистра с одним из
//элементов AppModeTexts
AppMode AppModeStrToEnum(const std::string &inputStr);

//Получить MarginType из строки, совпадающей без учёта регистра с одним из
//элементов MarginTypeTexts
MarginType MarginTypeStrToEnum(const std::string &inputStr);

//Получить MedfilterAlgo из строки, совпадающей без учёта регистра с одним из
//элементов MedfilterAlgoTexts
MedfilterAlgo MedfilterAlgoStrToEnum(const std::string &inputStr);

//Получить противоположное направление движения по пикселям.
PixelDirection RevertPixelDirection(const PixelDirection &value);

}	//namespace geoimgconv