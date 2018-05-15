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

#include <boost/cstdint.hpp>
#include <boost/static_assert.hpp>
#include "alt_matrix.h"
#include "common.h"
#include <string>
#include "base_filter.h"
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <cmath>

namespace geoimgconv
{

//Семейство классов для работы с медианным фильтром. Тут 2 основных класса:

//RealMedianFilterBase с наследниками - класс, который выполняет работу над
//непосредственно изображением. Сюда вынесен код, специфичный для типа пиксела
//- именно для этого оно и вынесено в отдельный класс, раньше было всё в общей куче.
//Поскольку пикселы могут быть разными - базовый класс оборудован шаблонными
//наследниками и полиморфизмом.

//MedianFilterBase - изначально тут была вся эта система с шаблонными наследниками и
//полиморфизмом, но теперь это просто обёртка, которая однако занимается предварительным
//чтением картинки, определением её параметров и решает какой наследник RealMedianFilterBase
//заюзать для реальной работы над изображением.

class MedianFilterBase;	//Предварительно объявление т.к. в RealMedianFilterbase будет на него ссылка.

//Базовый абстрактный класс для шаблонных классов.
class RealMedianFilterBase
{
private:
	//Нет смысла хранить тут поля типа aperture или threshold. Ибо этот класс нужен просто
	//чтобы вынести сюда код, который должен генерироваться по шаблону для разных типов пиксела.
	//Поэтому будем просто держать тут ссылку на основной объект класса и брать значения полей
	//оттуда.
	MedianFilterBase *ownerObj_;

	//Запретим конструктор по умолчанию и копирующий конструктор.
	RealMedianFilterBase() {};
	RealMedianFilterBase(RealMedianFilterBase&) {};
public:
	//Доступ к ссылке на объект-хозяин
	MedianFilterBase& getOwnerObj() const { return *ownerObj_; }

	//Нельзя создать объект не дав ссылку на MedianFilterBase
	RealMedianFilterBase(MedianFilterBase *ownerObj) : ownerObj_(ownerObj) {};
	virtual ~RealMedianFilterBase() {};

	//Абстрактные методы

	//Заполняет граничные (пустые пиксели) области вокруг значимых пикселей в соответствии с
	//выбранным алгоритмом.
	virtual void FillMargins(const int yStart, const int yToProcess, CallBackBase *callBackObj = NULL) = 0;

	//Обрабатывает выбранный исходный файл "тупым" фильтром. Результат записывается в выбранный destFile.
	virtual bool ApplyStupidFilter(CallBackBase *callBackObj = NULL, ErrorInfo *errObj = NULL) = 0;

	//Обрабатывает выбранный исходный файл "никаким" фильтром. По сути это просто копирование.
	//Для отладки. Результат записывается в выбранный destFile
	virtual bool ApplyStubFilter(CallBackBase *callBackObj = NULL, ErrorInfo *errObj = NULL) = 0;

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
template <typename CellType> class RealMedianFilter : public RealMedianFilterBase
{
private:
	//Приватные поля.

	//Матрица для исходного изображения.
	AltMatrix<CellType> sourceMatrix_;
	//Матрица для результата.
	AltMatrix<CellType> destMatrix_;
	//Вычислялись ли уже минимальное и максимальное значения высот для картинки.
	bool minMaxCalculated_;
	//Значение высоты пикселя, для которого пиксель считается незначимым.
	CellType noDataPixelValue_;
	//Минимальная высота, встречающаяся в изображении.
	CellType minPixelValue_;
	//Максимальная высота, встречающаяся в изображении.
	CellType maxPixelValue_;
	//Дельта - шаг между уровнями квантования.
	double levelsDelta_;

	//Приватные типы

	//Указатель на метод-заполнитель пикселей
	typedef void(RealMedianFilter<CellType>::*PixFillerMethod)(const int &x,
		const int &y, const PixelDirection direction, const int &marginSize);

	//Указатель на метод-фильтр
	typedef void(RealMedianFilter<CellType>::*FilterMethod)(const int &currYToProcess,
		CallBackBase *callBackObj);

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
	void FillMargins_PixelBasedAlgo(const PixFillerMethod FillerMethod, const int yStart,
		const int yToProcess, CallBackBase *callBackObj = NULL);

	//Заполнить пустые пиксели source-матрицы простым алгоритмом (сплошной цвет).
	void FillMargins_Simple(CallBackBase *callBackObj = NULL);

	//Заполнить пустые пиксели source-матрицы зеркальным алгоритмом.
	void FillMargins_Mirror(CallBackBase *callBackObj = NULL);

	//Применит указанный (ссылкой на метод) фильтр к изображению. Входящий и исходящий файлы
	//уже должны быть подключены. Вернёт false и инфу об ошибке если что-то пойдёт не так.
	bool ApplyFilter(FilterMethod CurrFilter, CallBackBase *callBackObj = NULL, ErrorInfo *errObj = NULL);

	//Метод "никакого" фильтра, который тупо копирует входящую матрицу в исходящую. Нужен для тестирования
	//и отладки. Первый аргумент указывает количество строк матрицы для реальной обработки.
	void StubFilter(const int &currYToProcess, CallBackBase *callBackObj = NULL);

	//Метод для обработки матрицы "тупым" фильтром, котороый действует практически в лоб.
	//Первый аргумент указывает количество строк матрицы для реальной обработки.
	void StupidFilter(const int &currYToProcess, CallBackBase *callBackObj = NULL);

	//Метод вычисляет минимальную и максимальную высоту в открытом изображении если это вообще нужно.
	//Также вычисляет дельту (шаг между уровнями).
	void CalcMinMaxPixelValues();

	//Преобразование значение пикселя в квантованное значение. minMaxCalculated_ не проверяется (чтобы
	//не тратить на это время, метод будет очень часто вызываться), однако оно должно быть true,
	//иначе получится ерунда.
	boost::uint16_t PixelValueToQuantedValue(const CellType &value)
	{
		//Код в header-е чтобы инлайнился.
		return ((boost::uint16_t)(value/levelsDelta_));
	}

public:
	//Нельзя создать объект не дав ссылку на MedianFilterBase
	RealMedianFilter(MedianFilterBase *ownerObj);

	//Доступ к полям.

	//minMaxCalculated
	const bool& getMinMaxCalculated() const { return minMaxCalculated_; }
	//noDataPixelValue
	const CellType& getNoDataPixelValue() const { return noDataPixelValue_; }
	void setNoDataPixelValue(CellType &value)
	{
		//После смены значения, по которому определяются незначимые пиксели - минимальную
		//и максимальную высоту придётся пересчитывать.
		minMaxCalculated_ = false;
		noDataPixelValue_ = value;
	}
	//minPixelValue
	const CellType& getMinPixelValue() { return minPixelValue_; }
	//maxPixelValue
	const CellType& getMaxPixelValue() { return maxPixelValue_; }
	//levelsDelta
	const double& getLevelsDelta() { return levelsDelta_; }

	//Прочий функционал

	//Заполняет граничные (пустые пиксели) области вокруг значимых пикселей в соответствии с
	//выбранным алгоритмом.
	void FillMargins(const int yStart, const int yToProcess, CallBackBase *callBackObj = NULL);

	//Заполняет матрицу квантованных пикселей в указанном промежутке, получая их из значений
	//оригинальных пикселей в исходной матрице. Нужно для алгоритма Хуанга.
	void FillQuantedMatrix(const int yStart, const int yToProcess);

	//Обрабатывает выбранный исходный файл "тупым" фильтром. Результат записывается в выбранный destFile.
	bool ApplyStupidFilter(CallBackBase *callBackObj = NULL, ErrorInfo *errObj = NULL);

	//Обрабатывает выбранный исходный файл "никаким" фильтром. По сути это просто копирование.
	//Для отладки. Результат записывается в выбранный destFile
	bool ApplyStubFilter(CallBackBase *callBackObj = NULL, ErrorInfo *errObj = NULL);

	//"Тупая" визуализация матрицы, отправляется прямо в cout.
	void SourcePrintStupidVisToCout();

	//Вывод исходной матрицы в csv-файл, который должны понимать всякие картографические
	//программы. Это значит что каждый пиксел - это одна строка в файле.
	//Это "тупой" вариант вывода - метаданные нормально не сохраняются.
	bool SourceSaveToCSVFile(const std::string &fileName, ErrorInfo *errObj = NULL);

	//Аналогично SourceSaveToCSVFile, но для матрицы с результатом.
	bool DestSaveToCSVFile(const std::string &fileName, ErrorInfo *errObj = NULL);

	//Вернёт положительную разницу между двумя значениями пикселя независимо от
	//типа данных в ячейках. Версия для неизвестного типа.
	inline CellType GetDelta(const CellType &value1, const CellType &value2)
	{
		if (value1 >= value2) return value1-value2;
		else return value2-value1;
	};

	//Преобразовать QuantedValue в CellType. Эта версия предполагает что CellType
	//- целочисленный тип и результат требует округления. Эта версия используется
	//для всех CellType кроме double и float
	inline CellType QuantedValueToPixelValue(const boost::uint16_t &value)
	{
		return (CellType)(std::lround((double)(value)* levelsDelta_));
	}
};

//Специализация методов RealMedianFilter (для тех типов где специализации могут
//работать быстрее).

//Вернёт положительную разницу между двумя значениями пикселя. Версия для double.
template <> inline double RealMedianFilter<double>::
	GetDelta(const double &value1, const double &value2)
{
	return std::abs(value1-value2);
}

//Вернёт положительную разницу между двумя значениями пикселя. Версия для float.
template <> inline float RealMedianFilter<float>::
GetDelta(const float &value1, const float &value2)
{
	return std::abs(value1-value2);
}

//Вернёт положительную разницу между двумя значениями пикселя. Версия для boost::int8_t.
template <> inline boost::int8_t RealMedianFilter<boost::int8_t>::
GetDelta(const boost::int8_t &value1, const boost::int8_t &value2)
{
	return std::abs(value1-value2);
}

//Вернёт положительную разницу между двумя значениями пикселя. Версия для boost::int16_t.
template <> inline boost::int16_t RealMedianFilter<boost::int16_t>::
GetDelta(const boost::int16_t &value1, const boost::int16_t &value2)
{
	return std::abs(value1-value2);
}

//Вернёт положительную разницу между двумя значениями пикселя. Версия для boost::int32_t.
template <> inline boost::int32_t RealMedianFilter<boost::int32_t>::
GetDelta(const boost::int32_t &value1, const boost::int32_t &value2)
{
	return std::abs(value1-value2);
}

//Преобразовать QuantedValue в double.
template <> inline double RealMedianFilter<double>::
 QuantedValueToPixelValue(const boost::uint16_t &value)
{
	return (double)value * levelsDelta_;
}

//Преобразовать QuantedValue в float.
template <> inline float RealMedianFilter<float>::
QuantedValueToPixelValue(const boost::uint16_t &value)
{
	return (float)((double)value * levelsDelta_);
}

//Алиасы для классов, работающих с реально использующимися в GeoTIFF типами пикселов.
//Все они испольуются внутри median_filter.cpp а значит их код точно сгенерируется по шаблону.
typedef RealMedianFilter<double> RealMedianFilterFloat64;
typedef RealMedianFilter<float> RealMedianFilterFloat32;
typedef RealMedianFilter<boost::int8_t> RealMedianFilterInt8;
typedef RealMedianFilter<boost::uint8_t> RealMedianFilterUInt8;
typedef RealMedianFilter<boost::int16_t> RealMedianFilterInt16;
typedef RealMedianFilter<boost::uint16_t> RealMedianFilterUInt16;
typedef RealMedianFilter<boost::int32_t> RealMedianFilterInt32;
typedef RealMedianFilter<boost::uint32_t> RealMedianFilterUInt32;

//Обязательно надо проверить и запретить дальнейшую компиляцию если double и float
//означают не то что мы думаем. Типы из boost не используются т.к. простых дефайнов
//на встроенные типы с++ там нет и всё через библиотеки-прослойки для нецелочисленных
//вычислений, и с этим всем надо экспериментировать будет ещё.
//TODO: - как нибудь надо всё-таки перевести это дело на типы фиксированного размера
//если это не ударит по производительности.
BOOST_STATIC_ASSERT_MSG(sizeof(double) == 8, "double size is not 64 bit! You need to fix the code for you compillator!");
BOOST_STATIC_ASSERT_MSG(sizeof(float) == 4, "float size is not 32 bit! You need to fix the code for you compillator!");

//Обёртка с общей для всех типов пиксела функциональностью. Работать с фильтром надо именно через
//этот класс! Не через RealMedianFilter*-ы! Это базовая абстрактная обёртка, которая не имеет метода
//для собственно применения фильтра.
class MedianFilterBase : public BaseFilter
{
private:
	//Поля
	int aperture_;		//Окно фильтра (длина стороны квадрата в пикселах). Должно быть нечётным.
	double threshold_;			//Порог фильтра. Если медиана отличается от значения пиксела меньше чем на порог - значение не будет изменено.
	//bool useMultiThreading_;		//Включает многопоточную обработку. Пока не реализовано.
	//unsigned int threadsNumber_;	//Количество потоков при многопоточной обработке. 0 - автоопределение. Пока не реализовано.
	MarginType marginType_;		//Тип заполнения краевых пикселей.
	bool useMemChunks_;		//Использовать ли режим обработки файла по кускам для экономии памяти.
	int blocksInMem_;	//Количество блоков, которое можно загружать в память. Граничные верхний и нижний блоки сюда тоже входят.
	std::string sourceFileName_;	//Имя и путь файла с исходными данными.
	std::string destFileName_;	//Имя и путь файла назначения.
	int imageSizeX_;	//Ширина картинки (0 если картинка не подсоединялась)
	int imageSizeY_;	//Высота картинки (0 если картинка не подсоединялась)
	bool imageIsLoaded_;	//Загружена ли картинка целиком в матрицу
	bool sourceIsAttached_;	//Настроен ли файл с источником.
	bool destIsAttached_;	//Настроен ли файл с назначением
	PixelType dataType_;	//Тип пикселя в картинке.
	size_t dataTypeSize_;	//Размер типа данных пикселя.
	RealMedianFilterBase *pFilterObj_;	//Сюда будет создаваться объект для нужного типа данных.
	unsigned long long minBlockSize_;	//Размер минимального блока, которыми обрабатывается файл.
	unsigned long long minBlockSizeHuang_;	//То же но для алгоритма Хуанга.
	unsigned long long minMemSize_;  //Минимальное количество памяти, без которого фильтр вообще не сможет обработать данное изображение.
	unsigned long long maxMemSize_;  //Максимальное количество памяти, которое может потребоваться для обработки изображения.
	GDALDataset *gdalSourceDataset_;	//GDAL-овский датасет с исходным файлом.
	GDALDataset *gdalDestDataset_;	//GDAL-овский датасет с файлом назначения.
	GDALRasterBand *gdalSourceRaster_;	//GDAL-овский объект для работы с пикселами исходного файла.
	GDALRasterBand *gdalDestRaster_;		//GDAL-овский объект для работы с пикселами файла назначения.
	int currPositionY_;	//Позиция "курсора" при чтении\записи очередных блоков из файла.
	bool useHuangAlgo_;	//Если true то размеры блоков памяти будут считаться для алгоритма Хуанга.
	boost::uint16_t huangLevelsNum_;	//Количество уровней квантования для алгоритма Хуанга. Имеет значение для подсчёта размера требуемой памяти.

	//Приватные методы

	//Вычислить минимальные размеры блоков памяти, которые нужны для принятия решения о
	//том сколько памяти разрешено использовать медианному фильтру в процессе своей работы.
	void CalcMemSizes();
protected:
	//sourceIsAttached
	bool const& getSourceIsAttached() const { return sourceIsAttached_; }
	void setSourceIsAttached(const bool &value) { sourceIsAttached_ = value; }
	//destIsAttached
	bool const& getDestIsAttached() const { return destIsAttached_; }
	void setDestIsAttached(const bool &value) { destIsAttached_ = value; }
	//pFilterObj
	RealMedianFilterBase& getFilterObj() const { return *pFilterObj_; }
public:
	//"События", к которым можно привязать обработчики.

	//Объект начал вычислять минимальную и максимальную высоту на карте высот.
	boost::function<void()> onMinMaxDetectionStart;
	//Объект закончил вычислять минимальную и максимальную высоту на карте высот.
	boost::function<void()> onMinMaxDetectionEnd;

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
	//useMemChunks
	bool const& getUseMemChunks() const { return useMemChunks_; }
	void setUseMemChunks(const bool &useMemChunks) { useMemChunks_ = useMemChunks; }
	//blocksInMem
	int const& getBlocksInMem() const { return blocksInMem_; }
	void setBlocksInMem(const int &blocksInMem) { blocksInMem_ = blocksInMem; }
	//sourceFileName
	std::string const& getSourceFileName() const { return sourceFileName_; }
	//TODO setSourceFileName нужен только временно! Убрать после того как уберутся всякие LoadFile!
	void setSourceFileName(const std::string &sourceFileName) { sourceFileName_ = sourceFileName; }
	//destFileName
	std::string const& getDestFileName() const { return destFileName_; }
	//imageSizeX
	int const& getImageSizeX() const { return imageSizeX_; }
	//void setImageSizeX(const int &imageSizeX) { imageSizeX_ = imageSizeX; }
	//imageSizeY
	int const& getImageSizeY() const { return imageSizeY_; }
	//void setImageSizeY(const int &imageSizeY) { imageSizeY_ = imageSizeY; }
	//minBlockSize
	unsigned long long const& getMinBlockSize() const { return minBlockSize_; }
	//void setMinBlockSize(const unsigned long long &value) { minBlockSize_ = value; }
	//minMemSize
	unsigned long long const& getMinMemSize() const { return minMemSize_; }
	//void setMinMemSize(const unsigned long long &value) { minMemSize_ = value; }
	//maxMemSize
	unsigned long long const& getMaxMemSize() const { return maxMemSize_; }
	//gdalSourceRaster_
	GDALRasterBand* getGdalSourceRaster() { return gdalSourceRaster_; }
	//gdalDestRaster_
	GDALRasterBand* getGdalDestRaster() { return gdalDestRaster_; }
	//currPositionY
	int const& getCurrPositionY() const { return currPositionY_; }
	void setCurrPositionY(const int &value) { currPositionY_ = value; }
	//useHuangAlgo
	bool const& getUseHuangAlgo() const { return useHuangAlgo_; }
	//huangLevelsNum
	const boost::uint16_t& getHuangLevelsNum() const { return huangLevelsNum_; }

	//Конструкторы-деструкторы
	MedianFilterBase(bool useHuangAlgo = false, boost::uint16_t huangLevelsNum = DEFAULT_HUANG_LEVELS_NUM);
	~MedianFilterBase();

	//Прочий функционал

	//Выбрать исходный файл для дальнейшего чтения и обработки. Получает информацию о параметрах изображения,
	//запоминает её в полях объекта.
	bool OpenInputFile(const std::string &fileName, ErrorInfo *errObj = NULL);

	//Подготовить целевой файл к записи в него результата. Если forceRewrite==true - перезапишет уже
	//существующий файл. Иначе вернёт ошибку (false и инфу в errObj). Input-файл уже должен быть открыт.
	bool OpenOutputFile(const std::string &fileName, const bool &forceRewrite, ErrorInfo *errObj = NULL);

	//Закрыть исходный файл.
	void CloseInputFile();

	//Закрыть файл назначения.
	void CloseOutputFile();

	//Закрыть все файлы.
	void CloseAllFiles();

	//Приводит апертуру к имеющему смысл значению.
	void FixAperture();

	//"Тупая" визуализация матрицы, отправляется прямо в cout.
	void SourcePrintStupidVisToCout();

	//Вывод исходной матрицы в csv-файл, который должны понимать всякие картографические
	//программы. Это значит что каждый пиксел - это одна строка в файле.
	//Это "тупой" вариант вывода - метаданные нормально не сохраняются.
	bool SourceSaveToCSVFile(const std::string &fileName, ErrorInfo *errObj = NULL);

	//Аналогично SourceSaveToCSVFile, но для матрицы с результатом.
	bool DestSaveToCSVFile(const std::string &fileName, ErrorInfo *errObj = NULL);

};

//Наследник, реализующий "никакой" фильтр. По сути это просто копирование. Для отладки.
//Результат записывается в выбранный destFile
class MedianFilterStub : public MedianFilterBase
{
public:
	//Конструктор по умолчанию. Другие использовать нельзя.
	MedianFilterStub() : MedianFilterBase() {}
	//Применить "никакой" медианный фильтр.
	bool ApplyFilter(CallBackBase *callBackObj = NULL, ErrorInfo *errObj = NULL);
};

//Наследник, реализующий "тупой" фильтр. Алгоритм медианной фильтрации работает "в лоб".
//Результат записывается в выбранный destFile
class MedianFilterStupid : public MedianFilterBase
{
public:
	//Конструктор по умолчанию. Другие использовать нельзя.
	MedianFilterStupid() : MedianFilterBase() {}
	//Применить "тупой" медианный фильтр.
	bool ApplyFilter(CallBackBase *callBackObj = NULL, ErrorInfo *errObj = NULL);
};

//Наследник, "реализующий" медианную фильтрацию алгоритмом Хуанга.
//Результат записывается в выбранный destFile
class MedianFilterHuang : public MedianFilterBase
{
public:
	//Конструктор по умолчанию. Другие использовать нельзя.
	MedianFilterHuang(MedfilterAlgo algo, boost::uint16_t levelsNum) : MedianFilterBase(true, levelsNum) {}
	//Обработать изображение медианным фильтром по алгоритму Хуанга
	bool ApplyFilter(CallBackBase *callBackObj = NULL, ErrorInfo *errObj = NULL);
};

}	//namespace geoimgconv