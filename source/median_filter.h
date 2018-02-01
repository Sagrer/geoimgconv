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

#include <boost/cstdint.hpp>
#include <boost/static_assert.hpp>
#include "alt_matrix.h"
#include "common.h"
#include <string>

namespace geoimgconv
{

//Семейство классов для работы с медианным фильтром.

//Базовый абстрактный класс.
class MedianFilterBase
{
private:
	//Поля
	int aperture_;		//Окно фильтра (длина стороны квадрата в пикселах). Должно быть нечётным.
	double threshold_;			//Порог фильтра. Если медиана отличается от значения пиксела меньше чем на порог - значение не будет изменено.
	//bool useMultiThreading_;		//Включает многопоточную обработку. Пока не реализовано.
	//unsigned int threadsNumber_;	//Количество потоков при многопоточной обработке. 0 - автоопределение. Пока не реализовано.
	MarginType marginType_;		//Тип заполнения краевых пикселей.
public:
	//Доступ к полям.

	//aperture
	int const& getAperture() const { return aperture_; }
	void setAperture(const int &aperture) { aperture_ = aperture; }
	//threshold
	double const& getThreshold() const { return threshold_; }
	void setThreshold(const double &threshold) { threshold_ = threshold; }
	////useMultiThreading
	//bool const& getUseMultiThreading() const { return useMultiThreading_; }
	//void setUseMultiThreading(const bool &useMultiThreading) { useMultiThreading_ = useMultiThreading; }
	////threadsNumber
	//bool const& getThreadsNumber() const { return threadsNumber_; }
	//void setThreadsNumber(const bool &threadsNumber) { threadsNumber_ = threadsNumber; }
	//marginType
	MarginType const& getMarginType() const { return marginType_; }
	void setMarginType(const MarginType &marginType) { marginType_ = marginType; }

	//Конструкторы-деструкторы
	MedianFilterBase();
	~MedianFilterBase();

	//Прочий функционал

	//Читает изображение в матрицу так чтобы по краям оставалось место для создания граничных
	//пикселей.
	virtual bool LoadImage(const std::string &fileName, ErrorInfo *errObj = NULL, CallBackBase *callBackObj=NULL) = 0;
		
	//Сохраняет матрицу в изображение. За основу берётся ранее загруженная через LoadImage
	//картинка - файл копируется под новым именем и затем в него вносятся изменённые пиксели.
	///В первую очередь это нужно чтобы оставить метаданные в неизменном оригинальном виде.
	virtual bool SaveImage(const std::string &fileName, ErrorInfo *errObj = NULL) = 0;

	//Заполняет граничные (пустые пиксели) области вокруг значимых пикселей в соответствии с
	//выбранным алгоритмом.
	virtual void FillMargins(CallBackBase *callBackObj = NULL) = 0;
		
	//Обрабатывает матрицу sourceMatrix_ "тупым" фильтром. Результат записывает в destMatrix_.
	virtual void ApplyStupidFilter(CallBackBase *callBackObj = NULL) = 0;

	//Обрабатывает матрицу sourceMatrix_ "никаким" фильтром. По сути просто копирование.
	//Для отладки. Результат записывает в destMatrix_.
	virtual void ApplyStubFilter(CallBackBase *callBackObj = NULL) = 0;

	//Приводит апертуру к имеющему смысл значению.
	void FixAperture();

	//"Тупая" визуализация матрицы, отправляется прямо в cout.
	virtual void SourcePrintStupidVisToCout() = 0;
		
	//Вывод исходной матрицы в csv-файл, который должны понимать всякие картографические
	//программы. Это значит что каждый пиксел - это одна строка в файле.
	//Это "тупой" вариант вывода - метаданные нормально не сохраняются.
	virtual bool SourceSaveToCSVFile(const std::string &fileName, ErrorInfo *errObj = NULL) = 0;
		
	//Аналогично SourceSaveToCSVFile, но для матрицы с результатом.
	virtual bool DestSaveToCSVFile(const std::string &fileName, ErrorInfo *errObj = NULL) = 0;
};


//Шаблонный класс для произвольного типа ячейки. Базовая версия (для специализации наследников)
template <typename CellType> class MedianFilterTemplBase : public MedianFilterBase
{
private:
	//Приватные поля.
	std::string sourceFileName_;
	AltMatrix<CellType> sourceMatrix_;	//Тут хранится самое ценное! То с чем работаем :).
	AltMatrix<CellType> destMatrix_;		//А тут ещё более ценное - результат ).
		
	//Приватные типы
		
	//Указатель на метод-заполнитель пикселей
	typedef void(MedianFilterTemplBase<CellType>::*TFillerMethod)(const int &x,
		const int &y, const PixelDirection direction, const int &marginSize);
		
	//Приватные методы

	//Сделать шаг по пиксельным координатам в указанном направлении.
	//Вернёт false если координаты ушли за границы изображения, иначе true.
	bool PixelStep(int &x, int &y, const PixelDirection direction);
		
	//Получает координаты действительного пикселя, зеркального для
	//currXY по отношению к центральному xy. Результат записывается
	//в outXY.
	void GetMirrorPixel(const int &x, const int &y, const int &currX, const int &currY, int &outX, int &outY, const int recurseDepth=0);

	//Заполнять пиксели простым алгоритмом в указанном направлении
	void SimpleFiller(const int &x, const int &y, const PixelDirection direction,
		const int &marginSize);
		
	//Заполнять пиксели зеркальным алгоритмом в указанном направлении
	void MirrorFiller(const int &x, const int &y, PixelDirection direction,
		const int &marginSize);
		
	//Костяк алгоритма, общий для Simple и Mirror
	void FillMargins_PixelBasedAlgo(const TFillerMethod FillerMethod,
		CallBackBase *callBackObj = NULL);

	//Заполнить пустые пиксели source-матрицы простым алгоритмом (сплошной цвет).
	void FillMargins_Simple(CallBackBase *callBackObj = NULL);

	//Заполнить пустые пиксели source-матрицы зеркальным алгоритмом.
	void FillMargins_Mirror(CallBackBase *callBackObj = NULL);
		
	//Вернёт положительную разницу между двумя значениями пикселя независимо от
	//типа данных в ячейках. Виртуальная версия для неизвестного типа.
	inline virtual CellType GetDelta(const CellType &value1, const CellType &value2)
	{
		if (value1 >= value2) return value1-value2;
		else return value2-value1;
	};
public:
	//Конструкторы-деструкторы
	MedianFilterTemplBase();
	~MedianFilterTemplBase();

	//Прочий функционал

	//Читает изображение в матрицу так чтобы по краям оставалось место для создания граничных
	//пикселей.
	bool LoadImage(const std::string &fileName, ErrorInfo *errObj = NULL, CallBackBase *callBackObj = NULL);
		
	//Сохраняет матрицу в изображение. За основу берётся ранее загруженная через LoadImage
	//картинка - файл копируется под новым именем и затем в него вносятся изменённые пиксели.
	///В первую очередь это нужно чтобы оставить метаданные в неизменном оригинальном виде.
	bool SaveImage(const std::string &fileName, ErrorInfo *errObj = NULL);

	//Заполняет граничные (пустые пиксели) области вокруг значимых пикселей в соответствии с
	//выбранным алгоритмом.
	void FillMargins(CallBackBase *callBackObj = NULL);
		
	//Обрабатывает матрицу sourceMatrix_ "тупым" фильтром. Результат записывает в destMatrix_.
	void ApplyStupidFilter(CallBackBase *callBackObj = NULL);

	//Обрабатывает матрицу sourceMatrix_ "никаким" фильтром. По сути просто копирование.
	//Для отладки. Результат записывает в destMatrix_.
	void ApplyStubFilter(CallBackBase *callBackObj = NULL);

	//"Тупая" визуализация матрицы, отправляется прямо в cout.
	void SourcePrintStupidVisToCout();
		
	//Вывод исходной матрицы в csv-файл, который должны понимать всякие картографические
	//программы. Это значит что каждый пиксел - это одна строка в файле.
	//Это "тупой" вариант вывода - метаданные нормально не сохраняются.
	bool SourceSaveToCSVFile(const std::string &fileName, ErrorInfo *errObj = NULL);
		
	//Аналогично SourceSaveToCSVFile, но для матрицы с результатом.
	bool DestSaveToCSVFile(const std::string &fileName, ErrorInfo *errObj = NULL);
};

//Шаблонный класс для произвольного типа ячейки. Пустой наследник (не специализированная
//версия. Через этот класс работают все unsigned-версии CellType.
template <typename CellType> class MedianFilter : public MedianFilterTemplBase<CellType>
{
};

//Специализация MedianFilter для double
template <> class MedianFilter<double> : public MedianFilterTemplBase<double>
{
private:
	//Вернёт положительную разницу между двумя значениями пикселя независимо от
	//типа данных в ячейках.
	inline double GetDelta(const double &value1, const double &value2)
	{
		return std::abs(value1-value2);
	};
};

//Специализация MedianFilter для float
template <> class MedianFilter<float> : public MedianFilterTemplBase<float>
{
private:
	//Вернёт положительную разницу между двумя значениями пикселя независимо от
	//типа данных в ячейках.
	inline float GetDelta(const float &value1, const float &value2)
	{
		return std::abs(value1-value2);
	};
};

//Специализация MedianFilter для boost::int8_t
template <> class MedianFilter<boost::int8_t> : public MedianFilterTemplBase<boost::int8_t>
{
private:
	//Вернёт положительную разницу между двумя значениями пикселя независимо от
	//типа данных в ячейках.
	inline boost::int8_t GetDelta(const boost::int8_t &value1, const boost::int8_t &value2)
	{
		return std::abs(value1-value2);
	};
};

//Специализация MedianFilter для boost::int16_t
template <> class MedianFilter<boost::int16_t> : public MedianFilterTemplBase<boost::int16_t>
{
private:
	//Вернёт положительную разницу между двумя значениями пикселя независимо от
	//типа данных в ячейках.
	inline boost::int16_t GetDelta(const boost::int16_t &value1, const boost::int16_t &value2)
	{
		return std::abs(value1-value2);
	};
};

//Специализация MedianFilter для boost::int32_t
template <> class MedianFilter<boost::int32_t> : public MedianFilterTemplBase<boost::int32_t>
{
private:
	//Вернёт положительную разницу между двумя значениями пикселя независимо от
	//типа данных в ячейках.
	inline boost::int32_t GetDelta(const boost::int32_t &value1, const boost::int32_t &value2)
	{
		return std::abs(value1-value2);
	};
};

//Алиасы для классов, работающих с реально использующимися в GeoTIFF типами пикселов.
typedef MedianFilter<double> MedianFilterFloat64;
typedef MedianFilter<float> MedianFilterFloat32;
typedef MedianFilter<boost::int8_t> MedianFilterInt8;
typedef MedianFilter<boost::uint8_t> MedianFilterUInt8;
typedef MedianFilter<boost::int16_t> MedianFilterInt16;
typedef MedianFilter<boost::uint16_t> MedianFilterUInt16;
typedef MedianFilter<boost::int32_t> MedianFilterInt32;
typedef MedianFilter<boost::uint32_t> MedianFilterUInt32;

//Обязательно надо проверить и запретить дальнейшую компиляцию если double и float
//означают не то что мы думаем. Типы из boost не используются т.к. простых дефайнов
//на встроенные типы с++ там нет и всё через библиотеки-прослойки для нецелочисленных
//вычислений, и с этим всем надо экспериментировать будет ещё.
//TODO: - как нибудь надо всё-таки перевести это дело на типы фиксированного размера
//если это не ударит по производительности.
BOOST_STATIC_ASSERT_MSG(sizeof(double) == 8, "double size is not 64 bit! You need to fix the code for you compillator!");
BOOST_STATIC_ASSERT_MSG(sizeof(float) == 4, "float size is not 32 bit! You need to fix the code for you compillator!");

//Универсальный класс-обёртка. В момент чтения файла определяет тип пикселов
//и исходя из этого создаёт pFilterObj нужного типа. Потом просто пересылает
//ему вызовы методов.
class MedianFilterUniversal : public MedianFilterBase
//TODO: добавить синхронизацию полей с реальным объектом при вызовах!!!
{
private:
	//Приватные поля
	bool imageIsLoaded;
	PixelType dataType;
	MedianFilterBase *pFilterObj;	//Сюда будет создаваться объект для нужного типа данных.

	//Приватные методы.

	//Обновить параметры вложенного объекта из параметров данного объекта.
	//Можно вызывать только если точно известно что this->pFilterObj существует.
	void UpdateSettings();
public:
	//Поля
		
	//Конструкторы-деструкторы
	MedianFilterUniversal();
	~MedianFilterUniversal();

	//Прочий функционал

	//Читает изображение в матрицу так чтобы по краям оставалось место для создания граничных
	//пикселей.
	bool LoadImage(const std::string &fileName, ErrorInfo *errObj = NULL, CallBackBase *callBackObj = NULL);
		
	//Сохраняет матрицу в изображение. За основу берётся ранее загруженная через LoadImage
	//картинка - файл копируется под новым именем и затем в него вносятся изменённые пиксели.
	///В первую очередь это нужно чтобы оставить метаданные в неизменном оригинальном виде.
	bool SaveImage(const std::string &fileName, ErrorInfo *errObj = NULL);

	//Заполняет граничные (пустые пиксели) области вокруг значимых пикселей в соответствии с
	//выбранным алгоритмом.
	void FillMargins(CallBackBase *callBackObj = NULL);
		
	//Обрабатывает матрицу sourceMatrix_ "тупым" фильтром. Результат записывает в destMatrix_.
	void ApplyStupidFilter(CallBackBase *callBackObj = NULL);

	//Обрабатывает матрицу sourceMatrix_ "никаким" фильтром. По сути просто копирование.
	//Для отладки. Результат записывает в destMatrix_.
	void ApplyStubFilter(CallBackBase *callBackObj = NULL);

	//"Тупая" визуализация матрицы, отправляется прямо в cout.
	void SourcePrintStupidVisToCout();
		
	//Вывод исходной матрицы в csv-файл, который должны понимать всякие картографические
	//программы. Это значит что каждый пиксел - это одна строка в файле.
	//Это "тупой" вариант вывода - метаданные нормально не сохраняются.
	bool SourceSaveToCSVFile(const std::string &fileName, ErrorInfo *errObj = NULL);
		
	//Аналогично SourceSaveToCSVFile, но для матрицы с результатом.
	bool DestSaveToCSVFile(const std::string &fileName, ErrorInfo *errObj = NULL);
};

}	//namespace geoimgconv