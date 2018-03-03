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
#include <boost/lexical_cast.hpp>
#include <boost/predef.h>
#include <cctype>

//Для определения количества памяти нужен платформозависимый код :(
#ifdef _WIN32
	#include <Windows.h>
#elif defined(unix) || defined(__unix__) || defined(__unix)
	#include <sys/types.h>
	#include <unistd.h>
	#include <sys/param.h>
	#include <sys/time.h>
	#include <sys/resource.h>
	//В разных операционках константа с размером страницы может называться по разному или
	//может вообще отсутствовать. Приводим всё к единому знаменателю - _SC_PAGE_SIZE.
	#if (!defined(_SC_PAGE_SIZE) && defined(_SC_PAGESIZE))
		#define _SC_PAGE_SIZE = _SC_PAGESIZE
	#elif !defined(_SC_PAGE_SIZE)
		#define _SC_PAGE_SIZE = 1024L
	#endif

	//Для linux кое-что придётся делать иначе.
	#if defined(__linux__)
		//Тут придётся парсить /proc/meminfo для чего нужны будут потоки и tokenizer
		#include <boost/tokenizer.hpp>
		#include <iostream>
		#include <sstream>
	#endif
#else
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

//Переключает глобальную локаль на текущую, в которой запущено приложение.\
//Узнаёт и запоминает кодировки - в консоли и системную. Желательно вызвать
//до обращения к функциям перекодировки.
void SmallToolsBox::InitEncodings()
{
	boost::locale::generator locGen;
	#ifdef _WIN32
		//Пока что всё будет чисто русскоязычным (пока руки не дойдут до чего-нибудь
		//типа gettext, поэтому жёстко проставим кодировки.
		consoleEncoding_ = "cp866";
		systemEncoding_ = "cp1251";
		//Вообще там ниже через фасет locale::info можно как минимум получить код языка,
		//но потом, всё потом. TODO типа.
	#else
		//Линукс и всякое неизвестно-что.
		setlocale(LC_ALL, "");	//Нужно только тут. Чтобы работало ncursesw. Для PDCurses - наоборот вредно.
		locale currLocale = locGen("");
		consoleEncoding_ = std::use_facet<boost::locale::info>(currLocale).encoding();
		systemEncoding_ = consoleEncoding_;
	#endif

	//Сгенерируем локаль для работы с utf8
	utf8Locale_ = locGen("utf8");

	//Готово.
	encodingsInited_ = true;
}

//Перекодирует строку из utf8 в кодировку, подходящую для вывода в консоль.
string SmallToolsBox::Utf8ToConsoleCharset(const string &InputStr)
{
	if (!encodingsInited_)
		InitEncodings();
	return boost::locale::conv::from_utf(InputStr, consoleEncoding_);
}

//Перекодирует строку из кодировки консоли в utf8
string SmallToolsBox::ConsoleCharsetToUtf8(const string &InputStr)
{
	if (!encodingsInited_)
		InitEncodings();
	return boost::locale::conv::to_utf<char>(InputStr, consoleEncoding_);
}

//Перекодирует строку из utf8 в системную кодировку.
string SmallToolsBox::Utf8ToSystemCharset(const string &InputStr)
{
	if (!encodingsInited_)
		InitEncodings();
	return boost::locale::conv::from_utf(InputStr, systemEncoding_);
}

//Перекодирует строку из системной кодировки в utf8
string SmallToolsBox::SystemCharsetToUtf8(const string &InputStr)
{
	if (!encodingsInited_)
		InitEncodings();
	return boost::locale::conv::to_utf<char>(InputStr, systemEncoding_);
}

//Перекодирует строку из utf8 в кодировку, выбранную ранее методами SelectCharset*.
string SmallToolsBox::Utf8ToSelectedCharset(const string &InputStr)
{
	if (consoleEncodingIsSelected_)
		return Utf8ToConsoleCharset(InputStr);
	else
		return Utf8ToSystemCharset(InputStr);
}

//Перекодирует строку из кодировки, выбранной ранее методами SelectCharset* в utf8
string SmallToolsBox::SelectedCharsetToUtf8(const string &InputStr)
{
	if (consoleEncodingIsSelected_)
		return ConsoleCharsetToUtf8(InputStr);
	else
		return SystemCharsetToUtf8(InputStr);
}

//Перекодирует wstring в string в кодировке utf8
std::string SmallToolsBox::WstringToUtf8(const std::wstring &inputStr)
{
	if (!encodingsInited_)
		InitEncodings();
	return boost::locale::conv::utf_to_utf<char,wchar_t>(inputStr);
}

//Перекодирует string в кодировке utf8 в wstring
std::wstring SmallToolsBox::Utf8ToWstring(const std::string &inputStr)
{
	if (!encodingsInited_)
		InitEncodings();
	return boost::locale::conv::utf_to_utf<wchar_t,char>(inputStr);
}

//Преобразует double в строку с указанным количеством знаков после запятой.
std::string SmallToolsBox::DoubleToString(const double &input,
	const unsigned int &precision) const
{
	std::ostringstream stringStream;
	stringStream << std::fixed << std::setprecision(precision) << input;
	return stringStream.str();
}

//Преобразует bool в строку. Потоки или lexical_cast имхо лишнее.
std::string SmallToolsBox::BoolToString(const bool &input) const
{
	return input ? "true" : "false";
}

//Преобразовать целое число в строку фиксированного размера с заданным количеством
//нулей в начале.
std::string SmallToolsBox::IntToString(const long long value, int width)
{
	std::stringstream sStream;
	sStream << std::setw(width) << std::setfill('0') << value;
	return sStream.str();
}

//Перевести в нижний регистр utf8-строку.
void SmallToolsBox::Utf8ToLower(const std::string &inputStr, std::string &outputStr) const
{
	outputStr = boost::locale::to_lower(inputStr, utf8Locale_);
}

//Перевести в верхний регистр utf8-строку.
void SmallToolsBox::Utf8ToUpper(const std::string &inputStr, std::string &outputStr) const
{
	outputStr = boost::locale::to_upper(inputStr, utf8Locale_);
}

//Преобразовать количество байт в удобную для юзера строку с мегабайтами-гигабайтами.
const std::string SmallToolsBox::BytesNumToInfoSizeStr(const unsigned long long &bytesNum) const
{
	using namespace boost;
	//Если число меньше килобайта - выводим байты, иначе если меньше мегабайта - выводим
	//килобайты, и так далее до терабайтов.
	//TODO: нужна будет какая-то i18n и l10n. Когда-нибудь. В отдалённом светлом будущем.

	if (bytesNum < 1024)
	{
		return lexical_cast<string>(bytesNum) + " байт";
	}
	else if (bytesNum < 1048576)
	{
		return DoubleToString((long double)(bytesNum)/1024.0, 2) + " кб";
	}
	else if (bytesNum < 1073741824)
	{
		return DoubleToString((long double)(bytesNum) / 1048576.0, 2) + " мб";
	}
	else if (bytesNum < 1099511627776)
	{
		return DoubleToString((long double)(bytesNum) / 1073741824.0, 2) + " гб";
	}
	else
	{
		return DoubleToString((long double)(bytesNum) / 1099511627776.0, 2) + " тб";
	}
}

//Прочитать количество информации в байтах из строки. Формат не совпадает с форматом,
//выводимым методом выше. Здесь все символы кроме последнего должны быть беззнаковым
//целым числом, последний же символ может быть B или b - байты, K или k - килобайты,
//M или m - мегабайты, G или g - гигабайты, T или t - терабайты. Если символ не указан
//- применяется символ по умолчанию (второй аргумент, b если не указан).
const unsigned long long SmallToolsBox::InfoSizeToBytesNum(const std::string &inputStr, char defaultUnit) const
{
	unsigned long long result = 0;
	char lastChar;
	string numberStr;
	if (CheckInfoSizeStr(inputStr))
	{
		//Преобразуем строку и при необходимости умножаем значение в зависимости
		//от последнего символа.
		lastChar = inputStr[inputStr.length() - 1];
		if (isdigit(lastChar))
		{
			lastChar = defaultUnit;
			numberStr = inputStr;
		}
		else
			numberStr = inputStr.substr(0, inputStr.length() - 1);
		lastChar = tolower(lastChar);
		result = boost::lexical_cast<unsigned long long>(numberStr);
		if (lastChar == 'k')
			result *= 1024;
		else if (lastChar == 'm')
			result *= 1048576;
		else if (lastChar == 'g')
			result *= 1073741824;
		else if (lastChar == 't')
			result *= 1099511627776;
	};
	
	return result;
}

//Проверить содержится ли в строке целое беззнаковое число.
const bool SmallToolsBox::CheckUnsIntStr(const std::string &inputStr) const
{
	if ((inputStr.length() > 0) && ((isdigit((unsigned char)(inputStr[0]))) || (inputStr[0] == '+')) )
	{
		//^^^ - исключить что это целое число но со знаком.
		istringstream strStream(inputStr);
		unsigned long long testVar;
		//noskipws нужен чтобы всякие пробелы не игнорировались
		strStream >> noskipws >> testVar;
		return strStream && strStream.eof();
	}
	else
		return false;
}

//Проверить содержится ли в строке целое со знаком.
const bool SmallToolsBox::CheckSignedIntStr(const std::string &inputStr) const
{
	istringstream strStream(inputStr);
	signed long long testVar;
	//noskipws нужен чтобы всякие пробелы не игнорировались
	strStream >> noskipws >> testVar;
	return strStream && strStream.eof();
}

//Проверить содержится ли в строке число с плавающей запятой
const bool SmallToolsBox::CheckFloatStr(const std::string &inputStr) const
{
	istringstream strStream(inputStr);
	long double testVar;
	//noskipws нужен чтобы всякие пробелы не игнорировались
	strStream >> noskipws >> testVar;
	return strStream && strStream.eof();
}

//Проверить содержится ли в строке размер чего-либо в байтах (формат тот же, что в
//методе InfoSizeToBytesNum()
const bool SmallToolsBox::CheckInfoSizeStr(const std::string &inputStr) const
{
	//Всё кроме последнего символа должно быть unsigned int.
	string tempStr = inputStr;
	char lastChar = 'b';
	if ((tempStr.length() > 0) && (!isdigit((unsigned char)(tempStr[tempStr.length() - 1]))))
	{
		//Последний символ - не число, запомним его и уменьшим размер строки.
		lastChar = tempStr[tempStr.length() - 1];
		tempStr = tempStr.substr(0, tempStr.length() - 1);
	}
	if (!CheckUnsIntStr(tempStr))
		return false;
	
	//Надо проверить последний символ.
	lastChar = tolower(lastChar);
	if ((lastChar != 'b') && (lastChar != 'k') && (lastChar != 'm') && (lastChar != 'g') &&
		(lastChar != 't'))
		return false;
	else return true;
}

//Возвращает число процессорных ядер или 0 если это количество получить не удалось.
const unsigned int SmallToolsBox::GetCpuCoresNumber() const
{
	//Количество ядер умеет узнавать Boost. C++11 тоже умеет, но духи гугла говорят
	//что буст надёжнее и реже возвращает 0.
	unsigned int result = boost::thread::hardware_concurrency();
	if (!result) result = 1;	//0 ядер не бывает.
	return result;
}

#ifdef __linux__
//Парсим /proc/meminfo и вынимаем нужное нам значение. При ошибке вернём 0.
const unsigned long long GetProcMeminfoField(const std::string &fieldName)
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

	//Итераторы
	tokenizer<char_separator<char> >::const_iterator i;
	tokenizer<char_separator<char> >::const_iterator j;

	//Ковыряем содержимое.
	for (i = linesTok.begin();
		i !=linesTok.end();i++)
	{
		tokenizer<char_separator<char> >currLineTok(*i, tokensSep);
		j = currLineTok.begin();
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

//Парсим /proc/meminfo и вынимаем сразу все значения из тех, что можно записать в SysResInfo.
void FillSysInfoFromProcMeminfo(SysResInfo &infoStruct)
{
	using namespace boost;
	char fieldsNum = 2;	//Счётчик прочитанных полей чтобы не продолжать парсить файл когда вся инфа
						//уже будет прочитана.
	unsigned long long *fieldPointer;	//Указатель на поле в которое будем записывать значение.
	unsigned long long result = 0;
	//Открываем файл.
	ifstream ifile("/proc/meminfo");
	if (!ifile)
	{
		return;
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

	//Итераторы
	tokenizer<char_separator<char> >::const_iterator i;
	tokenizer<char_separator<char> >::const_iterator j;

	//Ковыряем содержимое.
	for (i = linesTok.begin(); i != linesTok.end(); i++)
	{
		tokenizer<char_separator<char> >currLineTok(*i, tokensSep);
		j = currLineTok.begin();
		if (j != currLineTok.end())
		{
			//Вот это шаманство с указателем - чтобы строковое сравнение делать всего 1 раз и при этом
			//не использовать никакого набора констант по которому можно было бы понять какое из полей
			//было найдено. При совпадении строк мы сразу получим указатель на поле куда надо инфу
			//записать.
			fieldPointer = NULL;
			if ((*j == "MemTotal"))
				fieldPointer = &(infoStruct.systemMemoryFullSize);
			else if ((*j == "MemAvailable"))
				fieldPointer = &(infoStruct.systemMemoryFreeSize);
			if (fieldPointer)
			{
				//Было совпадение имя поля.
				j++;
				if (j != currLineTok.end())
				{
					//Есть второе поле - это будет число, его и запомним, умножив на размер килобайта.
					*fieldPointer = lexical_cast<unsigned long long>(*j) * 1024;
					//Если это было последнее поле - нет дальше смысла парсить файл.
					fieldsNum--;
					if (!fieldsNum)
						return;
				}
			}
		}
	}

	return;
}
#endif // __linux__

//Возвращает общее количество оперативной памяти (без свопа) в системе или 0 при ошибке.
const unsigned long long SmallToolsBox::GetSystemMemoryFullSize() const
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

//Возвращает количество свободной оперативной памяти (без свопа) в системе или 0 при ошибке.
const unsigned long long SmallToolsBox::GetSystemMemoryFreeSize() const
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

//Возвращает максимальное количество памяти, которое вообще может потребить данный процесс.
const unsigned long long SmallToolsBox::GetMaxProcessMemorySize() const
{
	unsigned long long result = 0;
	#ifdef _WIN32
		MEMORYSTATUSEX statex;
		statex.dwLength = sizeof(statex);
		if (GlobalMemoryStatusEx(&statex))
		{
			//Вытягиваем инфу
			result = statex.ullTotalVirtual;
			if (result == -1) result = 0;
		}
		else
		{
			//Что-то пошло не так :(
			result = 0;
		};
	#elif (defined(unix) || defined(__unix__) || defined(__unix))
		//По идее getrlimit должен быть в любом unix  с поддержкой POSIX 2001
		rlimit infoStruct;
		if (!getrlimit(RLIMIT_DATA, &infoStruct))
		{
			result = infoStruct.rlim_max;
			//Если там -1 то это не ошибка а отсутствие лимита. Уменьшим значение
			//чтобы где-нибудь его не сравнили с -1 и не посчитали за ошибку.
			if (result == -1)
				result--;
		}
		else
		{
			//Что-то пошло не так.
			result = 0;
		}
	#elif
		#error Unknown target OS. Cant preprocess memory detecting code :(
	#endif

	//Если у нас i386 32-битная или 64-битная архитектура - можно попробовать жёстко
	//задать максимальный лимит (если до сих пор ничего найти не удалось).
	#if BOOST_ARCH_X86_32
		if (result == 0)
			result = 2147483648;	//2 гигабайта. Больше система может и не уметь.
	#elif BOOST_ARCH_X86_64
		if (result == 0)
			result = 8796093022208;	//8 терабайт. Больше система может и не уметь.
	#endif

	return result;
}

//Вернуть информацию о ресурсах системы - то же, что и несколько методов выше, но должно
//работать быстрее чем последовательный вызов всех этих методов.
void SmallToolsBox::GetSysResInfo(SysResInfo &infoStruct) const
{
	//Количество ядер умеет узнавать Boost. C++11 тоже умеет, но духи гугла говорят
	//что буст надёжнее и реже возвращает 0.
	infoStruct.cpuCoresNumber = boost::thread::hardware_concurrency();
	if (!infoStruct.cpuCoresNumber) infoStruct.cpuCoresNumber = 1;	//0 ядер не бывает.

	//Инфу о памяти буст получать не умеет, или не умел в версии 1.66. Всё как надо сделаем для
	//вендов и линуха, для остальных юниксов код скорее на удачу - может и сработает.
	infoStruct.systemMemoryFullSize = 0;	//Нули - признак ошибки, т.е. получить инфу для поля не удалось.
	infoStruct.systemMemoryFreeSize = 0;
	infoStruct.maxProcessMemorySize = 0;

	#ifdef _WIN32
		//Под вендами всё просто и единообразно - единственный вызов WinAPI даст всё в одной структуре.
		MEMORYSTATUSEX statex;
		statex.dwLength = sizeof(statex);
		if (GlobalMemoryStatusEx(&statex))
		{
			//Общее количество оперативки:
			if (statex.ullTotalPhys != -1) infoStruct.systemMemoryFullSize = statex.ullTotalPhys;
			//Сколько оперативки свободно:
			if (statex.ullAvailPhys != -1) infoStruct.systemMemoryFreeSize = statex.ullAvailPhys;
			//Сколько может адресовать процесс:
			if (statex.ullTotalVirtual != -1) infoStruct.maxProcessMemorySize = statex.ullTotalVirtual;
		};
	#elif (defined(unix) || defined(__unix__) || defined(__unix))
		//Сколько может адресовать процесс:
		//По идее getrlimit должен быть в любом unix  с поддержкой POSIX 2001
		rlimit rlimitInfo;
		if (!getrlimit(RLIMIT_DATA, &rlimitInfo))
		{
			infoStruct.maxProcessMemorySize = rlimitInfo.rlim_max;
			//Если там -1 то это не ошибка а отсутствие лимита. Уменьшим значение
			//чтобы где-нибудь его не сравнили с -1 и не посчитали за ошибку.
			if (infoStruct.maxProcessMemorySize == -1)
				infoStruct.maxProcessMemorySize--;
		}

		//Остальные параметры памяти по-разному получаются для linux-а и для остальных юниксов.
		#ifdef __linux__
			//Тут нужно расковыривать содержимое /proc/meminfo
			FillSysInfoFromProcMeminfo(infoStruct);
		#elif defined(_SC_PAGE_SIZE) && defined(_SC_AVPHYS_PAGES) && defined(_SC_PHYS_PAGES)
			//Может быть и сработает.
			long pagesize = sysconf(_SC_PAGE_SIZE);
			if (pagesize != -1)
			{
				//Общее количество оперативки:
				long pages = sysconf(_SC_PHYS_PAGES);
				if ((pagesize != -1) && (pages != -1))
				{
					infoStruct.systemMemoryFullSize = pagesize * pages;
				}
				//Сколько оперативки свободно:
				pages = sysconf(_SC_AVPHYS_PAGES);
				if ((pagesize != -1) && (pages != -1))
				{
					infoStruct.systemMemoryFreeSize = pagesize * pages;
				}
			}
		#else
			#error Unknown target OS. Cant preprocess memory detecting code :(
		#endif
	#else
		#error Unknown target OS. Cant preprocess memory detecting code :(
	#endif

	//Если у нас i386 32-битная или 64-битная архитектура - можно попробовать жёстко
	//задать maxProcessMemorySize (если до сих пор ничего найти не удалось).
	#if BOOST_ARCH_X86_32
	if (infoStruct.maxProcessMemorySize == 0)
		infoStruct.maxProcessMemorySize = 2147483648;	//2 гигабайта. Больше система может и не уметь.
	#elif BOOST_ARCH_X86_64
	if (infoStruct.maxProcessMemorySize == 0)
		infoStruct.maxProcessMemorySize = 8796093022208;	//8 терабайт. Больше система может и не уметь.
	#endif
}

}	//namespace geoimgconv
