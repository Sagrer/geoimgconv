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

//Обёртка с общей для всех типов пиксела функциональностью. Работать с фильтром надо именно через
//этот класс! Не через RealMedianFilter*-ы! Это базовая абстрактная обёртка, которая не имеет метода
//для собственно применения фильтра.

#include "common.h"
#include <string>
#include "base_filter.h"
#include <functional>
#include <memory>

namespace geoimgconv
{

class RealMedianFilterBase;

class MedianFilterBase : public BaseFilter
{
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

protected:
	//sourceIsAttached
	bool const& getSourceIsAttached() const { return sourceIsAttached_; }
	void setSourceIsAttached(const bool &value) { sourceIsAttached_ = value; }
	//destIsAttached
	bool const& getDestIsAttached() const { return destIsAttached_; }
	void setDestIsAttached(const bool &value) { destIsAttached_ = value; }
	//pFilterObj
	RealMedianFilterBase& getFilterObj() const { return *pFilterObj_; }

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
	PixelType dataType_ = PixelType::Unknown;	//Тип пикселя в картинке.
	size_t dataTypeSize_ = 0;	//Размер типа данных пикселя.
	std::unique_ptr<RealMedianFilterBase> pFilterObj_ = nullptr;	//Сюда будет создаваться объект для нужного типа данных.
	unsigned long long minBlockSize_ = 0;	//Размер минимального блока, которыми обрабатывается файл.
	unsigned long long minBlockSizeHuang_ = 0;	//То же но для алгоритма Хуанга.
	unsigned long long minMemSize_ = 0;  //Минимальное количество памяти, без которого фильтр вообще не сможет обработать данное изображение.
	unsigned long long maxMemSize_ = 0;  //Максимальное количество памяти, которое может потребоваться для обработки изображения.
	//Голые указатели на объекты GDAL. Ибо удаляются они тоже через API GDAL а не обычным delete.
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
};

}	//namespace geoimgconv

#include "real_median_filter_base.h"