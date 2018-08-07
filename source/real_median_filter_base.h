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

//Базовый абстрактный класс для шаблонных классов.

#include <string>
#include "call_back_base.h"
#include "errors.h"

namespace geoimgconv
{

class MedianFilterBase;

class RealMedianFilterBase
{
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

private:
	//Нет смысла хранить тут поля типа aperture или threshold. Ибо этот класс нужен просто
	//чтобы вынести сюда код, который должен генерироваться по шаблону для разных типов пиксела.
	//Поэтому будем просто держать тут указатель на основной объект класса и брать значения полей
	//оттуда.
	MedianFilterBase *ownerObj_;
};

}	//namespace geoimgconv

#include "median_filter_base.h"