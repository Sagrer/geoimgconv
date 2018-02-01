///////////////////////////////////////////////////////////
//                                                       //
//                  GeoImageConverter                    //
//       Преобразователь изображений с геоданными        //
//       Copyright © 2017 Александр (Sagrer) Гриднев     //
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
#include "small_tools_box.h"
#include <boost/filesystem.hpp>
#include <sstream>

namespace geoimgconv
{

//Константы со значениями полей по умолчанию.
const std::string DEFAULT_INPUT_FILE_NAME = "input.tif";
const std::string DEFAULT_OUTPUT_FILE_NAME = "output.tif";
const size_t DEFAULT_MEDFILTER_APERTURE = 101;
const double DEFAULT_MEDFILTER_THRESHOLD = 0.5;
const MarginType DEFAULT_MEDFILTER_MARGIN_TYPE = MARGIN_MIRROR_FILLING;
const AppMode DEFAULT_APP_MODE = APPMODE_MEDIAN;

//////////////////////////////////
//        Класс AppConfig       //
//////////////////////////////////

//--------------------------------//
//   Конструкторы-деструкторы     //
//--------------------------------//

//Конструктор по умолчанию просто проставляет умолчальные собственно значения туда,
//где это вообще может иметь смысл.
AppConfig::AppConfig() : inputFileNameCfg_(DEFAULT_INPUT_FILE_NAME),
	inputFileNameCfgIsSaving_(false), inputFileNameCmdIsSet_(false),
	outputFileNameCfg_(DEFAULT_OUTPUT_FILE_NAME), outputFileNameCfgIsSaving_(false),
	outputFileNameCmdIsSet_(false), medfilterApertureCfg_(DEFAULT_MEDFILTER_APERTURE),
	medfilterApertureCfgIsSaving_(false), medfilterApertureCmdIsSet_(false),
	medfilterThresholdCfg_(DEFAULT_MEDFILTER_THRESHOLD), medfilterThresholdCfgIsSaving_(false),
	medfilterThresholdCmdIsSet_(false), medfilterMarginTypeCfg_(DEFAULT_MEDFILTER_MARGIN_TYPE),
	medfilterMarginTypeCfgIsSaving_(false), medfilterMarginTypeCmdIsSet_(false),
	appModeCfg_(DEFAULT_APP_MODE), appModeCfgIsSaving_(false), appModeCmdIsSet_(false),
	helpAsked_(false), versionAsked_(false), argc_(0), argv_(NULL), appPath_(""),
	currPath_(""), helpParamsDesc_(NULL)
{
	//Надо сразу заполнить базовые объекты program_options
	this->FillBasePO_();
};

//Деструктор
AppConfig::~AppConfig()
{
	delete helpParamsDesc_;
}

//--------------------------------//
//       Приватные методы         //
//--------------------------------//

bool AppConfig::ReadConfigFile_(const std::string &filePath, ErrorInfo *errObj)
//Прочитать ini-файл по указанному пути. Файл должен существовать.
{
	//Заглушка
	if (errObj) errObj->SetError(CMNERR_FEATURE_NOT_READY);
	return false;
};

void AppConfig::FillBasePO_()
//Заполнить заполнябельные ещё до разбора командной строки объекты
//program_options. Вызывается конструктором.
{
	namespace po = boost::program_options;
	//Извратный синтаксис с перегруженными скобками.
	//Описание пустое т.к. хелп генерится не здесь.
	this->cmdLineParamsDesc_.add_options()
		("help,h","")
		("version,v","")
		("input", po::value<std::string>(), "" )
		("output", po::value<std::string>(), "" )
		("medfilter.aperture", po::value<int>(), "" )
		("medfilter.threshold", po::value<double>(), "")
		("medfilter.margintype", po::value<std::string>(), "")
		("appmode", po::value<std::string>(), "")
	;
	//Позиционные параметры (без имени). Опять извратный синтаксис.
	//Это значит что первый безымянный параметр в количестве 1 штука
	//будет поименован как input а второй как output.
	this->positionalDesc_.add("input",1).add("output",1);
	//TODO: аналогично задать configParamsDesc_
}

void AppConfig::FillDependentPO_()
//Заполнить остальные объекты program_options, которые могут быть заполнены
//только после чтения параметров командной строки и конфига.
{
	//Считаем что STB настроен на нужную Selected-кодировку.
	namespace po = boost::program_options;
	//Заполняем desc, который единственное что делает - генерирует текст
	//справки по опциям.
	helpParamsDesc_ = new po::options_description(STB.Utf8ToSelectedCharset("Опции командной строки"));
	this->helpParamsDesc_->add_options()
		("help,h",STB.Utf8ToSelectedCharset("Вывести справку по опциям (ту, которую \
Вы сейчас читаете).").c_str())
		("version,v",STB.Utf8ToSelectedCharset("Вывести информацию о версии программы \
и не выполнять никаких других действий.").c_str())
		//("input", po::value<std::string>,STB.Utf8ToSelectedCharset("Скрытая опция, \
		//	задаёт имя входного файла. По умолчанию равна input.tif").c_str())
		//("output", po::value<std::string>,STB.Utf8ToSelectedCharset("Скрытая опция, \
		//	задаёт имя выходного файла. По умолчанию равна output.tif").c_str())
		("medfilter.aperture", po::value<int>(),STB.Utf8ToSelectedCharset("Длина стороны \
окна медианного фильтра в пикселях. Округляется вверх до ближайшего нечетного \
значения. По умолчанию равна "+boost::lexical_cast<std::string>(DEFAULT_MEDFILTER_APERTURE)
+" пикс.").c_str())
		("medfilter.threshold", po::value<double>(),STB.Utf8ToSelectedCharset("Порог \
медианного фильтра в метрах. Если отличие значения пикселя от медианы не \
превышает этого значение - пиксель при фильтрации не изменяется. По умолчанию \
опция равна "+boost::lexical_cast<std::string>(DEFAULT_MEDFILTER_THRESHOLD)+" (метров).").c_str())
		("medfilter.margintype", po::value<std::string>(),STB.Utf8ToSelectedCharset(
"Тип заполнения краевых областей для медианного фильтра. Заполнение нужно для \
того, чтобы пикселы, находящиеся по краям значимой части изображения также можно \
было обработать фильтром, т.е. чтобы в окне вокруг этого пикселя были незначимые \
пиксели, похожие на обычные значимые (в частности, не сильно отличающиеся по \
высоте). Возможные варианты заполнения:\n    " + MarginTypesTexts[MARGIN_SIMPLE_FILLING]
+ " - требующая заполнения область заполняется копией краевого пикселя. Работает быстро, но \
создаёт холмы если копируемый пиксель сильно отличается по своей высоте от окружающих. \n\
    "+MarginTypesTexts[MARGIN_MIRROR_FILLING]+" - краевые области заполняются зеркальным \
отражением изображения, расположенного в противоположную сторону от краевого пикселя. \
Работает медленно, но при достаточном размере окна позволяет например избежать появления \
холмов на месте леса при фильтрации.").c_str())
//		("appmode", po::value<std::string>(),STB.Utf8ToSelectedCharset("Режим работы \
//программы. На данный момент единственный работающий вариант - median.").c_str())
	;
}

//--------------------------------//
//         Прочие методы          //
//--------------------------------//

const std::string AppConfig::getHelpMsg()
//helpMsg (генерируется)
{
	std::ostringstream tempStream;
	tempStream << STB.Utf8ToSelectedCharset(
		"Способ вызова:\ngeoimgconv [опции] [имя входящего файла] [имя исходящего файла]\n\n\
По умолчанию (если вызвать без опций) - входящим файлом будет input.tif, а \n\
исходящим output.tif.\n\n\
Программа обрабатывает входящий файл медианным фильтром и, сохраняя метаданные\n\
- записывает то что получилось в исходящий файл.");
	tempStream << *this->helpParamsDesc_;
	return tempStream.str();
}

bool AppConfig::ParseCommandLine(const int &argc, char **argv, ErrorInfo *errObj)
//Прочитать параметры командной строки.
{
	namespace po = boost::program_options;
	po::options_description desc;
	
	//argc и argv могут потребоваться пользователям объекта. Их надо запомнить.
	this->argc_ = argc;
	this->argv_ = argv;
	
	//Задетектить пути
	boost::filesystem::path p(argv[0]);
	//make_preferred - чтобы поправить кривые пути типа C:/somedir\blabla\loolz.txt
	this->appPath_ = STB.WstringToUtf8(
		boost::filesystem::canonical(p.parent_path()).make_preferred().wstring());
	this->currPath_ = STB.WstringToUtf8(boost::filesystem::current_path().wstring());

	this->poVarMap_.clear();
	//Вот эта жуть читает в poVarMap_ параметры командной строки в соответствии с
	//инфой о параметрах из cmdLineParamsDesc_ и с учётом списка позиционных
	//(неименованных) параметров, заданных в positionalDesc_.
	po::store(po::command_line_parser(argc, argv).options(this->cmdLineParamsDesc_).
		positional(this->positionalDesc_).run(), this->poVarMap_);
	//notify в данном случае не нужно.
	
	//Я не стал пользоваться фичей автоматической загрузки значений по проставленным в
	//cmdLineParamsDesc_ ссылкам т.к. всё равно нужно просматривать poVarMap_ на предмет
	//того, какие именно параметры вообще были переданы в командной строке. Что сейчас и
	//будет происходить:
	if (this->poVarMap_.count("appmode"))
	{
		this->appModeCmd_ = AppModeStrToEnum(this->poVarMap_["appmode"].
			as<std::string>());
		if (this->appModeCmd_ != APPMODE_MEDIAN)
		{
			//Неизвестный или пока не реализованный вариант.
			if(errObj) errObj->SetError(CMNERR_UNKNOWN_IDENTIF,": " +
				this->poVarMap_["appmode"].as<std::string>());
			return false;
		};
		this->appModeCmdIsSet_ = true;
	};
	if (this->poVarMap_.count("help"))
		this->helpAsked_ = true;
	if (this->poVarMap_.count("version"))
		this->versionAsked_ = true;
	if ((this->helpAsked_) || (this->versionAsked_))
		//Дальше обрабатывать нет смысла, прога ничего не будет делать кроме вывода
		//собственно версии или справки.
		return true;
	if (this->poVarMap_.count("input"))
	{
		this->inputFileNameCmd_ = STB.SystemCharsetToUtf8(this->poVarMap_["input"].as<std::string>());
		this->inputFileNameCmdIsSet_ = true;
	};
	if (this->poVarMap_.count("output"))
	{
		this->outputFileNameCmd_ = STB.SystemCharsetToUtf8(this->poVarMap_["output"].as<std::string>());
		this->outputFileNameCmdIsSet_ = true;
	};
	if (this->poVarMap_.count("medfilter.aperture"))
	{
		this->medfilterApertureCmd_ = this->poVarMap_["medfilter.aperture"].as<int>();
		this->medfilterApertureCmdIsSet_ = true;
	};
	if (this->poVarMap_.count("medfilter.threshold"))
	{
		this->medfilterThresholdCmd_ = this->poVarMap_["medfilter.threshold"].as<double>();
		this->medfilterThresholdCmdIsSet_ = true;
	};
	if (this->poVarMap_.count("medfilter.margintype"))
	{
		this->medfilterMarginTypeCmd_ = MarginTypeStrToEnum(this->poVarMap_[
			"medfilter.margintype"].as<std::string>());
		if (this->medfilterMarginTypeCmd_ == MARGIN_UNKNOWN_FILLING)
		{
			//Неизвестный или пока не реализованный вариант.
			if(errObj) errObj->SetError(CMNERR_UNKNOWN_IDENTIF,": " +
				this->poVarMap_["medfilter.margintype"].as<std::string>());
			return false;
		};
		this->medfilterMarginTypeCmdIsSet_ = true;
	};
		
	//TODO: нужна обработка возможного здесь исключения!
	//if (errObj) errObj->SetError(CMNERR_FEATURE_NOT_READY);
	return true;
}

bool AppConfig::ReadConfigFile(ErrorInfo *errObj)
//Прочитать ini-файл с настройками. Файл ищется по стандартным местам
//где он может быть и имеет стандартное для программы имя.
{
	//Заглушка
	if (errObj) errObj->SetError(CMNERR_FEATURE_NOT_READY);
	return false;
}

} //namespace geoimgconv