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

//ВНИМАНИЕ! НА Debug-версии тесты долгие! Запускайте на Release!

#define BOOST_TEST_MODULE geoimgconv_tests
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
#include <boost/mpl/list.hpp>
#include <boost/test/debug.hpp>
#include <boost/dll.hpp>
#include "image_comparer.h"
#include "../alt_matrix.h"
#include "common_vars.h"
#include "../small_tools_box.h"

namespace b_ut = boost::unit_test;
namespace b_tt = boost::test_tools;
namespace b_fs = boost::filesystem;
using namespace geoimgconv;

//Глобальная фикстура. Объект этого класса тестовый движок инициализует до запуска
//каких-либо тестов.
struct GlobalFixture {
	GlobalFixture()
	{
		//Отключим детектор утечек памяти, поскольку работает он только в debug-сборках,
		//сделанных компиллятором вижлы + детектятся утечки в boost::locale которые если
		//верить форумам - не сильно то и утечки. При необходимости поиск утечек можно
		//будет временно включить прямо тут.
		boost::debug::detect_memory_leaks(false);

		//Узнаем и запомним пути - к исполнимому файлу, текущий путь и путь к тестовым данным.
		CommonVars &commonVars = CommonVars::Instance();
		//make_preferred - чтобы поправить кривые пути типа C:/somedir\blabla\loolz.txt
		b_fs::path appPath = boost::dll::program_location().
			parent_path().make_preferred();
		commonVars.setAppPath(STB.WstringToUtf8(appPath.wstring()));
		commonVars.setCurrPath(STB.WstringToUtf8(b_fs::current_path().wstring()));
		//Путь к тестовым данным может различаться. Он может начинаться как на 1 уровень выше
		//текущего каталога, так и на 2 уровня. Проверяем по наличию директории source.
		auto dataPath = b_fs::path(appPath.parent_path().wstring() + L"/source/tests/test_data")
			.make_preferred();
		if (!b_fs::exists(dataPath))
		{
			//Значит, возможно, оно на 2 уровня выше.
			dataPath = b_fs::path(appPath.parent_path().parent_path().wstring() +
				L"/source/tests/test_data").make_preferred();
		}
		if (b_fs::exists(dataPath))
		{
			//Путь найден. Всё ок.
			commonVars.setDataPath(STB.WstringToUtf8(dataPath.wstring()));
		}
		else
		{
			//Путь не найдён - будем надеяться что тестовые данные есть по текущему пути.
			//Иначе кирдык, который проявится зафейленными тестами.
			commonVars.setDataPath(commonVars.getCurrPath());
		}

		//Регистрация драйвера GDAL.
		GDALRegister_GTiff();
		//Явное включение именно консольной кодировки. К сожалению, Test Explorer вижлы не понимает
		//cp886 от Boost.Test. А если поставить там другую кодировку - нифига не будет видно из консоли.
		STB.SelectConsoleEncoding();
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
BOOST_AUTO_TEST_CASE_TEMPLATE(check_altmatrix_comparing, T, TestTypes)
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

//Тестируем работоспособность класса-сравнивалки картинок.
BOOST_AUTO_TEST_CASE(check_image_comparer)
{
	//Подразумевается что путь к тестовым данным был успешно найден фикстурой.

	//Сравнение одной и той же картинки должно быть успешным.
	CommonVars &commonVars = CommonVars::Instance();
	ErrorInfo errObj;
	double result = ImageComparer::CompareGeoTIFF(commonVars.getDataPath() + STB.GetFilesystemSeparator() + "in_uint8.tif",
		commonVars.getDataPath() + STB.GetFilesystemSeparator() + "in_uint8.tif", &errObj);
	BOOST_TEST_INFO("Checking: same pictures");
	if ((result >= -0.00001) && (result <= 0.00001))
		BOOST_TEST_INFO("Error was: "+ STB.Utf8ToConsoleCharset(errObj.getErrorText()));
	BOOST_TEST(result == 1.0, b_tt::tolerance(0.00001));

	//Сравнение разных картинок должно быть безуспешным.
	result = ImageComparer::CompareGeoTIFF(commonVars.getDataPath() + STB.GetFilesystemSeparator() + "in_uint8.tif",
		commonVars.getDataPath() + STB.GetFilesystemSeparator() + "in_float32.tif", &errObj);
	BOOST_TEST_INFO("Checking: different pictures");
	if ((result >= 0.99999) && (result <= 1.00001))
		BOOST_TEST_INFO("Error was: "+ STB.Utf8ToConsoleCharset(errObj.getErrorText()));
	BOOST_TEST(result == 0.0, b_tt::tolerance(0.00001));
}

////Это можно раскомментировать если надо будет протестировать зафейлившийся тест.
//BOOST_AUTO_TEST_CASE(dummy)
//{
//	BOOST_TEST(false);
//}