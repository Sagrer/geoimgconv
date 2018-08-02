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

//Тесты для классов из median_filter.h
//Работают "поверхностно", т.е. чтобы не лезть в детали реализации, что позволит
//сильно менять то, как всё внутри устроено не ломая тесты и проверяя что ничего
//в процессе переделок не сломалось.

//ВНИМАНИЕ! НА Debug-версии эти тесты долгие! Запускайте на Release!

#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
#include <boost/test/data/test_case.hpp>
#include <boost/test/data/monomorphic.hpp>
#include <boost/mpl/list.hpp>
#include <boost/filesystem.hpp>
#include <string>
#include "../median_filter.h"
#include "../small_tools_box.h"
#include "../common.h"
#include "image_comparer.h"
#include "common_vars.h"

namespace b_ut = boost::unit_test;
namespace b_fs = boost::filesystem;
using std::string;
using namespace geoimgconv;

//Переменные и фикстура, которые понадобятся всем тестам.
static string outfileName = "";
static b_fs::path outfilePath;
struct MedfilterTestsFixture
{	
	MedfilterTestsFixture()
	{
		//Сгенерировать имена.
		if (outfileName == "")
		{
			outfileName = CommonVars::Instance().getCurrPath() + STB.GetFilesystemSeparator()
				+ "outtest.tif";
			outfilePath = STB.Utf8ToWstring(outfileName);
		}		
		
		//Перед началом теста надо удалить выходной файл, если он существует.
		FileRemover();
	}

	~MedfilterTestsFixture()
	{
		//По завершению теста - файл также надо удалить :).
		FileRemover();
	}

	void FileRemover()
	{
		if (b_fs::exists(outfilePath))
		{
			b_fs::remove(outfilePath);
		}
	}

	static void MedFilterPrepare(MedianFilterBase &medFilter, const int &aperture = DEFAULT_MEDFILTER_APERTURE)
	{
		medFilter.setFillPits(DEFAULT_MEDFILTER_FILL_PITS);
		medFilter.setAperture(aperture);
		medFilter.setThreshold(DEFAULT_MEDFILTER_THRESHOLD);
		medFilter.setMarginType(DEFAULT_MEDFILTER_MARGIN_TYPE);
		medFilter.setUseMemChunks(false);
	}
};

BOOST_AUTO_TEST_SUITE(median_filter_tests)

//Для фильтра Хуанга придётся создать производный класс с конструктором по умолчанию,
//чтобы можно было применять шаблонные тесты.
class MedianFilterHuangDefault : public MedianFilterHuang
{
public:
	MedianFilterHuangDefault() : MedianFilterHuang(DEFAULT_HUANG_LEVELS_NUM) {}
};

//Типы объектов-фильтров.
using FilterTypes = boost::mpl::list<MedianFilterStub, MedianFilterStupid,
	MedianFilterHuangDefault>;

//Проверяем все вариации фильтров на не поддерживаемый формат.
BOOST_FIXTURE_TEST_CASE_TEMPLATE(check_unsupported, T, FilterTypes, MedfilterTestsFixture)
{
	//Подготовка фильтра
	T medFilter;
	MedfilterTestsFixture::MedFilterPrepare(medFilter);

	//Применяем.
	bool result = medFilter.OpenInputFile(CommonVars::Instance().getDataPath() +
		STB.GetFilesystemSeparator() + "in_cint16.tif", nullptr);
	//Уже на этом этапе всё должно сломаться.
	BOOST_TEST_INFO("Checking stub filter: in_cint16.tif");
	BOOST_TEST(!result);
}

//Функция, которая будет идентична для всех остальных тестов. А всё потому, что Boost.Test
//умеет в data-driven-tests, умеет в темплейты, но не умеет (или я не нашёл) чтобы и то и
//другое одновременно. По крайней мере в удобном виде и с авторегистрацией. Придётся извращаться.
//Но, думаю, и так будет неплохо.
//T - тип фильтра.
//inputFileName - имя входного файла.
//inputFileName - имя файла-эталона, с которым сравниваем.
//minValue - минимальная допустимая степень похожести образца на эталон.
template <typename T>
void CheckFile(const string &inputFileName, const string &sampleFileName, const double &minValue,
	const int &aperture = DEFAULT_MEDFILTER_APERTURE)
{
	//Подготовка фильтра.
	T medFilter;
	//MedianFilterStub medFilter;
	MedfilterTestsFixture::MedFilterPrepare(medFilter, aperture);
	CommonVars &commonVars = CommonVars::Instance();
	ErrorInfo errObj;

	//Открываем входной файл.
	bool result = medFilter.OpenInputFile(commonVars.getDataPath() +
		STB.GetFilesystemSeparator() + inputFileName, &errObj);
	BOOST_TEST_INFO("Opening: " + inputFileName);
	if (!result)
	{
		medFilter.CloseAllFiles();
		BOOST_TEST_INFO("Error was: " + STB.Utf8ToConsoleCharset(errObj.getErrorText()));
	}
	BOOST_TEST_REQUIRE(result);

	//Открываем выходной файл.
	result = medFilter.OpenOutputFile(outfileName, true, &errObj);
	BOOST_TEST_INFO("Opening: " + outfileName);
	if (!result)
	{
		medFilter.CloseAllFiles();
		BOOST_TEST_INFO("Error was: " + STB.Utf8ToConsoleCharset(errObj.getErrorText()));
	}
	BOOST_TEST_REQUIRE(result);

	//Применяем фильтр.
	result = medFilter.ApplyFilter(nullptr, &errObj);
	BOOST_TEST_INFO("Filtering: " + inputFileName + " to " + outfileName);
	if (!result)
	{
		medFilter.CloseAllFiles();
		BOOST_TEST_INFO("Error was: " + STB.Utf8ToConsoleCharset(errObj.getErrorText()));
	}
	BOOST_TEST_REQUIRE(result);

	//Фильтр больше не нужен, закроем все файлы.
	medFilter.CloseAllFiles();

	//Сверяем полученное изображение с эталоном.
	double doubleResult = ImageComparer::CompareGeoTIFF(commonVars.getDataPath() +
		STB.GetFilesystemSeparator() + sampleFileName, outfileName);
	BOOST_TEST_INFO("Comparing result with: " + sampleFileName);
	BOOST_TEST_REQUIRE(doubleResult > minValue);
}

//Структура для data driven tests.
struct TestFileStruct
{
	string inputFile;
	string sampleFile;
	double minValue;
};

//Этот оператор нужен фреймворку чтобы если что ругаться в консоль, выводя структуру.
std::ostream& operator<<(std::ostream &ostr, const TestFileStruct &sample)
{
	ostr << "inputFile=\"" << sample.inputFile << "; sampleFile=\"" <<
		sample.sampleFile << "; minValue=" << sample.minValue;
	return ostr;
}


//Массив структур для data driven. Только для stub, поскольку он не меняет картинку,
//то и сравнивать надо не с эталонами а с оригиналами.
const TestFileStruct stubTestFiles[] = {
	{ "in_float32.tif", "in_float32.tif", 0.99 },
	{ "in_float64.tif", "in_float64.tif", 0.99 },
	{ "in_int16.tif", "in_int16.tif", 0.99 },
	{ "in_int32.tif", "in_int32.tif", 0.99 },
	{ "in_uint8.tif", "in_uint8.tif", 0.99 },
	{ "in_uint16.tif", "in_uint16.tif", 0.99 },
	{ "in_uint32.tif", "in_uint32.tif", 0.99 }
};

//Массив структур для data driven. Для алгоритма stupid.
const TestFileStruct stupidTestFiles[] = {
	{ "in_float32.tif", "out_31_float32.tif", 0.99 },
	{ "in_float64.tif", "out_31_float64.tif", 0.99 },
	{ "in_int16.tif", "out_31_int16.tif", 0.99 },
	{ "in_int32.tif", "out_31_int32.tif", 0.99 },
	{ "in_uint8.tif", "out_31_uint8.tif", 0.99 },
	{ "in_uint16.tif", "out_31_uint16.tif", 0.99 },
	{ "in_uint32.tif", "out_31_uint32.tif", 0.99 }
};

//Массив структур для data driven. Для алгоритма Huang.
const TestFileStruct huangTestFiles[] = {
	{ "in_float32.tif", "out_101_float32.tif", 0.99 },
	{ "in_float64.tif", "out_101_float64.tif", 0.99 },
	{ "in_int16.tif", "out_101_int16.tif", 0.99 },
	{ "in_int32.tif", "out_101_int32.tif", 0.99 },
	{ "in_uint8.tif", "out_101_uint8.tif", 0.99 },
	{ "in_uint16.tif", "out_101_uint16.tif", 0.99 },
	{ "in_uint32.tif", "out_101_uint32.tif", 0.99 }
};

//Проверяем stub-фильтр.
BOOST_DATA_TEST_CASE_F(MedfilterTestsFixture, check_stub, b_ut::data::make(stubTestFiles))
{
	CheckFile<MedianFilterStub>(sample.inputFile, sample.sampleFile, sample.minValue);
}

//Проверяем stupid-фильтр.
BOOST_DATA_TEST_CASE_F(MedfilterTestsFixture, check_stupid, b_ut::data::make(stupidTestFiles))
{
	//В данном случае апертура 31.
	CheckFile<MedianFilterStupid>(sample.inputFile, sample.sampleFile, sample.minValue, 31);
}

//Проверяем huang-фильтр.
BOOST_DATA_TEST_CASE_F(MedfilterTestsFixture, check_huang, b_ut::data::make(huangTestFiles))
{
	CheckFile<MedianFilterHuangDefault>(sample.inputFile, sample.sampleFile, sample.minValue);
}

BOOST_AUTO_TEST_SUITE_END()