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

//Шаблонный класс для произвольного типа ячейки. На базе этой версии создаются
//специализации наследников

//TODO: модуль по-прежнему монструозен. В идеале нужно:
//1) Абстрагировать код, читающий информацию из картинок, заполняющий граничные области
//от кода, выполняющего фильтрацию. Скорее всего через наследование.
//2) Механику заполнения границ возможно тоже стоит вытащить во что-то отдельное.
//3) Есть огромные монструозные методы - их надо разбить.
//4) Там где возможно - стоит применить фичи с++11, как минимум умные указатели.

#include <string>
#include "call_back_base.h"
#include "errors.h"
#include "real_median_filter_base.h"
#include "alt_matrix.h"

namespace geoimgconv
{

template <typename CellType> class RealMedianFilter : public RealMedianFilterBase
{
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
		sourceMatrix_(true, ownerObj->getUseHuangAlgo()), destMatrix_(false, false) {}
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

private:
	//Приватные поля.

	//Матрица для исходного изображения. Инициализация в конструкторе.
	AltMatrix<CellType> sourceMatrix_;
	//Матрица для результата. Инициализация тоже в конструкторе т.к. иначе вызывается конструктор
	//переноса, а для AltMatrix он deleted.
	AltMatrix<CellType> destMatrix_;
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
	using PixFillerMethod = void (RealMedianFilter<CellType>::*)(const int &x,
		const int &y, const PixelDirection direction, const int &marginSize,
		const char &signMatrixValue);

	//Указатель на метод-фильтр
	using FilterMethod = void (RealMedianFilter<CellType>::*)(const int &currYToProcess,
		CallBackBase *callBackObj);

	//Приватные методы

	//Сделать шаг по пиксельным координатам в указанном направлении.
	//Вернёт false если координаты ушли за границы изображения, иначе true.
	bool PixelStep(int &x, int &y, const PixelDirection direction);

	//Получает координаты действительного пикселя, зеркального для
	//currXY по отношению к центральному xy. Результат записывается
	//в outXY.
	void GetMirrorPixel(const int &x, const int &y, const int &currX, const int &currY, int &outX, int &outY, const int recurseDepth = 0);

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

//Алиасы для классов, работающих с реально использующимися в GeoTIFF типами пикселей.
using RealMedianFilterFloat64 = RealMedianFilter<double>;
using RealMedianFilterFloat32 = RealMedianFilter<float>;
using RealMedianFilterInt8 = RealMedianFilter<int8_t>;
using RealMedianFilterUInt8 = RealMedianFilter<uint8_t>;
using RealMedianFilterInt16 = RealMedianFilter<int16_t>;
using RealMedianFilterUInt16 = RealMedianFilter<uint16_t>;
using RealMedianFilterInt32 = RealMedianFilter<int32_t>;
using RealMedianFilterUInt32 = RealMedianFilter<uint32_t>;

}	//namespace geoimgconv

#include "median_filter_base.h"