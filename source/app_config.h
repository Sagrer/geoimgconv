#pragma once

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
	std::string inputFileNameCfg_;
	std::string inputFileNameCmd_;
	bool inputFileNameCfgIsSaving_;
	bool inputFileNameCmdIsSet_;
	
	//Выходной файл
	std::string outputFileNameCfg_;
	std::string outputFileNameCmd_;
	bool outputFileNameCfgIsSaving_;
	bool outputFileNameCmdIsSet_;
	
	//Длина стороны окна медианного фильтра.
	int medfilterApertureCfg_;
	int medfilterApertureCmd_;
	bool medfilterApertureCfgIsSaving_;
	bool medfilterApertureCmdIsSet_;
	
	//Порог медианного фильтра.
	double medfilterThresholdCfg_;
	double medfilterThresholdCmd_;
	bool medfilterThresholdCfgIsSaving_;
	bool medfilterThresholdCmdIsSet_;	
	
	//Тип заполнителя краевых областей для медианного фильтра.
	MarginType medfilterMarginTypeCfg_;
	MarginType medfilterMarginTypeCmd_;
	bool medfilterMarginTypeCfgIsSaving_;
	bool medfilterMarginTypeCmdIsSet_;
	
	//Режим работы программы.
	AppMode appModeCfg_;
	AppMode appModeCmd_;
	bool appModeCfgIsSaving_;
	bool appModeCmdIsSet_;
	
	//Остальные поля (не поля конфига).
	bool helpAsked_;	//Была ли в командной строке запрошена справка.
	bool versionAsked_;	//Была ли в командной строке запрошена версия.
	int argc_;	//argc и argv запоминаются при разборе командной строки.
	char **argv_;	//--"--
	std::string appPath_;	//Путь к исполнимому бинарнику и теущий путь.
	std::string currPath_;	//--"--
	
	//Объекты для вывода справки и чтения\записи настроек.
	//Инициализируются частично в конструкторе и частично отдельным
	//методом (который зависит от настроенных в STB кодировок).
	boost::program_options::options_description cmdLineParamsDesc_;
	boost::program_options::options_description *helpParamsDesc_;	//По указателю т.к. создавать объект в конструкторе нельзя, только позже, когда будет известна кодировка.
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
public:
	//Конструкторы-деструкторы
	AppConfig();
	~AppConfig();
	
	//Методы доступа.
	
	//inputFileName;
	std::string const& getInputFileName() const
	{
		if (this->inputFileNameCmdIsSet_)
			return this->inputFileNameCmd_;
		else return this->inputFileNameCfg_;
	}
	std::string const& getInputFileNameCfg() const
		{return this->inputFileNameCfg_;}
	bool const& getInputFileNameIsSaving() const
		{return this->inputFileNameCfgIsSaving_;}
	void setInputFileName(const std::string &value)
	{
		this->inputFileNameCfg_ = value;
		this->inputFileNameCfgIsSaving_ = true;
		this->inputFileNameCmdIsSet_ = false;
	}
	
	//outputFileName;
	std::string const& getOutputFileName() const
	{
		if (this->outputFileNameCmdIsSet_)
			return this->outputFileNameCmd_;
		else return this->outputFileNameCfg_;
	}
	std::string const& getOutputFileNameCfg() const
		{return this->outputFileNameCfg_;}
	bool const& getOutputFileNameIsSaving() const
		{return this->outputFileNameCfgIsSaving_;}
	void setOutputFileName(const std::string &value)
	{
		this->outputFileNameCfg_ = value;
		this->outputFileNameCfgIsSaving_ = true;
		this->outputFileNameCmdIsSet_ = false;
	};
	
	//medfilterAperture;
	int const& getMedfilterAperture() const
	{
		if (this->medfilterApertureCmdIsSet_)
			return this->medfilterApertureCmd_;
		else return this->medfilterApertureCfg_;
	};
	int const& getMedfilterApertureCfg() const
		{return this->medfilterApertureCfg_;};
	bool const& getMedfilterApertureIsSaving() const
		{return this->medfilterApertureCfgIsSaving_;};
	void setMedfilterAperture(const int &value)
	{
		this->medfilterApertureCfg_ = value;
		this->medfilterApertureCfgIsSaving_ = true;
		this->medfilterApertureCmdIsSet_ = false;
	};
	
	//medfilterThreshold;
	double const& getMedfilterThreshold() const
	{
		if (this->medfilterThresholdCmdIsSet_)
			return this->medfilterThresholdCmd_;
		else return this->medfilterThresholdCfg_;
	};
	double const& getMedfilterThresholdCfg() const
		{return this->medfilterThresholdCfg_;};
	bool const& getMedfilterThresholdIsSaving() const
		{return this->medfilterThresholdCfgIsSaving_;};
	void setMedfilterThreshold(const double &value)
	{
		this->medfilterThresholdCfg_ = value;
		this->medfilterThresholdCfgIsSaving_ = true;
		this->medfilterThresholdCmdIsSet_ = false;
	};
	
	//medfilterMarginType
	MarginType const& getMedfilterMarginType() const
	{
		if (this->medfilterMarginTypeCmdIsSet_)
			return this->medfilterMarginTypeCmd_;
		else return this->medfilterMarginTypeCfg_;
	};
	MarginType const& getMedfilterMarginTypeCfg() const
		{return this->medfilterMarginTypeCfg_;};
	bool const& getMedfilterMarginTypeIsSaving() const
		{return this->medfilterMarginTypeCfgIsSaving_;};
	void setMedfilterMarginType(const MarginType &value)
	{
		this->medfilterMarginTypeCfg_ = value;
		this->medfilterMarginTypeCfgIsSaving_ = true;
		this->medfilterMarginTypeCmdIsSet_ = false;
	};
	
	//appMode
	AppMode const& getAppMode() const
	{
		if (this->appModeCmdIsSet_)
			return this->appModeCmd_;
		else return this->appModeCfg_;
	};
	AppMode const& getAppModeCfg() const
		{return this->appModeCfg_;};
	bool const& getAppModeIsSaving() const
		{return this->appModeCfgIsSaving_;};
	void setAppMode(const AppMode &value)
	{
		this->appModeCfg_ = value;
		this->appModeCfgIsSaving_ = true;
		this->appModeCmdIsSet_ = false;
	};
	
	//helpAsked
	bool const& getHelpAsked() const {return this->helpAsked_;};
	//versionAsked
	bool const& getVersionAsked() const {return this->versionAsked_;};
	//argc
	int const& getArgc() const {return this->argc_;};
	//argv
	char const* const* getArgv() const {return this->argv_;};
	//appPath
	std::string const& getAppPath() const {return this->appPath_;};
	//currPath
	std::string const& getCurrPath() const {return this->currPath_;};
	//helpMsg (генерируется)
	const std::string getHelpMsg();
	
	//Остальные методы, т.е. чтение и запись настроек.
	
	//Прочитать параметры командной строки.
	bool ParseCommandLine(const int &argc, char **argv, ErrorInfo *errObj = NULL);
	//Прочитать ini-файл с настройками. Файл ищется по стандартным местам
	//где он может быть и имеет стандартное для программы имя.
	bool ReadConfigFile(ErrorInfo *errObj = NULL);
	//Завершить инициализацию объекта (отдельно от конструктора т.к. для правильной инициализации
	//могло быть необходимо сначала обработать конфиги и командную строку)
	void FinishInitialization() { this->FillDependentPO_(); };
};

} //namespace geoimgconv