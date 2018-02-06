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

//Небольшой сборник функций общего назначения, которые пока непонятно куда засунуть.
//В основном - разнообразная работа с консолью, кодировками, файловой системой.
//Оформлено в виде класса с методами.
//Многое написано ближе к стилю С чем С++ - голые указатели, char* и всё такое.
//Не пинайте погромиста, он погромляет как умеет :)).

#include "small_tools_box.h"
#include <string>
#include <locale>
#include <boost/locale.hpp>
#include <sstream>
#include <iomanip>
#include <boost/filesystem.hpp>
#include <boost/thread/thread.hpp>

//Для определения количества памяти нужен платформозависимый код :(
#ifdef _WIN32
	#include <Windows.h>
#elif defined(__linux__)
    //Тут придётся парсить /proc/meminfo для чего нужны будут потоки и tokenizer
    #include <boost/tokenizer.hpp>
    #include <boost/lexical_cast.hpp>
    #include <iostream>
    #include <sstream>
#elif defined(unix) || defined(__unix__) || defined(__unix)
	#include <sys/types.h>
	#include <unistd.h>
	#include <sys/param.h>
	#if (!defined(_SC_PAGE_SIZE) && defined(_SC_PAGESIZE))
		#define _SC_PAGE_SIZE = _SC_PAGESIZE
	#elif !defined(_SC_PAGE_SIZE)
		#define _SC_PAGE_SIZE = 1024L
	#endif
#elif !defined(__linux__)
	#error Unknown target OS. Cant preprocess memory detecting code :(
#endif

using namespace std;

namespace geoimgconv
{

SmallToolsBox STB;

SmallToolsBox::SmallToolsBox() : encodingsInited_(false),
	consoleEncodingIsSelected_(true)
{
}

SmallToolsBox::~SmallToolsBox()
{
}

void SmallToolsBox::InitEncodings()
//Переключает глобальную локаль на текущую, в которой запущено приложение.\
//Узнаёт и запоминает кодировки - в консоли и системную. Желательно вызвать
//до обращения к функциям перекодировки.
{
	boost::locale::generator locGen;
	#ifdef _WIN32
		//Пока что всё будет чисто русскоязычным (пока руки не дойдут до чего-нибудь
		//типа gettext, поэтому жёстко проставим кодировки.
		this->consoleEncoding_ = "cp866";
		this->systemEncoding_ = "cp1251";
		//Вообще там ниже через фасет locale::info можно как минимум получить код языка,
		//но потом, всё потом. TODO типа.
	#else
		//Линукс и всякое неизвестно-что.
		setlocale(LC_ALL, "");	//Нужно только тут. Чтобы работало ncursesw. Для PDCurses - наоборот вредно.
		locale currLocale = locGen("");
		this->consoleEncoding_ = std::use_facet<boost::locale::info>(currLocale).encoding();
		this->systemEncoding_ = consoleEncoding_;
	#endif

	//Сгенерируем локаль для работы с utf8
	this->utf8Locale_ = locGen("utf8");

	//Готово.
	this->encodingsInited_ = true;
}

string SmallToolsBox::Utf8ToConsoleCharset(const string &InputStr)
//Перекодирует строку из utf8 в кодировку, подходящую для вывода в консоль.
{
	if (!this->encodingsInited_)
		this->InitEncodings();
	return boost::locale::conv::from_utf(InputStr, this->consoleEncoding_);
}

string SmallToolsBox::ConsoleCharsetToUtf8(const string &InputStr)
//Перекодирует строку из кодировки консоли в utf8
{
	if (!this->encodingsInited_)
		this->InitEncodings();
	return boost::locale::conv::to_utf<char>(InputStr, this->consoleEncoding_);
}


string SmallToolsBox::Utf8ToSystemCharset(const string &InputStr)
//Перекодирует строку из utf8 в системную кодировку.
{
	if (!this->encodingsInited_)
		this->InitEncodings();
	return boost::locale::conv::from_utf(InputStr, this->systemEncoding_);
}


string SmallToolsBox::SystemCharsetToUtf8(const string &InputStr)
//Перекодирует строку из системной кодировки в utf8
{
	if (!this->encodingsInited_)
		this->InitEncodings();
	return boost::locale::conv::to_utf<char>(InputStr, this->systemEncoding_);
}


string SmallToolsBox::Utf8ToSelectedCharset(const string &InputStr)
//Перекодирует строку из utf8 в кодировку, выбранную ранее методами SelectCharset*.
{
	if (this->consoleEncodingIsSelected_)
		return this->Utf8ToConsoleCharset(InputStr);
	else
		return this->Utf8ToSystemCharset(InputStr);
}


string SmallToolsBox::SelectedCharsetToUtf8(const string &InputStr)
//Перекодирует строку из кодировки, выбранной ранее методами SelectCharset* в utf8
{
	if (this->consoleEncodingIsSelected_)
		return this->ConsoleCharsetToUtf8(InputStr);
	else
		return this->SystemCharsetToUtf8(InputStr);
}

std::string SmallToolsBox::WstringToUtf8(const std::wstring &inputStr)
//Перекодирует wstring в string в кодировке utf8
{
	if (!this->encodingsInited_)
		this->InitEncodings();
	return boost::locale::conv::utf_to_utf<char,wchar_t>(inputStr);
}

std::wstring SmallToolsBox::Utf8ToWstring(const std::string &inputStr)
//Перекодирует string в кодировке utf8 в wstring
{
	if (!this->encodingsInited_)
		this->InitEncodings();
	return boost::locale::conv::utf_to_utf<wchar_t,char>(inputStr);
}


std::string SmallToolsBox::DoubleToString(const double &input,
	const unsigned int &precision) const
//Преобразует double в строку с указанным количеством знаков после запятой.
{
	std::ostringstream stringStream;
    stringStream << std::fixed << std::setprecision(precision) << input;
    return stringStream.str();
}


void SmallToolsBox::Utf8ToLower(const std::string &inputStr, std::string &outputStr) const
//Перевести в нижний регистр utf8-строку.
{
	outputStr = boost::locale::to_lower(inputStr, this->utf8Locale_);
}


void SmallToolsBox::Utf8ToUpper(const std::string &inputStr, std::string &outputStr) const
//Перевести в верхний регистр utf8-строку.
{
	outputStr = boost::locale::to_upper(inputStr, this->utf8Locale_);
}

const unsigned int SmallToolsBox::GetCpuCoresNumber() const
//Возвращает число процессорных ядер или 0 если это количество получить не удалось.
{
	//Буст это умеет, нет смысла извращаться вручную.
	return boost::thread::hardware_concurrency();
}

#ifdef __linux__
const unsigned long long GetProcMeminfoField(const std::string &fieldName)
//Парсим /proc/meminfo и вынимаем нужное нам значение. При ошибке вернём 0.
{
    using namespace boost;
    unsigned long long result = 0;
    //Открываем файл.
    ifstream ifile("/proc/meminfo");
    if (!ifile)
    {
        return 0;
    }

    //Вытягиваем всё в строку.
    stringstream sstream;
    sstream << ifile.rdbuf();
    string fileContents(sstream.str());
    ifile.close();

    //Готовим токенайзеры.
    char_separator<char> linesSep("\n"); //Делить файл на строки
    char_separator<char> tokensSep(" \t:");  //Делить строки на поля
    tokenizer<char_separator<char> > linesTok(fileContents, linesSep);

    //Ковыряем содержимое.
    for (tokenizer<char_separator<char> >::const_iterator i = linesTok.begin();
        i !=linesTok.end();i++)
    {
        tokenizer<char_separator<char> >currLineTok(*i, tokensSep);
        tokenizer<char_separator<char> >::const_iterator j = currLineTok.begin();
        if (j!=currLineTok.end())
        {
            if(*j == fieldName)
            {
                //Совпадает имя поля.
                j++;
                if (j!=currLineTok.end())
                {
                    //Есть второе поле - это будет число, его и вернём умножив на размер килобайта.
                    return lexical_cast<unsigned long long>(*j) * 1024;
                }
                else
                {
                    //Нет смысла дальше что-то искать
                    return 0;
                }
            }
        }
    }

    //Независимо от того нашли ли что-нибудь - вернём результат. Если не нашли будет 0.
    return result;
}
#endif // __linux__

const unsigned long long SmallToolsBox::GetSystemMemoryFullSize() const
//Возвращает общее количество оперативной памяти (без свопа) в системе или 0 при ошибке.
{
	unsigned long long result;
	#ifdef _WIN32
		MEMORYSTATUSEX statex;
		statex.dwLength = sizeof(statex);
		if (GlobalMemoryStatusEx(&statex))
		{
			//Вытягиваем инфу
			result = statex.ullTotalPhys;
			if (result == -1) result = 0;
		}
		else
		{
			//Что-то пошло не так :(
			result = 0;
		};
	#elif defined(__linux__)
		//Тут нужно расковыривать содержимое /proc/meminfo
		result = GetProcMeminfoField("MemTotal");
	#elif (defined(unix) || defined(__unix__) || defined(__unix)) && (defined(_SC_PAGE_SIZE) && defined(_SC_PHYS_PAGES))
		long pagesize = sysconf(_SC_PAGE_SIZE);
		long pages = sysconf(_SC_PHYS_PAGES);
		if ((pagesize != -1) && (pages != -1))
		{
			result = pagesize * pages;
		}
		else
			result = 0;
	#else
		#error Unknown target OS. Cant preprocess memory detecting code :(
	#endif
	return result;
}


const unsigned long long SmallToolsBox::GetSystemMemoryFreeSize() const
//Возвращает количество свободной оперативной памяти (без свопа) в системе или 0 при ошибке.
{
	unsigned long long result;
	#ifdef _WIN32
	MEMORYSTATUSEX statex;
	statex.dwLength = sizeof(statex);
	if (GlobalMemoryStatusEx(&statex))
	{
		//Вытягиваем инфу
		result = statex.ullAvailPhys;
		if (result == -1) result = 0;
	}
	else
	{
		//Что-то пошло не так :(
		result = 0;
	};
	#elif defined(__linux__)
		//Тут нужно расковыривать содержимое /proc/meminfo
		result = GetProcMeminfoField("MemAvailable");
	#elif (defined(unix) || defined(__unix__) || defined(__unix)) && (defined(_SC_PAGE_SIZE) && defined(_SC_AVPHYS_PAGES))
		long pagesize = sysconf(_SC_PAGE_SIZE);
		long pages = sysconf(_SC_AVPHYS_PAGES);
		if ((pagesize != -1) && (pages != -1))
		{
			result = pagesize * pages;
		}
		else
			result = 0;
	#else
		#error Unknown target OS. Cant preprocess memory detecting code :(
	#endif
	return result;
}

}	//namespace geoimgconv
