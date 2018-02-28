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

namespace geoimgconv
{

//Семейство классов для работы с медианным фильтром. Тут 2 основных класса:

//RealMedianFilterBase с наследниками - класс, который выполняет работу над
//непосредственно изображением. Сюда вынесен код, специфичный для типа пиксела
//- именно для этого оно и вынесено в отдельный класс, раньше было всё в общей куче.
//Поскольку пикселы могут быть разными - базовый класс оборудован шаблонными
//наследниками и полиморфизмом.

//MedianFilter - изначально тут была вся эта система с шаблонными наследниками и
//полиморфизмом, но теперь это просто обёртка, которая однако занимается предварительным
//чтением картинки, определением её параметров и решает какой наследник RealMedianFilterBase
//заюзать для реальной работы над изображением.

class MedianFilter;	//Предварительно объявление т.к. в RealMedianFilterbase будет на него ссылка.

//Базовый абстрактный класс для шаблонных классов.
class RealMedianFilterBase
{
private:
	//Нет смысла хранить тут поля типа aperture или threshold. Ибо этот класс нужен просто
	//чтобы вынести сюда код, который должен генерироваться по шаблону для разных типов пиксела.
	//Поэтому будем просто держать тут ссылку на основной объект класса и брать значения полей
	//оттуда.
	MedianFilter *ownerObj_;

	//Запретим конструктор по умолчанию и копирующий конструктор.
	RealMedianFilterBase() {};
	RealMedianFilterBase(RealMedianFilterBase&) {};
public:
	//Доступ к ссылке на объект-хозяин
	MedianFilter& getOwnerObj() const { return *ownerObj_; }

	//Нельзя создать объект не дав ссылку на MedianFilter
	RealMedianFilterBase(MedianFilter *ownerObj) : ownerObj_(ownerObj) {};
	virtual ~RealMedianFilterBase() {};

	//Абстрактные методы

	//Читает изображение в матрицу так чтобы по краям оставалось место для создания граничных
	//пикселей.
	virtual bool LoadImage(const std::string &fileName, ErrorInfo *errObj = NULL, CallBackBase *callBackObj = NULL) = 0;

	//Сохраняет матрицу в изображение. За основу берётся ранее загруженная через LoadImage
	//картинка - файл копируется под новым именем и затем в него вносятся изменённые пиксели.
	///В первую очередь это нужно чтобы оставить метаданные в неизменном оригинальном виде.
	virtual bool SaveImage(const std::string &fileName, ErrorInfo *errObj = NULL) = 0;

	//Заполняет граничные (пустые пиксели) области вокруг значимых пикселей в соответствии с
	//выбранным алгоритмом.
	virtual void FillMargins(CallBackBase *callBackObj = NULL) = 0;

	//Обрабатывает матрицу sourceMatrix_ "тупым" фильтром. Результат записывает в destMatrix_.
	virtual void ApplyStupidFilter_old(CallBackBase *callBackObj = NULL) = 0;

	//Обрабатывает матрицу sourceMatrix_ "никаким" фильтром. По сути просто копирование.
	//Для отладки. Результат записывает в destMatrix_.
	virtual void ApplyStubFilter_old(CallBackBase *callBackObj = NULL) = 0;

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
template <typename CellType> class RealMedianFilterTemplBase : public RealMedianFilterBase
{
private:
	//Приватные поля.
	//std::string sourceFileName_;
	AltMatrix<CellType> sourceMatrix_;	//Тут хранится самое ценное! То с чем работаем :).
	AltMatrix<CellType> destMatrix_;		//А тут ещё более ценное - результат ).
		
	//Приватные типы
		
	//Указатель на метод-заполнитель пикселей
	typedef void(RealMedianFilterTemplBase<CellType>::*PixFillerMethod)(const int &x,
		const int &y, const PixelDirection direction, const int &marginSize);

	//Указатель на метод-фильтр
	typedef void(RealMedianFilterTemplBase<CellType>::*FilterMethod)(CallBackBase *callBackObj);
		
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
	void FillMargins_PixelBasedAlgo(const PixFillerMethod FillerMethod,
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

	//Применит указанный (ссылкой на метод) фильтр к изображению. Входящий и исходящий файлы
	//уже должны быть подключены. Вернёт false и инфу об ошибке если что-то пойдёт не так.
	bool ApplyFilter(FilterMethod CurrFilter, CallBackBase *callBackObj = NULL, ErrorInfo *errObj = NULL);

	//Метод "тупого" фильтра, который тупо копирует входящую матрицу в исходящую. Нужен для тестирования
	//и отладки.
	void StubFilter(CallBackBase *callBackObj = NULL);

	//Метод для обработки матрицы "тупым" фильтром, котороый действует практически в лоб.
	void StupiudFilter(CallBackBase *callBackObj = NULL);
public:
	//Нельзя создать объект не дав ссылку на MedianFilter
	RealMedianFilterTemplBase(MedianFilter *ownerObj) : RealMedianFilterBase(ownerObj) {};
	//~RealMedianFilterTemplBase() {}; //Пустой. Хватит по умолчанию.

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
	void ApplyStupidFilter_old(CallBackBase *callBackObj = NULL);

	//Обрабатывает матрицу sourceMatrix_ "никаким" фильтром. По сути просто копирование.
	//Для отладки. Результат записывает в destMatrix_.
	void ApplyStubFilter_old(CallBackBase *callBackObj = NULL);

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
};

//Шаблонный класс для произвольного типа ячейки. Пустой наследник (не специализированная
//версия. Через этот класс работают все unsigned-версии CellType.
template <typename CellType> class RealMedianFilter : public RealMedianFilterTemplBase<CellType>
{
public:
	//Нельзя создать объект не дав ссылку на MedianFilter
	RealMedianFilter(MedianFilter *ownerObj) : RealMedianFilterTemplBase<CellType>(ownerObj) {};
};

//Специализация RealMedianFilter для double
template <> class RealMedianFilter<double> : public RealMedianFilterTemplBase<double>
{
private:
	//Вернёт положительную разницу между двумя значениями пикселя независимо от
	//типа данных в ячейках.
	inline double GetDelta(const double &value1, const double &value2)
	{
		return std::abs(value1-value2);
	};
public:
	//Нельзя создать объект не дав ссылку на MedianFilter
	RealMedianFilter(MedianFilter *ownerObj) : RealMedianFilterTemplBase<double>(ownerObj) {};
};

//Специализация RealMedianFilter для float
template <> class RealMedianFilter<float> : public RealMedianFilterTemplBase<float>
{
private:
	//Вернёт положительную разницу между двумя значениями пикселя независимо от
	//типа данных в ячейках.
	inline float GetDelta(const float &value1, const float &value2)
	{
		return std::abs(value1-value2);
	};
public:
	//Нельзя создать объект не дав ссылку на MedianFilter
	RealMedianFilter(MedianFilter *ownerObj) : RealMedianFilterTemplBase<float>(ownerObj) {};
};

//Специализация RealMedianFilter для boost::int8_t
template <> class RealMedianFilter<boost::int8_t> : public RealMedianFilterTemplBase<boost::int8_t>
{
private:
	//Вернёт положительную разницу между двумя значениями пикселя независимо от
	//типа данных в ячейках.
	inline boost::int8_t GetDelta(const boost::int8_t &value1, const boost::int8_t &value2)
	{
		return std::abs(value1-value2);
	};
public:
	//Нельзя создать объект не дав ссылку на MedianFilter
	RealMedianFilter(MedianFilter *ownerObj) : RealMedianFilterTemplBase<boost::int8_t>(ownerObj) {};
};

//Специализация RealMedianFilter для boost::int16_t
template <> class RealMedianFilter<boost::int16_t> : public RealMedianFilterTemplBase<boost::int16_t>
{
private:
	//Вернёт положительную разницу между двумя значениями пикселя независимо от
	//типа данных в ячейках.
	inline boost::int16_t GetDelta(const boost::int16_t &value1, const boost::int16_t &value2)
	{
		return std::abs(value1-value2);
	};
public:
	//Нельзя создать объект не дав ссылку на MedianFilter
	RealMedianFilter(MedianFilter *ownerObj) : RealMedianFilterTemplBase<boost::int16_t>(ownerObj) {};
};

//Специализация RealMedianFilter для boost::int32_t
template <> class RealMedianFilter<boost::int32_t> : public RealMedianFilterTemplBase<boost::int32_t>
{
private:
	//Вернёт положительную разницу между двумя значениями пикселя независимо от
	//типа данных в ячейках.
	inline boost::int32_t GetDelta(const boost::int32_t &value1, const boost::int32_t &value2)
	{
		return std::abs(value1-value2);
	};
public:
	//Нельзя создать объект не дав ссылку на MedianFilter
	RealMedianFilter(MedianFilter *ownerObj) : RealMedianFilterTemplBase<boost::int32_t>(ownerObj) {};
};

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
//этот класс! Не через RealMedianFilter*-ы!
class MedianFilter : public BaseFilter
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
	unsigned long long minMemSize_;  //Минимальное количество памяти, без которого фильтр вообще не сможет обработать данное изображение.
	unsigned long long maxMemSize_;  //Максимальное количество памяти, которое может потребоваться для обработки изображения.
	GDALDataset *gdalSourceDataset_;	//GDAL-овский датасет с исходным файлом.
	GDALDataset *gdalDestDataset_;	//GDAL-овский датасет с файлом назначения.
	GDALRasterBand *gdalSourceRaster_;	//GDAL-овский объект для работы с пикселами исходного файла.
	GDALRasterBand *gdalDestRaster_;		//GDAL-овский объект для работы с пикселами файла назначения.
	int currPositionY_;	//Позиция "курсора" при чтении\записи очередных блоков из файла.

	//Приватные методы

	//Вычислить минимальные размеры блоков памяти, которые нужны для принятия решения о
	//том сколько памяти разрешено использовать медианному фильтру в процессе своей работы.
	void CalcMemSizes();
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

	//Конструкторы-деструкторы
	MedianFilter();
	~MedianFilter();

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
	void ApplyStupidFilter_old(CallBackBase *callBackObj = NULL);

	//Обрабатывает матрицу sourceMatrix_ "никаким" фильтром. По сути просто копирование.
	//Для отладки. Результат записывает в destMatrix_.
	void ApplyStubFilter_old(CallBackBase *callBackObj = NULL);

	//Обрабатывает выбранный исходный файл "никаким" фильтром. По сути это просто копирование.
	//Для отладки. Результат записывается в выбранный destFile
	bool ApplyStubFilter(CallBackBase *callBackObj = NULL, ErrorInfo *errObj = NULL);

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

}	//namespace geoimgconv