///////////////////////////////////////////////////////////
//                                                       //
//                  GeoImageConverter                    //
//       Преобразователь изображений с геоданными        //
//      Copyright © 2018 Александр (Sagrer) Гриднев      //
//              Распространяется на условиях             //
//                 GNU GPL v3 или выше                   //
//                  см. файл gpl.txt                     //
//                                                       //
///////////////////////////////////////////////////////////

//Место для списка авторов данного конкретного файла (если изменения
//вносили несколько человек).

////////////////////////////////////////////////////////////////////////

//Главный файл для юнит-тестирования. Тесты пишутся с помощью Boost.Test.
//Тут - только обращение к скриптам, которые сгенерируют главную функцию
//и всю такую прочую обвязку.

#define BOOST_TEST_MODULE geoimgconv_tests
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
#include <boost/mpl/list.hpp>
#include <boost/test/debug.hpp>
//#include "image_comparer.h"
#include "..\alt_matrix.h"

namespace b_ut = boost::unit_test;
namespace b_tt = boost::test_tools;
using namespace geoimgconv;

//Отключим детектор утечек памяти, поскольку работает он только в debug-сборках,
//сделанных компиллятором вижлы + детектятся утечки в boost::locale которые если
//верить форумам - не сильно то и утечки. При необходимости поиск утечек можно
//будет временно включить прямо тут.
struct GlobalFixture {
	GlobalFixture() {
		boost::debug::detect_memory_leaks(false);
	}
	~GlobalFixture() {  }
};
BOOST_GLOBAL_FIXTURE(GlobalFixture);

//Сюда можно засунуть какие-нибудь тесты, которые непонятно куда деть.
//Но лучше основную часть тестов писать в отдельных файлах. В этих файлах
//НЕ ДОЛЖНО быть дефайна BOOST_TEST_MODULE!

//Типы пикселей, для которых будем тестировать всякое разное.
using TestTypes = boost::mpl::list<double, float, boost::int8_t, boost::uint8_t,
	boost::int16_t, boost::uint16_t, boost::int32_t, boost::uint32_t>;

//Проверяем работоспособность сравнения 2 матриц в AltMatrix. Оно нужно для других тестов.
BOOST_AUTO_TEST_CASE_TEMPLATE(altmatrix_comparing_test, T, TestTypes)
{
	AltMatrix<T> testMatr1, testMatr2;
	const int matrSize = 10;
	testMatr1.CreateEmpty(matrSize, matrSize);
	testMatr2.CreateEmpty(matrSize, matrSize);

	//Заполняем одинаково.
	int i, j;
	for (i = 0; i<matrSize; ++i)
	{
		for (j = 0; j<matrSize; ++j)
		{
			testMatr1.setMatrixElem(i, j, 1);
			testMatr2.setMatrixElem(i, j, 1);
		}
	}
	//Убеждаемся что результат сравнения стремится к единице.
	double result = testMatr1.CompareWithAnother(&testMatr2);
	BOOST_TEST_INFO("Checking: 0.0 difference.");
	BOOST_TEST(result == 1.0, b_tt::tolerance(0.00001));

	//Заполняем полностью по разному.
	for (i = 0; i<matrSize; ++i)
	{
		for (j = 0; j<matrSize; ++j)
		{
			testMatr2.setMatrixElem(i, j, 0);
		}
	}
	//Убеждаемся что результат сравнения стремится к нулю.
	result = testMatr1.CompareWithAnother(&testMatr2);
	BOOST_TEST_INFO("Checking: 1.0 difference.");
	BOOST_TEST(result == 0.0, b_tt::tolerance(0.00001));

	//Заполняем 1/10 одинаковыми значениями.
	i = 0;
	for (j = 0; j<matrSize; ++j)
	{
		testMatr2.setMatrixElem(i, j, 1);
	}
	//Убеждаемся что результат сравнения стремится к 0.1.
	result = testMatr1.CompareWithAnother(&testMatr2);
	BOOST_TEST_INFO("Checking: 0.9 difference.");
	BOOST_TEST(result == 0.1, b_tt::tolerance(0.00001));

	//Убеждаемся что нельзя сравнить несравнимое.
	//Сначала по размеру.
	testMatr2.CreateEmpty(2, 2);
	result = testMatr1.CompareWithAnother(&testMatr2);
	BOOST_TEST_INFO("Checking: wrong size comparing.");
	BOOST_TEST(result == 0.0, b_tt::tolerance(0.00001));

	//Теперь по типу пикселя. Матрицы разного типа не могут быть похожи больше чем
	//на 0% даже при равных размерах и численных значениях элементов.
	AltMatrix<int16_t> testMatr3;
	AltMatrix<int32_t> testMatr4;
	testMatr3.CreateEmpty(matrSize, matrSize);
	testMatr4.CreateEmpty(matrSize, matrSize);
	//На всякий случай убедимся что формально они заполнены как бы идентичными значениями.
	for (i = 0; i<matrSize; ++i)
	{
		for (j = 0; j<matrSize; ++j)
		{
			testMatr3.setMatrixElem(i, j, 1);
			testMatr4.setMatrixElem(i, j, 1);
		}
	}
	result = testMatr3.CompareWithAnother(&testMatr4);
	BOOST_TEST_INFO("Checking: wrong type comparing.");
	BOOST_TEST(result == 0.0, b_tt::tolerance(0.00001));
}