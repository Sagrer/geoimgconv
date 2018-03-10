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
enum MarginType : unsigned char
{
	MARGIN_SIMPLE_FILLING = 0,	//- заполняется просто копией краевых пикселей.
	MARGIN_MIRROR_FILLING = 1,	//- заполняется зеркальным отображением пикселей за краевым на глубину в половину апертуры.
	MARGIN_UNKNOWN_FILLING = 2	//- тип заполнения неизвестен. Элемент ДОЛЖЕН быть последним!
	//По нему определяется количество элементов!
};

//Текстовое представление элементов MarginType
extern const std::string MarginTypesTexts[];

//Типы пикселей в изображениях, поддерживаемые geoimgeconv.
enum PixelType
{
	PIXEL_UNKNOWN,
	PIXEL_INT8,
	PIXEL_UINT8,
	PIXEL_INT16,
	PIXEL_UINT16,
	PIXEL_INT32,
	PIXEL_UINT32,
	PIXEL_FLOAT32,
	PIXEL_FLOAT64
};

//Направление движения по пикселям
enum PixelDirection
{
	PIXEL_DIR_UP,
	PIXEL_DIR_DOWN,
	PIXEL_DIR_RIGHT,
	PIXEL_DIR_LEFT,
	PIXEL_DIR_UP_RIGHT,
	PIXEL_DIR_UP_LEFT,
	PIXEL_DIR_DOWN_RIGHT,
	PIXEL_DIR_DOWN_LEFT
};

//Режим работы программы, задаваемый при запуске
enum AppMode : unsigned char
{
	APPMODE_MEDIAN = 0,	//Просто медианный фильтр в консоли.
	APPMODE_MEDIAN_CURSES = 1,	//Медианный фильтр в curses-интерфейсе.
	APPMODE_INTERACTIVE_CURSES = 2,	//Интерактивный режим в curses-интерфейсе.
	APPMODE_MEDIAN_GUI = 3,	//Медианный фильтр в графическом интерфейсе.
	APPMODE_INTERACTIVE_GUI = 4,	//Интерактивный режим в графическом интерфейсе.
	APPMODE_DEVTEST = 5,		//Режим тестирования чего-нибудь в консоли. Только для разработки\отладки, не документировать в справке юзеру!
	APPMODE_UNKNOWN = 6	//Неизвестно что. На случай если юзер напишет непонятную фигню.
	//APPMODE_UNKNOWN ДОЛЖЕН быть последним, по нему определяется количество элементов!
};

//Текстовое представление для AppMode
extern const std::string AppModeTexts[];

//Режим использования памяти программой
enum MemoryMode : unsigned char
{
	MEMORY_MODE_AUTO = 0,		//Выберать режим на своё усмотрение.
	MEMORY_MODE_LIMIT = 1,		//Занять под рабочее множество не более указанного количества памяти.
	MEMORY_MODE_LIMIT_FREEPRC = 2,	//Занять не более указанного процента свободной памяти.
	MEMORY_MODE_LIMIT_FULLPRC = 3,	//Занять не более указанного процента от всего ОЗУ.
	MEMORY_MODE_STAYFREE = 4,	//Занять столько памяти чтобы в ОЗУ осталось не менее указанного количества.
	MEMORY_MODE_ONECHUNK = 5,	//Попытаться загрузить изображение в память целиком и обработать одним куском.
	MEMORY_MODE_UNKNOWN = 6		//Режим неизвестен. Это признак ошибки. Должен быть последним в enum-е.
};

//Текстовое представление для MemoryMode (без цифр)
extern const std::string MemoryModeTexts[];

//Режим чтения верхней граничной части изображения.
enum TopMarginMode : unsigned char
{
	TOP_MM_FILE1 = 0,	//Читать 1 блок из файла (только второй, т.е. ниже самого верхнего).
	TOP_MM_FILE2 = 1,	//Читать 2 блока из файла.
	TOP_MM_MATR = 3		//Скопировать 2 блока из конца другой (или той же) матрицы.
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
extern const size_t DEFAULT_MEDFILTER_APERTURE;
//Порог медианного фильтра
extern const double DEFAULT_MEDFILTER_THRESHOLD;
//Какой тип заполнения граничных пикселей применяется по умолчанию
extern const MarginType DEFAULT_MEDFILTER_MARGIN_TYPE;
//Режим работы программы
extern const AppMode DEFAULT_APP_MODE;
//Режим использования памяти
extern const MemoryMode DEFAULT_MEM_MODE;

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

//Абстрактный класс для организации вызова калбеков. Можно было бы передавать просто
//ссылки на функцию, но тогда эти функции будут отвязаны от данных потока в котором
//должны выполняться. Поэтому имхо проще передать вместо ссылки на функцию ссылку на
//объект, в котором есть и вызываемый метод и привязанные к этому вызову данные.
//Данный класс - абстрактный, от него нужно унаследовать конкретную реализацию с нужным
//функционалом.
class CallBackBase
{
	private:
		unsigned long maxProgress_;	//Значение при котором прогресс считается 100%
	public:
		
		CallBackBase() : maxProgress_(100) {};
		~CallBackBase() {};

		//Доступ к полям

		unsigned long const& getMaxProgress() const { return maxProgress_; }
		void setMaxProgress(const unsigned long &maxProgress) { maxProgress_ = maxProgress; }

		//Собсно каллбек.
		virtual void CallBack(const unsigned long &progressPosition)=0;
		//Сообщить объекту о том что операция начинается
		virtual void OperationStart()=0;
		//Сообщить объекту о том что операция завершена.
		virtual void OperationEnd()=0;
};

}	//namespace geoimgconv