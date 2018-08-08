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
#include "median_filter_stub.h"
#include "median_filter_stupid.h"
#include "median_filter_huang.h"

namespace geoimgconv
{

//Этот header подключает семейство классов для работы с медианным фильтром. Оно состоит из
//2 основных классов:

//PixelTypeSpecieficFilterBase с наследниками - класс, который выполняет работу над
//непосредственно изображением. Сюда вынесен код, специфичный для типа пиксела
//- именно для этого оно и вынесено в отдельный класс, раньше было всё в общей куче.
//Поскольку пикселы могут быть разными - базовый класс оборудован шаблонными
//наследниками и полиморфизмом. Напрямую эти классы лучше не использовать, только
//через обёртки (см. ниже).

//FilterBase - просто обёртка, которая однако занимается предварительным
//чтением картинки, определением её параметров и решает какой наследник PixelTypeSpecieficFilterBase
//будет использоваться для реальной работы над изображением.
//Сам по себе FilterBase использовать тоже нельзя - класс абстрактный, т.к. не имеет
//реализации непосредственно алгоритма фильтрации. Использовать нужно его наследники,
//а именно:

//MedianFilterStub - "никакой фильтр". Ничего не делает, просто копирует изображение. Удобен
//для тестирования и отладки всей остальной, не выполняющей непосредственно фильтрацию обвязки.

//MedianFilterStupid - "тупой фильтр". Работает нагло и в лоб, реализует алгоритм по определению
//- т.е. каждый раз берутся все пиксели окна, ищется медианное значение, записывается в центр окна.
//Никаких гистограмм, поэтому медленный. Применять его имеет смысл, наверное, лишь для
//сравнения результатов с более навороченными и оптимизированными реализациями медианного фильтра.

//MedianFilterHuang - то, ради чего всё и затевалось. Реализует алгоритм быстрой медианной
//фильтрации, предложенный Хуангом (через гистограмму, с квантованием если пиксели имеют
//не целочисленный тип). Много-гигабайтные картинки обрабатывает с окном в сотни пикселей за
//минуты, в то время как "тупой" фильтр тратит на то же самое часы и даже сутки.

//Да, вот эти вот наследники с якобы реализацией - фактически фикция, т.к. реальный алгоритм
//всё равно шаблонный (ибо пиксели бывают разные), а поэтому на самом деле реализация
//всех этих алгоритмов лежит среди методов PixelTypeSpecieficFilter<>. Это TODO: кандидат на
//разделение на отдельные подклассы в будущем.

//Обязательно надо проверить и запретить дальнейшую компиляцию если double и float
//означают не то что мы думаем.
static_assert(sizeof(double) == 8, "double size is not 64 bit! You need to fix the code for you compillator!");
static_assert(sizeof(float) == 4, "float size is not 32 bit! You need to fix the code for you compillator!");

}	//namespace geoimgconv