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

//Базовый абстрактный класс для шаблонных классов с пиксель-специфическим кодом
//фильтров

#include <string>
#include "call_back_base.h"
#include "errors.h"

namespace geoimgconv
{

class FilterBase;

class PixelTypeSpecieficFilterBase
{
public:
	//Доступ к ссылке на объект-хозяин
	FilterBase& getOwnerObj() const { return *ownerObj_; }

	//Запретим конструктор по умолчанию, копирующий и переносящие конструкторы и аналогичные
	//им операторы присваивания.
	PixelTypeSpecieficFilterBase() = delete;
	PixelTypeSpecieficFilterBase(const PixelTypeSpecieficFilterBase&) = delete;
	PixelTypeSpecieficFilterBase(PixelTypeSpecieficFilterBase&&) = delete;
	PixelTypeSpecieficFilterBase& operator=(const PixelTypeSpecieficFilterBase&) = delete;
	PixelTypeSpecieficFilterBase& operator=(PixelTypeSpecieficFilterBase&&) = delete;
	//Cоздать объект можно только передав ссылку на FilterBase
	PixelTypeSpecieficFilterBase(FilterBase *ownerObj) : ownerObj_(ownerObj) {};
	virtual ~PixelTypeSpecieficFilterBase() {};

	//Абстрактные методы

	//Заполняет граничные (пустые пиксели) области вокруг значимых пикселей в соответствии с
	//выбранным алгоритмом.
	virtual void FillMargins(const int yStart, const int yToProcess, CallBackBase *callBackObj = NULL) = 0;

	//Этот полиморфный метод вызывается ApplyFilter-ом и обрабатывает изображение правильным алгоритмом.
	//Первый аргумент указывает количество строк матрицы для реальной обработки.
	//ДОЛЖЕН быть override в реальном наследнике.
	virtual void FilterMethod(const int &currYToProcess, CallBackBase *callBackObj) = 0;

	//Применит свой фильтр (переопределённый для наследника метод FilterMethod) к изображению.
	//Входящий и исходящий файлы уже должны быть подключены. Вернёт false и инфу об ошибке если
	//что-то пойдёт не так.
	virtual bool ApplyFilter(CallBackBase *callBackObj = NULL, ErrorInfo *errObj = NULL) = 0;

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
	FilterBase *ownerObj_;
};

}	//namespace geoimgconv

#include "filter_base.h"