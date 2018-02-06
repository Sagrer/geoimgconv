#pragma once

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

#pragma warning(push)
//Потому что библиотечный хидер даёт ворнинги :(
#pragma warning(disable:4251)
#include <gdal_priv.h>
#pragma warning(pop)

namespace geoimgconv
{

enum MarginType : unsigned char
//Возможные типы заполнения "пустых" пикселей чтобы можно было обсчитывать краевые пиксели.
{
	MARGIN_SIMPLE_FILLING = 0,	//- заполняется просто копией краевых пикселей.
	MARGIN_MIRROR_FILLING = 1,	//- заполняется зеркальным отображением пикселей за краевым на глубину в половину апертуры.
	MARGIN_UNKNOWN_FILLING = 2	//- тип заполнения неизвестен. Элемент ДОЛЖЕН быть последним!
	//По нему определяется количество элементов!
};

//Текстовое представление элементов MarginType
extern const std::string MarginTypesTexts[];

enum PixelType
//Типы пикселей в изображениях, поддерживаемые geoimgeconv.
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

enum PixelDirection
//Направление движения по пикселям
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

enum AppMode : unsigned char
//Режим работы программы, задаваемый при запуске
{
	APPMODE_MEDIAN = 0,	//Просто медианный фильтр в консоли.
	APPMODE_MEDIAN_CURSES = 1,	//Медианный фильтр в curses-интерфейсе.
	APPMODE_INTERACTIVE_CURSES = 2,	//Интерактивный режим в curses-интерфейсе.
	APPMODE_MEDIAN_GUI = 3,	//Медианный фильтр в графическом интерфейсе.
	APPMODE_INTERACTIVE_GUI = 4,	//Интерактивный режим в графическом интерфейсе.
	APPMODE_UNKNOWN = 5	//Неизвестно что. На случай если юзер напишет непонятную фигню.
	//APPMODE_UNKNOWN ДОЛЖЕН быть последним, по нему определяется количество элементов!
};

//Текстовое представление для AppMode
extern const std::string AppModeTexts[];

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
//элементов GIC_MarginTypeTexts
MarginType MarginTypeStrToEnum(const std::string &inputStr);

class CallBackBase
//Абстрактный класс для организации вызова калбеков. Можно было бы передавать просто
//ссылки на функцию, но тогда эти функции будут отвязаны от данных потока в котором
//должны выполняться. Поэтому имхо проще передать вместо ссылки на функцию ссылку на
//объект, в котором есть и вызываемый метод и привязанные к этому вызову данные.
//Данный класс - абстрактный, от него нужно унаследовать конкретную реализацию с нужным
//функционалом.
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