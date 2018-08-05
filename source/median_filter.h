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

#include <boost/static_assert.hpp>
#include "alt_matrix.h"
#include "common.h"
#include <string>
#include "base_filter.h"
#include <functional>
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

//TODO: модуль стал нереально монструозным. Как будет время - надо:
//1) Избавиться от всего кода, от которого можно здесь избавиться. Т.е. к примеру механику
//заполнения граничных пикселей вполне можно вынести в вообще отдельный файл и класс.
//2) Монструозные методы от которых избавиться нельзя - разбить на несколько более простых
//методов.
//3) Решить, применять ли костыли для раскидывания всей этой кучи классов по отдельным единицам
//компилляции. Костыли т.к. просто так, сходу - реализацию шаблонных классов в cpp-файлы не засунуть.
//4) А это наверное касается и всего проекта - надо просмотреть весь код и во-первых поправить явные
//косяки в стиле написания кода, во вторых - раз уж я всё-таки отвязался от 2008 вижлы - имеет
//смысл переделать те места, где оправдано использовать фичи того же C++11.

class RealMedianFilterBase;	//Предварительно объявление т.к. до основного на него будет ссылка.

//Обёртка с общей для всех типов пиксела функциональностью. Работать с фильтром надо именно через
//этот класс! Не через RealMedianFilter*-ы! Это базовая абстрактная обёртка, которая не имеет метода
//для собственно применения фильтра.
class MedianFilterBase : public BaseFilter
{
private:
	//Поля
	int aperture_ = DEFAULT_MEDFILTER_APERTURE;		//Окно фильтра (длина стороны квадрата в пикселах). Должно быть нечётным.
	double threshold_ = DEFAULT_MEDFILTER_THRESHOLD; //Порог фильтра. Если медиана отличается от значения пиксела меньше чем на порог - значение не будет изменено.
	//bool useMultiThreading_;		//Включает многопоточную обработку. Пока не реализовано.
	//unsigned int threadsNumber_;	//Количество потоков при многопоточной обработке. 0 - автоопределение. Пока не реализовано.
	MarginType marginType_ = DEFAULT_MEDFILTER_MARGIN_TYPE;		//Тип заполнения краевых пикселей.
	bool useMemChunks_ = false;		//Использовать ли режим обработки файла по кускам для экономии памяти.
	int blocksInMem_ = 0;	//Количество блоков, которое можно загружать в память. Граничные верхний и нижний блоки сюда тоже входят.
	std::string sourceFileName_ = "";	//Имя и путь файла с исходными данными.
	std::string destFileName_ = "";	//Имя и путь файла назначения.
	int imageSizeX_ = 0;	//Ширина картинки (0 если картинка не подсоединялась)
	int imageSizeY_ = 0;	//Высота картинки (0 если картинка не подсоединялась)
	bool imageIsLoaded_ = false;	//Загружена ли картинка целиком в матрицу
	bool sourceIsAttached_ = false;	//Настроен ли файл с источником.
	bool destIsAttached_ = false;	//Настроен ли файл с назначением
	PixelType dataType_ = PIXEL_UNKNOWN;	//Тип пикселя в картинке.
	size_t dataTypeSize_ = 0;	//Размер типа данных пикселя.
	RealMedianFilterBase *pFilterObj_ = nullptr;	//Сюда будет создаваться объект для нужного типа данных.
	unsigned long long minBlockSize_ = 0;	//Размер минимального блока, которыми обрабатывается файл.
	unsigned long long minBlockSizeHuang_ = 0;	//То же но для алгоритма Хуанга.
	unsigned long long minMemSize_ = 0;  //Минимальное количество памяти, без которого фильтр вообще не сможет обработать данное изображение.
	unsigned long long maxMemSize_ = 0;  //Максимальное количество памяти, которое может потребоваться для обработки изображения.
	GDALDataset *gdalSourceDataset_ = nullptr;	//GDAL-овский датасет с исходным файлом.
	GDALDataset *gdalDestDataset_ = nullptr;	//GDAL-овский датасет с файлом назначения.
	GDALRasterBand *gdalSourceRaster_ = nullptr;	//GDAL-овский объект для работы с пикселами исходного файла.
	GDALRasterBand *gdalDestRaster_ = nullptr;		//GDAL-овский объект для работы с пикселами файла назначения.
	int currPositionY_ = 0;	//Позиция "курсора" при чтении\записи очередных блоков из файла.
	bool useHuangAlgo_ = false;	//Если true то размеры блоков памяти будут считаться для алгоритма Хуанга.
	uint16_t huangLevelsNum_ = DEFAULT_HUANG_LEVELS_NUM;	//Количество уровней квантования для алгоритма Хуанга. Имеет значение для подсчёта размера требуемой памяти.
	bool fillPits_ = false;	//Заполнять ли "ямы" т.е. точки, которые ниже медианы.

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
	std::function<void()> onMinMaxDetectionStart;
	//Объект закончил вычислять минимальную и максимальную высоту на карте высот.
	std::function<void()> onMinMaxDetectionEnd;

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
	int const& getImageSizeX() const override { return imageSizeX_; }
	//void setImageSizeX(const int &imageSizeX) { imageSizeX_ = imageSizeX; }
	//imageSizeY
	int const& getImageSizeY() const override { return imageSizeY_; }
	//void setImageSizeY(const int &imageSizeY) { imageSizeY_ = imageSizeY; }
	//minBlockSize
	unsigned long long const& getMinBlockSize() const override { return minBlockSize_; }
	//void setMinBlockSize(const unsigned long long &value) { minBlockSize_ = value; }
	//minMemSize
	unsigned long long const& getMinMemSize() const override { return minMemSize_; }
	//void setMinMemSize(const unsigned long long &value) { minMemSize_ = value; }
	//maxMemSize
	unsigned long long const& getMaxMemSize() const override { return maxMemSize_; }
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
	const uint16_t& getHuangLevelsNum() const { return huangLevelsNum_; }
	//fillPits
	const bool& getFillPits() const { return fillPits_; }
	void setFillPits(const bool &value) { fillPits_ = value; }

	//Конструкторы-деструкторы
	MedianFilterBase(bool useHuangAlgo = false, uint16_t huangLevelsNum = DEFAULT_HUANG_LEVELS_NUM) :
		useHuangAlgo_(useHuangAlgo), huangLevelsNum_(huangLevelsNum) {}
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
	bool ApplyFilter(CallBackBase *callBackObj = NULL, ErrorInfo *errObj = NULL) override;
};

//Наследник, реализующий "тупой" фильтр. Алгоритм медианной фильтрации работает "в лоб".
//Результат записывается в выбранный destFile
class MedianFilterStupid : public MedianFilterBase
{
public:
	//Конструктор по умолчанию. Другие использовать нельзя.
	MedianFilterStupid() : MedianFilterBase() {}
	//Применить "тупой" медианный фильтр.
	bool ApplyFilter(CallBackBase *callBackObj = NULL, ErrorInfo *errObj = NULL) override;
};

//Наследник, "реализующий" медианную фильтрацию алгоритмом Хуанга.
//Результат записывается в выбранный destFile
class MedianFilterHuang : public MedianFilterBase
{
public:
	//Конструктор по умолчанию. Другие использовать нельзя.
	MedianFilterHuang(uint16_t levelsNum) : MedianFilterBase(true, levelsNum) {}
	//Обработать изображение медианным фильтром по алгоритму Хуанга
	bool ApplyFilter(CallBackBase *callBackObj = NULL, ErrorInfo *errObj = NULL) override;
};

//Базовый абстрактный класс для шаблонных классов.
class RealMedianFilterBase
{
private:
	//Нет смысла хранить тут поля типа aperture или threshold. Ибо этот класс нужен просто
	//чтобы вынести сюда код, который должен генерироваться по шаблону для разных типов пиксела.
	//Поэтому будем просто держать тут ссылку на основной объект класса и брать значения полей
	//оттуда.
	MedianFilterBase *ownerObj_;
public:
	//Доступ к ссылке на объект-хозяин
	MedianFilterBase& getOwnerObj() const { return *ownerObj_; }

	//Запретим конструктор по умолчанию, копирующий и переносящие конструкторы и аналогичные
	//им операторы присваивания.
	RealMedianFilterBase() = delete;
	RealMedianFilterBase(const RealMedianFilterBase&) = delete;
	RealMedianFilterBase(RealMedianFilterBase&&) = delete;
	RealMedianFilterBase& operator=(const RealMedianFilterBase&) = delete;
	RealMedianFilterBase& operator=(RealMedianFilterBase&&) = delete;
	//Cоздать объект можно только передав ссылку на MedianFilterBase
	RealMedianFilterBase(MedianFilterBase *ownerObj) : ownerObj_(ownerObj) {};
	virtual ~RealMedianFilterBase() {};

	//Абстрактные методы

	//Заполняет граничные (пустые пиксели) области вокруг значимых пикселей в соответствии с
	//выбранным алгоритмом.
	virtual void FillMargins(const int yStart, const int yToProcess, CallBackBase *callBackObj = NULL) = 0;

	//Обрабатывает выбранный исходный файл "тупым" фильтром. Результат записывается в выбранный destFile.
	virtual bool ApplyStupidFilter(CallBackBase *callBackObj = NULL, ErrorInfo *errObj = NULL) = 0;

	//Обрабатывает выбранный исходный файл алгоритмом Хуанга.
	virtual bool ApplyHuangFilter(CallBackBase *callBackObj = NULL, ErrorInfo *errObj = NULL) = 0;

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
	AltMatrix<CellType> sourceMatrix_;	//Инициализация в конструкторе.
	//Матрица для результата.
	AltMatrix<CellType> destMatrix_ = AltMatrix<CellType>(false,false);
	//Вычислялись ли уже минимальное и максимальное значения высот для картинки.
	bool minMaxCalculated_ = false;
	//Значение высоты пикселя, для которого пиксель считается незначимым.
	CellType noDataPixelValue_ = 0;
	//Минимальная высота, встречающаяся в изображении.
	CellType minPixelValue_ = 0;
	//Максимальная высота, встречающаяся в изображении.
	CellType maxPixelValue_ = 0;
	//Дельта - шаг между уровнями квантования.
	double levelsDelta_ = 0.0;

	//Приватные типы

	//Указатель на метод-заполнитель пикселей
	typedef void(RealMedianFilter<CellType>::*PixFillerMethod)(const int &x,
		const int &y, const PixelDirection direction, const int &marginSize,
		const char &signMatrixValue);

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
		const int &marginSize, const char &signMatrixValue = 2);

	//Заполнять пиксели зеркальным алгоритмом в указанном направлении
	void MirrorFiller(const int &x, const int &y, PixelDirection direction,
		const int &marginSize, const char &signMatrixValue = 2);

	//Костяк алгоритма, общий для Simple и Mirror
	void FillMargins_PixelBasedAlgo(const PixFillerMethod FillerMethod, const int yStart,
		const int yToProcess, CallBackBase *callBackObj = NULL);

	//Алгоритм заполнения граничных пикселей по незначимым пикселям, общий для Simple и Mirror
	void FillMargins_EmptyPixelBasedAlgo(const PixFillerMethod FillerMethod, const int yStart,
		const int yToProcess, CallBackBase *callBackObj = NULL);

	//Вспомогательный метод для прохода по пикселам строки вправо.
	inline void FillMargins_EmptyPixelBasedAlgo_ProcessToRight(int &y, int &x, int &windowY, int &windowX,
		int &windowYEnd, int &windowXEnd, int &actualPixelY, int &actualPixelX,
		PixelDirection &actualPixelDirection, int &actualPixelDistance, CellType &tempPixelValue,
		bool &windowWasEmpty, int marginSize, const PixFillerMethod FillerMethod);

	//Вспомогательный метод для прохода по пикселам строки влево.
	inline void FillMargins_EmptyPixelBasedAlgo_ProcessToLeft(int &y, int &x, int &windowY, int &windowX,
		int &windowYEnd, int &windowXEnd, int &actualPixelY, int &actualPixelX,
		PixelDirection &actualPixelDirection, int &actualPixelDistance, CellType &tempPixelValue,
		bool &windowWasEmpty, int marginSize, const PixFillerMethod FillerMethod);

	//Вспомогательный метод для обработки очередного незначимого пикселя при проходе
	//в указанном направлении (currentDirection).
	inline void FillMargins_EmptyPixelBasedAlgo_ProcessNextPixel(int &y, int &x, int &windowY,
		int &windowX, int& windowYEnd, int &windowXEnd, int &actualPixelY, int &actualPixelX,
		int &actualPixelDistance, PixelDirection &actualPixelDirection,
		const PixelDirection &currentDirection, bool &windowWasEmpty, CellType &tempPixelValue,
		const PixFillerMethod FillerMethod);

	//Вспомогательный метод для проверки наличия в окне значимых пикселей. Вернёт true если
	//значимых пикселей не нашлось. Если нашлось - то false и координаты со значением первого
	//найденного пикселя (через не-константные ссылочные параметры). Если windowWasEmpty
	//то воспользуется direction чтобы проверить только одну строчку или колонку, считая что
	//остальное уже было проверено при предыдущем вызове этого метода.
	inline bool FillMargins_WindowIsEmpty(const int &windowY, const int &windowX, const int &windowYEnd,
		const int &windowXEnd, int &pixelY, int &pixelX, CellType &pixelValue,
		const bool &windowWasEmpty, const PixelDirection &direction) const;

	//Вспомогательный метод для поиска ближайшего к указанному незначимому значимого пикселя. Сначала
	//поиск выполняется по горизонтали и вертикали, только затем по диагоналям. Если найти пиксел не
	//удалось - вернёт false. Если удалось вернёт true, координаты пикселя, направление и дистанцию
	//на него.
	inline bool FillMargins_FindNearestActualPixel(const int &startPixelY, const int &startPixelX,
		int &resultPixelY, int &resultPixelX, PixelDirection &resultDirection,
		int &resultDistance);

	//Заполнить пустые пиксели source-матрицы простым алгоритмом (сплошной цвет).
	void FillMargins_Simple(CallBackBase *callBackObj = NULL);

	//Заполнить пустые пиксели source-матрицы зеркальным алгоритмом.
	void FillMargins_Mirror(CallBackBase *callBackObj = NULL);

	//Применит указанный (ссылкой на метод) фильтр к изображению. Входящий и исходящий файлы
	//уже должны быть подключены. Вернёт false и инфу об ошибке если что-то пойдёт не так.
	bool ApplyFilter(FilterMethod CurrFilter, CallBackBase *callBackObj = NULL, ErrorInfo *errObj = NULL);

	//Вспомогательный предикат - вернёт true если пиксель в матрице назначения надо заменить медианой и false
	//если пиксель надо просто скопировать из исходной матрицы.
	inline bool UseMedian(const CellType &median, const CellType &pixelValue);

	//Метод "никакого" фильтра, который тупо копирует входящую матрицу в исходящую. Нужен для тестирования
	//и отладки. Первый аргумент указывает количество строк матрицы для реальной обработки.
	void StubFilter(const int &currYToProcess, CallBackBase *callBackObj = NULL);

	//Метод для обработки матрицы "тупым" фильтром, котороый действует практически в лоб.
	//Первый аргумент указывает количество строк матрицы для реальной обработки.
	void StupidFilter(const int &currYToProcess, CallBackBase *callBackObj = NULL);

	//Метод для обработки матрицы алгоритмом Хуанга. Теоретически на больших окнах это очень быстрый
	//алгоритм, единственный недостаток которого - потеря точности из за квантования.
	void HuangFilter(const int &currYToProcess, CallBackBase *callBackObj = NULL);

	//Вспомогательный метод для алгоритма Хуанга. Цикл по строке вправо.
	inline void HuangFilter_ProcessStringToRight(int &destX, int &destY, int &sourceX,
		int &sourceY, const int &marginSize, unsigned long &progressPosition, bool &gistIsActual,
		bool &gistIsEmpty, unsigned long *gist, uint16_t &median, unsigned long &elemsLeftMed,
		int &oldY, int &oldX, const uint16_t &halfMedPos, CallBackBase *callBackObj);

	//Вспомогательный метод для алгоритма Хуанга. Цикл по строке влево.
	//Два почти одинаковых метода здесь чтобы внутри не делать проверок направления
	//за счёт чего оно может быть будет работать немного быстрее.
	inline void HuangFilter_ProcessStringToLeft(int &destX, int &destY, int &sourceX,
		int &sourceY, const int &marginSize, unsigned long &progressPosition, bool &gistIsActual,
		bool &gistIsEmpty, unsigned long *gist, uint16_t &median, unsigned long &elemsLeftMed,
		int &oldY, int &oldX, const uint16_t &halfMedPos, CallBackBase *callBackObj);

	//Вспомогательный метод для алгоритма Хуанга. Заполняет гистограмму с нуля. В параметрах координаты
	//верхнего левого угла апертуры.
	inline void HuangFilter_FillGist(const int &leftUpY, const int &leftUpX, unsigned long *gist,
		uint16_t &median, unsigned long &elemsLeftMed,
		const uint16_t &halfMedPos);

	//Вспомогательный метод для алгоритма Хуанга. Выполняет шаг вправо.
	inline void HuangFilter_DoStepRight(const int &leftUpY, const int &leftUpX, unsigned long *gist,
		const uint16_t &median, unsigned long &elemsLeftMed);

	//Вспомогательный метод для алгоритма Хуанга. Выполняет шаг влево.
	inline void HuangFilter_DoStepLeft(const int &leftUpY, const int &leftUpX, unsigned long *gist,
		const uint16_t &median, unsigned long &elemsLeftMed);

	//Вспомогательный метод для алгоритма Хуанга. Выполняет шаг вниз.
	inline void HuangFilter_DoStepDown(const int &leftUpY, const int &leftUpX, unsigned long *gist,
		const uint16_t &median, unsigned long &elemsLeftMed);

	//Вспомогательный метод для алгоритма Хуанга. Корректирует медиану.
	inline void HuangFilter_DoMedianCorrection(uint16_t &median, unsigned long &elemsLeftMed,
		const uint16_t &halfMedPos, unsigned long *gist);

	//Вспомогательный метод для алгоритма Хуанга. Запись нового значения пикселя в матрицу
	//назначения.
	inline void HuangFilter_WriteDestPixel(const int &destY, const int &destX, const int &sourceY,
		const int &sourceX, const uint16_t &median);

	//Метод вычисляет минимальную и максимальную высоту в открытом изображении если это вообще нужно.
	//Также вычисляет дельту (шаг между уровнями).
	void CalcMinMaxPixelValues();

	//Преобразование значение пикселя в квантованное значение. minMaxCalculated_ не проверяется (чтобы
	//не тратить на это время, метод будет очень часто вызываться), однако оно должно быть true,
	//иначе получится ерунда.
	uint16_t PixelValueToQuantedValue(const CellType &value)
	{
		//Код в header-е чтобы инлайнился.
		if (value == noDataPixelValue_)
		{
			return 0;
		}
		else
		{
			uint16_t temp = (uint16_t)(((value-minPixelValue_)/levelsDelta_));
			return ((uint16_t)((value-minPixelValue_)/levelsDelta_));
		}
	}

public:
	//Запретим конструктор по умолчанию, копирующий и переносящие конструкторы и аналогичные
	//им операторы присваивания.
	RealMedianFilter() = delete;
	RealMedianFilter(const RealMedianFilter&) = delete;
	RealMedianFilter(RealMedianFilter&&) = delete;
	RealMedianFilter& operator=(const RealMedianFilter&) = delete;
	RealMedianFilter& operator=(RealMedianFilter&&) = delete;
	//Cоздать объект можно только передав ссылку на MedianFilterBase
	RealMedianFilter(MedianFilterBase *ownerObj) : RealMedianFilterBase(ownerObj),
		sourceMatrix_(true, ownerObj->getUseHuangAlgo()) {}
	//Виртуальный деструктор пущай его будет.

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
	void FillMargins(const int yStart, const int yToProcess, CallBackBase *callBackObj = NULL) override;

	//Заполняет матрицу квантованных пикселей в указанном промежутке, получая их из значений
	//оригинальных пикселей в исходной матрице. Нужно для алгоритма Хуанга.
	void FillQuantedMatrix(const int yStart, const int yToProcess);

	//Обрабатывает выбранный исходный файл "тупым" фильтром. Результат записывается в выбранный destFile.
	bool ApplyStupidFilter(CallBackBase *callBackObj = NULL, ErrorInfo *errObj = NULL) override;

	//Обрабатывает выбранный исходный файл алгоритмом Хуанга. Результат записывается в выбранный destFile.
	bool ApplyHuangFilter(CallBackBase *callBackObj = NULL, ErrorInfo *errObj = NULL) override;

	//Обрабатывает выбранный исходный файл "никаким" фильтром. По сути это просто копирование.
	//Для отладки. Результат записывается в выбранный destFile
	bool ApplyStubFilter(CallBackBase *callBackObj = NULL, ErrorInfo *errObj = NULL) override;

	//"Тупая" визуализация матрицы, отправляется прямо в cout.
	void SourcePrintStupidVisToCout() override;

	//Вывод исходной матрицы в csv-файл, который должны понимать всякие картографические
	//программы. Это значит что каждый пиксел - это одна строка в файле.
	//Это "тупой" вариант вывода - метаданные нормально не сохраняются.
	bool SourceSaveToCSVFile(const std::string &fileName, ErrorInfo *errObj = NULL) override;

	//Вывод исходной квантованной матрицы в csv-файл.
	bool QuantedSaveToCSVFile(const std::string &fileName, ErrorInfo *errObj = NULL);

	//Аналогично SourceSaveToCSVFile, но для матрицы с результатом.
	bool DestSaveToCSVFile(const std::string &fileName, ErrorInfo *errObj = NULL) override;

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
	inline CellType QuantedValueToPixelValue(const uint16_t &value)
	{
		return ((CellType)(std::lround((double)(value)* levelsDelta_)) + minPixelValue_);
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
template <> inline int8_t RealMedianFilter<int8_t>::
GetDelta(const int8_t &value1, const int8_t &value2)
{
	return std::abs(value1-value2);
}

//Вернёт положительную разницу между двумя значениями пикселя. Версия для boost::int16_t.
template <> inline int16_t RealMedianFilter<int16_t>::
GetDelta(const int16_t &value1, const int16_t &value2)
{
	return std::abs(value1-value2);
}

//Вернёт положительную разницу между двумя значениями пикселя. Версия для boost::int32_t.
template <> inline int32_t RealMedianFilter<int32_t>::
GetDelta(const int32_t &value1, const int32_t &value2)
{
	return std::abs(value1-value2);
}

//Преобразовать QuantedValue в double.
template <> inline double RealMedianFilter<double>::
 QuantedValueToPixelValue(const uint16_t &value)
{
	return ((double)value * levelsDelta_) + minPixelValue_;
}

//Преобразовать QuantedValue в float.
template <> inline float RealMedianFilter<float>::
QuantedValueToPixelValue(const uint16_t &value)
{
	return ((float)((double)value * levelsDelta_)) + minPixelValue_;
}

//Алиасы для классов, работающих с реально использующимися в GeoTIFF типами пикселов.
//Все они испольуются внутри median_filter.cpp а значит их код точно сгенерируется по шаблону.
typedef RealMedianFilter<double> RealMedianFilterFloat64;
typedef RealMedianFilter<float> RealMedianFilterFloat32;
typedef RealMedianFilter<int8_t> RealMedianFilterInt8;
typedef RealMedianFilter<uint8_t> RealMedianFilterUInt8;
typedef RealMedianFilter<int16_t> RealMedianFilterInt16;
typedef RealMedianFilter<uint16_t> RealMedianFilterUInt16;
typedef RealMedianFilter<int32_t> RealMedianFilterInt32;
typedef RealMedianFilter<uint32_t> RealMedianFilterUInt32;

//Обязательно надо проверить и запретить дальнейшую компиляцию если double и float
//означают не то что мы думаем.
static_assert(sizeof(double) == 8, "double size is not 64 bit! You need to fix the code for you compillator!");
static_assert(sizeof(float) == 4, "float size is not 32 bit! You need to fix the code for you compillator!");

}	//namespace geoimgconv