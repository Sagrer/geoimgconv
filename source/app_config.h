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

//Класс для работы с параметрами командной строки и конфиг-файлом приложения.
//Зачем:
//Можно было бы работать со значениями в map, созданном program_options напрямую,
//но мне не нравится идея обращаться к полям по их текстовому имени во время
//исполнения, а если пользоваться фичей с загрузкой значений полей по ссылке
//то переменные, ссылки на которые отдаются в program_options всё равно удобнее
//держать в полях объекта.

#include <string>
#include "common.h"
#include "errors.h"
#include <boost/program_options.hpp>

namespace geoimgconv
{

class AppConfig
{
public:
	//Конструктор. helpLineLength если равно 0 то ширина генерируемой справки остаётся на усмотрение
	//объекта, если же там некое число колонок - то ширина справки будет ему соответствовать.
	AppConfig(unsigned short helpLineLength = 0);
	//Деструктор
	~AppConfig();
	
	//Методы доступа.
	
	//inputFileName;
	std::string const& getInputFileName() const
	{
		if (inputFileNameCmdIsSet_)
			return inputFileNameCmd_;
		else return inputFileNameCfg_;
	}
	std::string const& getInputFileNameCfg() const
		{return inputFileNameCfg_;}
	bool const& getInputFileNameIsSaving() const
		{return inputFileNameCfgIsSaving_;}
	void setInputFileName(const std::string &value)
	{
		inputFileNameCfg_ = value;
		inputFileNameCfgIsSaving_ = true;
		inputFileNameCmdIsSet_ = false;
	}
	
	//outputFileName;
	std::string const& getOutputFileName() const
	{
		if (outputFileNameCmdIsSet_)
			return outputFileNameCmd_;
		else return outputFileNameCfg_;
	}
	std::string const& getOutputFileNameCfg() const
		{return outputFileNameCfg_;}
	bool const& getOutputFileNameIsSaving() const
		{return outputFileNameCfgIsSaving_;}
	void setOutputFileName(const std::string &value)
	{
		outputFileNameCfg_ = value;
		outputFileNameCfgIsSaving_ = true;
		outputFileNameCmdIsSet_ = false;
	};
	
	//medfilterAperture;
	int const& getMedfilterAperture() const
	{
		if (medfilterApertureCmdIsSet_)
			return medfilterApertureCmd_;
		else return medfilterApertureCfg_;
	};
	int const& getMedfilterApertureCfg() const
		{return medfilterApertureCfg_;};
	bool const& getMedfilterApertureIsSaving() const
		{return medfilterApertureCfgIsSaving_;};
	void setMedfilterAperture(const int &value)
	{
		medfilterApertureCfg_ = value;
		medfilterApertureCfgIsSaving_ = true;
		medfilterApertureCmdIsSet_ = false;
	};
	
	//medfilterThreshold;
	double const& getMedfilterThreshold() const
	{
		if (medfilterThresholdCmdIsSet_)
			return medfilterThresholdCmd_;
		else return medfilterThresholdCfg_;
	};
	double const& getMedfilterThresholdCfg() const
		{return medfilterThresholdCfg_;};
	bool const& getMedfilterThresholdIsSaving() const
		{return medfilterThresholdCfgIsSaving_;};
	void setMedfilterThreshold(const double &value)
	{
		medfilterThresholdCfg_ = value;
		medfilterThresholdCfgIsSaving_ = true;
		medfilterThresholdCmdIsSet_ = false;
	};
	
	//medfilterMarginType
	MarginType const& getMedfilterMarginType() const
	{
		if (medfilterMarginTypeCmdIsSet_)
			return medfilterMarginTypeCmd_;
		else return medfilterMarginTypeCfg_;
	};
	MarginType const& getMedfilterMarginTypeCfg() const
		{return medfilterMarginTypeCfg_;};
	bool const& getMedfilterMarginTypeIsSaving() const
		{return medfilterMarginTypeCfgIsSaving_;};
	void setMedfilterMarginType(const MarginType &value)
	{
		medfilterMarginTypeCfg_ = value;
		medfilterMarginTypeCfgIsSaving_ = true;
		medfilterMarginTypeCmdIsSet_ = false;
	};

	//medfilterAlgo
	MedfilterAlgo const& getMedfilterAlgo() const
	{
		if (medfilterAlgoCmdIsSet_)
			return medfilterAlgoCmd_;
		else return medfilterAlgoCfg_;
	};
	MedfilterAlgo const& getMedfilterAlgoCfg() const
	{
		return medfilterAlgoCfg_;
	};
	bool const& getMedfilterAlgoIsSaving() const
	{
		return medfilterAlgoCfgIsSaving_;
	};
	void setMedfilterAlgo(const MedfilterAlgo &value)
	{
		medfilterAlgoCfg_ = value;
		medfilterAlgoCfgIsSaving_ = true;
		medfilterAlgoCmdIsSet_ = false;
	};

	//medfilterHuangLevelsNum
	boost::uint16_t const& getMedfilterHuangLevelsNum() const
	{
		if (medfilterHuangLevelsNumCmdIsSet_)
			return medfilterHuangLevelsNumCmd_;
		else return medfilterHuangLevelsNumCfg_;
	};
	boost::uint16_t const& getMedfilterHuangLevelsNumCfg() const
	{
		return medfilterHuangLevelsNumCfg_;
	};
	bool const& getMedfilterHuangLevelsNumIsSaving() const
	{
		return medfilterHuangLevelsNumCfgIsSaving_;
	};
	void setMedfilterHuangLevelsNum(const boost::uint16_t &value)
	{
		medfilterHuangLevelsNumCfg_ = value;
		medfilterHuangLevelsNumCfgIsSaving_ = true;
		medfilterHuangLevelsNumCmdIsSet_ = false;
	};

	//medfilterFillPits
	bool const& getMedfilterFillPits() const
	{
		if (medfilterFillPitsCmdIsSet_)
			return medfilterFillPitsCmd_;
		else return medfilterFillPitsCfg_;
	};
	bool const& getMedfilterFillPitsCfg() const
	{
		return medfilterFillPitsCfg_;
	};
	bool const& getMedfilterFillPitsIsSaving() const
	{
		return medfilterFillPitsCfgIsSaving_;
	};
	void setMedfilterFillPits(const bool &value)
	{
		medfilterFillPitsCfg_ = value;
		medfilterFillPitsCfgIsSaving_ = true;
		medfilterFillPitsCmdIsSet_ = false;
	};
	
	//appMode
	AppMode const& getAppMode() const
	{
		if (appModeCmdIsSet_)
			return appModeCmd_;
		else return appModeCfg_;
	};
	AppMode const& getAppModeCfg() const
		{return appModeCfg_;};
	bool const& getAppModeIsSaving() const
		{return appModeCfgIsSaving_;};
	void setAppMode(const AppMode &value)
	{
		appModeCfg_ = value;
		appModeCfgIsSaving_ = true;
		appModeCmdIsSet_ = false;
	};

	//memMode и memSize
	MemoryMode const& getMemMode() const
	{
		if (memModeCmdIsSet_)
			return memModeCmd_;
		else return memModeCfg_;
	};
	unsigned long long const& getMemSize() const
	{
		if (memModeCmdIsSet_)
			return memSizeCmd_;
		else return memSizeCfg_;
	};
	MemoryMode const& getMemModeCfg() const
	{
		return memModeCfg_;
	};
	unsigned long long const& getMemSizeCfg() const
	{
		return memSizeCfg_;
	};
	bool const& getMemModeIsSaving() const
	{
		return memModeCfgIsSaving_;
	};
	void setMemMode(const MemoryMode &value, const unsigned long long &memSize)
	{
		memModeCfg_ = value;
		memSizeCfg_ = memSize;
		memModeCfgIsSaving_ = true;
		memModeCmdIsSet_ = false;
	};
	void setMemModeCmd(const MemoryMode &value, const unsigned long long &memSize)
	{
		//Этот параметр может меняться в процессе работы так, как будто он был таким
		//установлен из командной строки (т.е. имеет приоритет и не потребует сохранения).
		memModeCmd_ = value;
		memSizeCmd_ = memSize;
		memModeCmdIsSet_ = true;
	}
	
	//helpAsked
	bool const& getHelpAsked() const {return helpAsked_;};
	//versionAsked
	bool const& getVersionAsked() const {return versionAsked_;};
	//argc
	int const& getArgc() const {return argc_;};
	//argv
	char const* const* getArgv() const {return argv_;};
	//appPath
	std::string const& getAppPath() const {return appPath_;};
	//currPath
	std::string const& getCurrPath() const {return currPath_;};
	//helpMsg (генерируется)
	const std::string getHelpMsg();
	//helpLineLength
	void setHelpLineLength(const unsigned short &helpLineLength)
	{
		helpLineLength_ = helpLineLength;
		if (helpParamsDesc_)
		{
			delete helpParamsDesc_;
			FillDependentPO_();
		}			
	}
	unsigned short const& getHelpLineLength() { return helpLineLength_; }
	
	//Остальные методы, т.е. чтение и запись настроек.
	
	//Прочитать параметры командной строки.
	bool ParseCommandLine(const int &argc, char **argv, ErrorInfo *errObj = NULL);
	//Прочитать ini-файл с настройками. Файл ищется по стандартным местам
	//где он может быть и имеет стандартное для программы имя.
	bool ReadConfigFile(ErrorInfo *errObj = NULL);
	//Завершить инициализацию объекта (отдельно от конструктора т.к. для правильной инициализации
	//могло быть необходимо сначала обработать конфиги и командную строку)
	void FinishInitialization() { FillDependentPO_(); };

private:
	//Приватные поля. Доступ по сеттерам-геттерам. Одно и то же поле может иметь более 1
	//значения - например одно прочитанное из конфига, второе из командной строки.
	//
	//В Cfg-полях содержатся значения (по умолчанию) даже если они не были заданы в конфиге.
	//Из этих полей значения берутся всегда за исключением случаев когда Cmd-поля также
	//определены. bool-поле отвечает только за то, будет ли поле сохраняться в конфиг
	//(сохраняется если оно было оттуда прочитано, либо явно задано юзером во время работы
	//программы). В Cmd-полях содержатся значения, заданные через командную строку. Они не
	//сохраняются в конфиг, но имеют приоритет над Cfg-полями при чтении. Первое же
	//изменение поля делает Cmd-поле незаданным а Cfg-поле сохраняемым.

	//Входной файл
	std::string inputFileNameCfg_ = DEFAULT_INPUT_FILE_NAME;
	std::string inputFileNameCmd_;
	bool inputFileNameCfgIsSaving_ = false;
	bool inputFileNameCmdIsSet_ = false;

	//Выходной файл
	std::string outputFileNameCfg_ = DEFAULT_OUTPUT_FILE_NAME;
	std::string outputFileNameCmd_;
	bool outputFileNameCfgIsSaving_ = false;
	bool outputFileNameCmdIsSet_ = false;

	//Длина стороны окна медианного фильтра.
	int medfilterApertureCfg_ = DEFAULT_MEDFILTER_APERTURE;
	int medfilterApertureCmd_;
	bool medfilterApertureCfgIsSaving_ = false;
	bool medfilterApertureCmdIsSet_ = false;

	//Порог медианного фильтра.
	double medfilterThresholdCfg_ = DEFAULT_MEDFILTER_THRESHOLD;
	double medfilterThresholdCmd_;
	bool medfilterThresholdCfgIsSaving_ = false;
	bool medfilterThresholdCmdIsSet_ = false;

	//Тип заполнителя краевых областей для медианного фильтра.
	MarginType medfilterMarginTypeCfg_ = DEFAULT_MEDFILTER_MARGIN_TYPE;
	MarginType medfilterMarginTypeCmd_;
	bool medfilterMarginTypeCfgIsSaving_ = false;
	bool medfilterMarginTypeCmdIsSet_ = false;

	//Режим работы медианного фильтра.
	MedfilterAlgo medfilterAlgoCfg_ = DEFAULT_MEDFILTER_ALGO;
	MedfilterAlgo medfilterAlgoCmd_;
	bool medfilterAlgoCfgIsSaving_ = false;
	bool medfilterAlgoCmdIsSet_ = false;

	//Количество уровней квантования для алгоритма Хуанга (медианный фильтр).
	boost::uint16_t medfilterHuangLevelsNumCfg_ = DEFAULT_HUANG_LEVELS_NUM;
	boost::uint16_t medfilterHuangLevelsNumCmd_;
	bool medfilterHuangLevelsNumCfgIsSaving_ = false;
	bool medfilterHuangLevelsNumCmdIsSet_ = false;

	//Заполняет ли медианный фильтр ямы.
	bool medfilterFillPitsCfg_ = DEFAULT_MEDFILTER_FILL_PITS;
	bool medfilterFillPitsCmd_;
	bool medfilterFillPitsCfgIsSaving_ = false;
	bool medfilterFillPitsCmdIsSet_ = false;

	//Режим работы программы.
	AppMode appModeCfg_ = DEFAULT_APP_MODE;
	AppMode appModeCmd_;
	bool appModeCfgIsSaving_ = false;
	bool appModeCmdIsSet_ = false;

	//Режим использования памяти.
	MemoryMode memModeCfg_ = DEFAULT_MEM_MODE;
	MemoryMode memModeCmd_;
	unsigned long long memSizeCfg_ = 0;
	unsigned long long memSizeCmd_;
	bool memModeCfgIsSaving_ = false;
	bool memModeCmdIsSet_ = false;

	//Остальные поля (не поля конфига).
	bool helpAsked_ = false;	//Была ли в командной строке запрошена справка.
	bool versionAsked_ = false;	//Была ли в командной строке запрошена версия.
	int argc_ = 0;	//argc и argv запоминаются при разборе командной строки.
	char **argv_ = nullptr;	//--"--
	std::string appPath_ = "";	//Путь к исполнимому бинарнику и теущий путь.
	std::string currPath_ = "";	//--"--
	unsigned short helpLineLength_ = 0;	//Ширина генерируемой справки.

	//Объекты для вывода справки и чтения\записи настроек.
	//Инициализируются частично в конструкторе и частично отдельным
	//методом (который зависит от настроенных в STB кодировок).
	boost::program_options::options_description cmdLineParamsDesc_;
	//По указателю т.к. создавать объект в конструкторе нельзя, только позже, когда будет известна кодировка.
	boost::program_options::options_description *helpParamsDesc_ = nullptr;
	boost::program_options::options_description configParamsDesc_;
	boost::program_options::positional_options_description positionalDesc_;
	boost::program_options::variables_map poVarMap_;

	//Приватные методы

	//Заполнить заполнябельные ещё до разбора командной строки объекты
	//program_options. Вызывается конструктором.
	void FillBasePO_();
	//Заполнить остальные объекты program_options, которые могут быть заполнены
	//только после чтения параметров командной строки и конфига.
	void FillDependentPO_();
	//Прочитать ini-файл по указанному пути. Файл должен существовать.
	bool ReadConfigFile_(const std::string &filePath, ErrorInfo *errObj = NULL);
	//Получить MemoryMode из строки, совпадающей без учёта регистра с одним из
	//элементов MemoryModeTexts + прочитать и правильно интерпретировать
	//указанный там размер.
	void ParseMemoryModeStr(const std::string &inputStr, MemoryMode &memMode, unsigned long long &size);
};

} //namespace geoimgconv