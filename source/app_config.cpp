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

//Класс для работы с командной строкой и с конфигурационными файлами.

#include "app_config.h"
#include "strings_tools_box.h"
#include <boost/filesystem.hpp>
#include <sstream>
#include "common.h"

using namespace std;

namespace geoimgconv
{

//Константы со значениями полей по умолчанию см. в common.cpp

//////////////////////////////////
//        Класс AppConfig       //
//////////////////////////////////

//--------------------------------//
//   Конструкторы-деструкторы     //
//--------------------------------//

//Конструктор. helpLineLength если равно 0 то ширина генерируемой справки остаётся на усмотрение
//объекта, если же там некое число колонок - то ширина справки будет ему соответствовать.
AppConfig::AppConfig(unsigned short helpLineLength) : 
	helpLineLength_(helpLineLength)
{
	//Надо сразу заполнить базовые объекты program_options
	FillBasePO_();
};

//Деструктор
AppConfig::~AppConfig()
{
	delete helpParamsDesc_;
}

//--------------------------------//
//       Приватные методы         //
//--------------------------------//

//Прочитать ini-файл по указанному пути. Файл должен существовать.
bool AppConfig::ReadConfigFile_(const string &filePath, ErrorInfo *errObj)
{
	//Заглушка
	if (errObj) errObj->SetError(CommonErrors::FeatureNotReady);
	return false;
};

//Заполнить заполнябельные ещё до разбора командной строки объекты
//program_options. Вызывается конструктором.
void AppConfig::FillBasePO_()
{
	namespace po = boost::program_options;
	//Извратный синтаксис с перегруженными скобками.
	//Описание пустое т.к. хелп генерится не здесь.
	cmdLineParamsDesc_.add_options()
		("help,h","")
		("help?,?", "")	//Полускрытая опция. В справке говорится только о -?. А help? - костыль для boost.
		("version,v","")
		("input", po::value<string>(), "" )
		("output", po::value<string>(), "" )
		("medfilter.aperture", po::value<int>(), "" )
		("medfilter.threshold", po::value<double>(), "")
		("medfilter.margintype", po::value<string>(), "")
		("medfilter.algo", po::value<string>(), "")
		("medfilter.huanglevels", po::value<uint16_t>(), "")
		("medfilter.fillpits", "")
		("appmode", po::value<string>(), "")
		("memmode", po::value<string>(), "")
		("test","")	//Не документировать эту опцию в справку юзера! Только для разработки.
	;
	//Позиционные параметры (без имени). Опять извратный синтаксис.
	//Это значит что первый безымянный параметр в количестве 1 штука
	//будет поименован как input а второй как output.
	positionalDesc_.add("input",1).add("output",1);
	//TODO: аналогично задать configParamsDesc_
}

//Заполнить остальные объекты program_options, которые могут быть заполнены
//только после чтения параметров командной строки и конфига.
void AppConfig::FillDependentPO_()
{
	//Считаем что STB настроен на нужную Selected-кодировку.
	namespace po = boost::program_options;
	//Заполняем desc, который единственное что делает - генерирует текст
	//справки по опциям.
	if (helpParamsDesc_)
	{
		delete helpParamsDesc_;
	}
	if (helpLineLength_)
	{
		//Возможно нужно применить костыль для нормального отображения кириллицы в utf8.
		unsigned short currLength;
		if ((StrTB::GetConsoleEncoding() == "utf8") || (StrTB::GetConsoleEncoding() == "utf-8"))
		{
			currLength = (unsigned short)(helpLineLength_ * 1.3);
		}
		else
		{
			currLength = helpLineLength_;
		}
		helpParamsDesc_ = new po::options_description(StrTB::Utf8ToSelectedCharset("Опции командной строки"),
			currLength);
	}
	else
	{
		helpParamsDesc_ = new po::options_description(StrTB::Utf8ToSelectedCharset("Опции командной строки"));
	}
	helpParamsDesc_->add_options()
		("help,h", StrTB::Utf8ToSelectedCharset("Вывести справку по опциям (ту, которую \
Вы сейчас читаете).").c_str())
		(",?", StrTB::Utf8ToSelectedCharset("То же, что и -h").c_str())
		("version,v", StrTB::Utf8ToSelectedCharset("Вывести информацию о версии программы \
и не выполнять никаких других действий.").c_str())
		//("input", po::value<string>,StrTB::Utf8ToSelectedCharset("Скрытая опция, \
		//	задаёт имя входного файла. По умолчанию равна input.tif").c_str())
		//("output", po::value<string>,StrTB::Utf8ToSelectedCharset("Скрытая опция, \
		//	задаёт имя выходного файла. По умолчанию равна output.tif").c_str())
		("medfilter.aperture", po::value<int>(),StrTB::Utf8ToSelectedCharset("Длина стороны \
окна медианного фильтра в пикселях. Округляется вверх до ближайшего нечетного \
значения. По умолчанию равна "+to_string(DEFAULT_MEDFILTER_APERTURE) +" пикс.").c_str())
		("medfilter.threshold", po::value<double>(),StrTB::Utf8ToSelectedCharset("Порог \
медианного фильтра в метрах. Если отличие значения пикселя от медианы не \
превышает этого значение - пиксель при фильтрации не изменяется. По умолчанию \
опция равна "+to_string(DEFAULT_MEDFILTER_THRESHOLD)+" (метров).").c_str())
		("medfilter.margintype", po::value<string>(),StrTB::Utf8ToSelectedCharset(
"Тип заполнения краевых областей для медианного фильтра. Заполнение нужно для \
того, чтобы пикселы, находящиеся по краям значимой части изображения также можно \
было обработать фильтром, т.е. чтобы в окне вокруг этого пикселя были незначимые \
пиксели, похожие на обычные значимые (в частности, не сильно отличающиеся по \
высоте). Возможные варианты заполнения:\n    " + MarginTypesTexts[(unsigned char)MarginType::SimpleFilling]
+ " - требующая заполнения область заполняется копией краевого пикселя. Работает быстро, но \
создаёт холмы если копируемый пиксель сильно отличается по своей высоте от окружающих. \n\
    "+MarginTypesTexts[(unsigned char)MarginType::MirrorFilling]+" - краевые области заполняются зеркальным \
отражением изображения, расположенного в противоположную сторону от краевого пикселя. \
Работает медленно, но при достаточном размере окна позволяет например избежать появления \
холмов на месте леса при фильтрации.").c_str())
		("medfilter.algo", po::value<string>(), StrTB::Utf8ToSelectedCharset(
"Алгоритм медианного фильтра. Возможные варианты:\n    " + MedfilterAlgoTexts[(unsigned char)MedfilterAlgo::Stub]
+ " - пустой алгоритм. Ничего не делает, исходный файл просто копируется.\n    "
+ MedfilterAlgoTexts[(unsigned char)MedfilterAlgo::Stupid] + " - алгоритм, обрабатывающий изображение \
\"в лоб\", т.е. на каждой итерации берутся все пиксели апертуры, сортируются, берётся медиана. \
Этот алгоритм работает медленно, но медиана вычисляется без вызванной к примеру квантованием по \
уровням потери точности. Это вариант по умолчанию (т.е применяется если опция не была указана).\n\
    " + MedfilterAlgoTexts[(unsigned char)MedfilterAlgo::Huang] + " - алгоритм быстрой медианной фильтрации, \
предложенный Хуангом и соавторами. Реализован с незначительными модификациями, призванными \
немного ускорить обработку \"неровных\" изображений. Работает значительно быстрее чем обработка \
изображения \"в лоб\", т.к на каждом следующем пикселе используется часть информации, полученной при \
обработке предыдущего.").c_str())
		("medfilter.huanglevels", po::value<uint16_t>(), StrTB::Utf8ToSelectedCharset(
"Количество уровней квантования для медианной фильтрации алгоритмом Хуанга. По умолчанию этот \
параметр равен " + to_string(DEFAULT_HUANG_LEVELS_NUM) + ". Максимально \
возможное значение этого параметра: " + to_string(HUANG_MAX_LEVELS_NUM) + ".").c_str())
		("medfilter.fillpits", StrTB::Utf8ToSelectedCharset("Включить заполнение ям в медианном фильтре. \
Смысл в том, что если точка рельефа находится значительно ниже медианы, то при данной включённой опции \
данная точка станет равна медиане, но обычно это не имеет смысла т.к. алгоритм обычно применяется для \
срезания с карты высот растительности и различного техногена, а вовсе не для заполнения ям и оврагов. \
По умолчанию эта опция отключена в отличие от предыдущих версий (в которых этой опции не было).").c_str())
//		("appmode", po::value<string>(),StrTB::Utf8ToSelectedCharset("Режим работы \
//программы. На данный момент единственный работающий вариант - median.").c_str())
		("memmode", po::value<string>(), StrTB::Utf8ToSelectedCharset("Режим использования \
памяти. Позволяет ограничить количество памяти, которое программа может использовать для загрузки \
обрабатываемого изображения. Если изображение не поместится в эту ограниченную область целиком то \
оно будет обрабатываться по частям. Возможно указать один из следующих режимов:\n    " +
MemoryModeTexts[(unsigned char)MemoryMode::Auto] + " - оставить решение о режиме работы с памятью на усмотрение \
программы;\n    " + MemoryModeTexts[(unsigned char)MemoryMode::Limit] + " - явно задать максимальное количество \
используемой памяти в байтах. Сразу после имени режима без пробела нужно указать это количество. \
Буквы k, m, g, t после числа (без пробела) означают соответственно килобайты, мегабайты, гигабайты \
и терабайты. Размер в байтах указывается без буквы или буквой b;\n    " +
MemoryModeTexts[(unsigned char)MemoryMode::StayFree] + " - явно задать количество памяти в ОЗУ (т.е. сколько есть \
физически в компьютере, без учёта \"подкачки\"!), которое должно остаться свободным в момент начала \
работы над изображением. Размер указывается без пробела сразу за именем режима, точно так же как и для \
режима " + MemoryModeTexts[(unsigned char)MemoryMode::Limit] + ";\n    " + MemoryModeTexts[(unsigned char)MemoryMode::LimitFreePrc] +
" - задать количество памяти в процентах от свободного ОЗУ в момент начала работы. Сразу после имени \
режима без пробела должно быть указано целое число от 0 до 100;\n    " +
MemoryModeTexts[(unsigned char)MemoryMode::LimitFullPrc] + " - задать количество памяти в процентах от общего \
количества ОЗУ. Сразу после имени режима без пробела должно быть указано целое число от 0 до 100;\n    " +
MemoryModeTexts[(unsigned char)MemoryMode::OneChunk] + " - заставить программу попытаться обработать изображение одним \
куском, загрузив её в память сразу целиком.\n    По умолчанию (если не указано) используется режим " +
MemoryModeTexts[(unsigned char)MemoryMode::Auto] + ".").c_str())
	;
}

//Получить MemoryMode из строки, совпадающей без учёта регистра с одним из
//элементов MemoryModeTexts + прочитать и правильно интерпретировать
//указанный там размер.
void AppConfig::ParseMemoryModeStr(const string &inputStr, MemoryMode &memMode, unsigned long long &size)
{
	string inpStr;
	StrTB::Utf8ToLower(inputStr, inpStr);
	size = 0;
	//Помимо перевода в нижний регистр от строки нужно отрезать часть с возможными
	//цифрами.
	size_t pos = inpStr.find_first_of("0123456789", 0);
	if (pos != string::npos)
		inpStr = inpStr.substr(0, pos);
	//Теперь можно искать совпадение с одной из констант.
	memMode = MemoryMode::Unknown;
	for (unsigned char i = 0; i <= (unsigned char)MemoryMode::Unknown; i++)
	{
		if (inpStr == MemoryModeTexts[i])
		{
			memMode = MemoryMode(i);
			break;
		}
	};
	//Для тех констант, у которых должен был быть указан размер - прочитаем размер.
	if ((memMode == MemoryMode::Limit) || (memMode == MemoryMode::StayFree))
	{
		//Размер должен быть указан в байтах (кб, мб итд).
		if (pos != string::npos)
			size = StrTB::InfoSizeToBytesNum(inputStr.substr(pos, inputStr.length() - pos),'m');
		//Нулевого размера не бывает
		if (!size)
			memMode = MemoryMode::Unknown;
	}
	else if ((memMode == MemoryMode::LimitFreePrc) || (memMode == MemoryMode::LimitFullPrc))
	{
		//Размер должен быть указан в процентах, т.е. просто целое число.
		if (pos != string::npos)
		{
			inpStr = inputStr.substr(pos, inputStr.length() - pos);
			if (StrTB::CheckUnsIntStr(inpStr))
			{
				size = stoull(inpStr);
				if (size > 100)
					//Нельзя указывать больше 100%
					size = 0;
			}
		}
		//Если размер остался нулевым - всё плохо.
		if (!size)
			memMode = MemoryMode::Unknown;
	}
}

//--------------------------------//
//         Прочие методы          //
//--------------------------------//

//helpMsg (генерируется)
const string AppConfig::getHelpMsg()
{
	ostringstream tempStream;
	tempStream << StrTB::Utf8ToSelectedCharset(
		"Способ вызова:\ngeoimgconv [опции] [имя входящего файла] [имя исходящего файла]\n\n\
По умолчанию (если вызвать без опций) - входящим файлом будет input.tif, а \n\
исходящим output.tif.\n\n\
Программа обрабатывает входящий файл медианным фильтром и, сохраняя метаданные\n\
- записывает то что получилось в исходящий файл.\n\n");
	tempStream << *helpParamsDesc_;
	return tempStream.str();
}

//Прочитать параметры командной строки.
bool AppConfig::ParseCommandLine(const int &argc, char **argv, ErrorInfo *errObj)
{
	namespace po = boost::program_options;
	po::options_description desc;

	//argc и argv могут потребоваться пользователям объекта. Их надо запомнить.
	argc_ = argc;
	argv_ = argv;

	//Задетектить пути
	boost::filesystem::path p(argv[0]);
	//make_preferred - чтобы поправить кривые пути типа C:/somedir\blabla\loolz.txt
	appPath_ = StrTB::WstringToUtf8(
		boost::filesystem::canonical(p.parent_path()).make_preferred().wstring());
	currPath_ = StrTB::WstringToUtf8(boost::filesystem::current_path().wstring());

	poVarMap_.clear();
	try
	{
		//Вот эта жуть читает в poVarMap_ параметры командной строки в соответствии с
		//инфой о параметрах из cmdLineParamsDesc_ и с учётом списка позиционных
		//(неименованных) параметров, заданных в positionalDesc_.
		po::store(po::command_line_parser(argc, argv).options(cmdLineParamsDesc_).
			positional(positionalDesc_).run(), poVarMap_);
		//notify в данном случае не нужно.
	}
	catch (po::required_option &err)
	{
		//Отсутствует обязательная опция.
		if (errObj) errObj->SetError(CommonErrors::CmdLineParseError, ": отсутствует обязательная опция " +
			err.get_option_name());
		return false;
	}
	catch (po::error &err)
	{
		//Непонятно что случилось. Просто перехватим все ошибки, которые может выдать program_options
		//т.к. это в любом случае будет что-то не то с синтаксисом в командной строке.
		//Как ни странно, program_options в what() нередко даёт информации больше чем можно
		//вытянуть по классу исключения, например для po::unknown_option там будет имя неизвестной
		//опции, но узнать её кроме как по what() нельзя :(.
		if (errObj) errObj->SetError(CommonErrors::CmdLineParseError, err.what(), true);
		return false;
		//Все остальные исключения считаем неожиданными, не обрабатываем и либо падаем с ними,
		//либо их обработают где-то наверху стека вызовов.
	};


	//Я не стал пользоваться фичей автоматической загрузки значений по проставленным в
	//cmdLineParamsDesc_ ссылкам т.к. всё равно нужно просматривать poVarMap_ на предмет
	//того, какие именно параметры вообще были переданы в командной строке. Что сейчас и
	//будет происходить:
	try
	{
		if (poVarMap_.count("appmode"))
		{
			appModeCmd_ = AppModeStrToEnum(poVarMap_["appmode"].
				as<string>());
			if (appModeCmd_ != AppMode::Median)
			{
				//Неизвестный или пока не реализованный вариант.
				if (errObj) errObj->SetError(CommonErrors::UnknownIdentif, ": " +
					poVarMap_["appmode"].as<string>());
				return false;
			};
			appModeCmdIsSet_ = true;
		};
		if (poVarMap_.count("test"))
		{
			//Скрытая опция для разработки. Программа запускается в отдельном
			//тестовом режиме.
			appModeCmd_ = AppMode::DevTest;
			appModeCmdIsSet_ = true;
		}
		if (poVarMap_.count("help"))
			helpAsked_ = true;
		if (poVarMap_.count("help?"))	//Скрытая опция для -?
			helpAsked_ = true;
		if (poVarMap_.count("version"))
			versionAsked_ = true;
		if ((helpAsked_) || (versionAsked_))
			//Дальше обрабатывать нет смысла, прога ничего не будет делать кроме вывода
			//собственно версии или справки.
			return true;
		if (poVarMap_.count("input"))
		{
			inputFileNameCmd_ = StrTB::SystemCharsetToUtf8(poVarMap_["input"].as<string>());
			inputFileNameCmdIsSet_ = true;
		};
		if (poVarMap_.count("output"))
		{
			outputFileNameCmd_ = StrTB::SystemCharsetToUtf8(poVarMap_["output"].as<string>());
			outputFileNameCmdIsSet_ = true;
		};
		if (poVarMap_.count("medfilter.aperture"))
		{
			medfilterApertureCmd_ = poVarMap_["medfilter.aperture"].as<int>();
			medfilterApertureCmdIsSet_ = true;
		};
		if (poVarMap_.count("medfilter.threshold"))
		{
			medfilterThresholdCmd_ = poVarMap_["medfilter.threshold"].as<double>();
			medfilterThresholdCmdIsSet_ = true;
		};
		if (poVarMap_.count("medfilter.margintype"))
		{
			medfilterMarginTypeCmd_ = MarginTypeStrToEnum(poVarMap_[
				"medfilter.margintype"].as<string>());
			if (medfilterMarginTypeCmd_ == MarginType::UnknownFilling)
			{
				//Неизвестный или пока не реализованный вариант.
				if (errObj) errObj->SetError(CommonErrors::UnknownIdentif, ": " +
					poVarMap_["medfilter.margintype"].as<string>());
				return false;
			};
			medfilterMarginTypeCmdIsSet_ = true;
		};
		if (poVarMap_.count("medfilter.algo"))
		{
			medfilterAlgoCmd_ = MedfilterAlgoStrToEnum(poVarMap_[
				"medfilter.algo"].as<string>());
			if (medfilterAlgoCmd_ == MedfilterAlgo::Unknown)
			{
				//Неизвестный или пока не реализованный вариант.
				if (errObj) errObj->SetError(CommonErrors::UnknownIdentif, ": " +
					poVarMap_["medfilter.algo"].as<string>());
				return false;
			};
			medfilterAlgoCmdIsSet_ = true;
		};
		if (poVarMap_.count("medfilter.huanglevels"))
		{
			medfilterHuangLevelsNumCmd_ = poVarMap_["medfilter.huanglevels"].as<uint16_t>();
			if (medfilterHuangLevelsNumCmd_ < 3)
			{
				//Нельзя использовать совсем уж мало уровней.
				if (errObj) errObj->SetError(CommonErrors::CmdLineParseError, ": параметр \
medfilter.huanglevels не может иметь значение меньше чем 3.");
				return false;
			}
			medfilterHuangLevelsNumCmdIsSet_ = true;
		};
		if (poVarMap_.count("medfilter.fillpits"))
		{
			medfilterFillPitsCmd_ = true;
			medfilterFillPitsCmdIsSet_ = true;
		};
		if (poVarMap_.count("memmode"))
		{
			ParseMemoryModeStr(poVarMap_["memmode"].as<string>(), memModeCmd_,
				memSizeCmd_);
			if (memModeCmd_ == MemoryMode::Unknown)
			{
				//Неизвестный или пока не реализованный вариант.
				if (errObj) errObj->SetError(CommonErrors::UnknownIdentif, "Параметр --memmode имеет неверное значение: " +
					poVarMap_["memmode"].as<string>(),true);
				return false;
			};
			memModeCmdIsSet_ = true;
		};
	}
	catch (po::error &err)
	{
		//Ловим все исключения program_options - это однозначно будут проблемы с синтаксисом
		//в командной строке.
		if (errObj) errObj->SetError(CommonErrors::CmdLineParseError, err.what(), true);
		return false;
		//Все остальные исключения считаем неожиданными и скорее всего падаем.
	};

	return true;
}

//Прочитать ini-файл с настройками. Файл ищется по стандартным местам
//где он может быть и имеет стандартное для программы имя.
bool AppConfig::ReadConfigFile(ErrorInfo *errObj)
{
	//Заглушка
	if (errObj) errObj->SetError(CommonErrors::FeatureNotReady);
	return false;
}

} //namespace geoimgconv
