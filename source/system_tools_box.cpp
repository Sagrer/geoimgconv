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

//Небольшой сборник функций для получения информации о состоянии системы.
//Синглтон - не нужен, глобальный объект - тем более, поэтому статический класс.

#include "system_tools_box.h"
#include <boost/filesystem.hpp>
#include <boost/thread/thread.hpp>
#include <cctype>
#include <type_traits>
//predef - чтобы иметь доступ к макросам для определения архитектуры процессора.
#include <boost/predef.h>
#include "strings_tools_box.h"

//Для определения количества памяти нужен платформозависимый код :(
#ifdef _WIN32
#include <Windows.h>
#elif defined(unix) || defined(__unix__) || defined(__unix)
#include <sys/types.h>
#include <unistd.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/ioctl.h>
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

//Статические поля класса.
string SystemToolsBox::filesystemSeparator_ = "";

//Возвращает число процессорных ядер или 0 если это количество получить не удалось.
const unsigned int SystemToolsBox::GetCpuCoresNumber()
{
	//Количество ядер умеет узнавать Boost. C++11 тоже умеет, но духи гугла говорят
	//что буст надёжнее и реже возвращает 0.
	auto result = boost::thread::hardware_concurrency();
	if (!result) result = 1;	//0 ядер не бывает.
	return result;
}

#ifdef __linux__
//Парсим /proc/meminfo и вынимаем нужное нам значение. При ошибке вернём 0.
const unsigned long long GetProcMeminfoField(const string &fieldName)
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
	ostringstream sstream;
	sstream << ifile.rdbuf();
	string fileContents(sstream.str());
	ifile.close();

	//Готовим токенайзеры.
	char_separator<char> linesSep("\n"); //Делить файл на строки
	char_separator<char> tokensSep(" \t:");  //Делить строки на поля
	tokenizer<char_separator<char> > linesTok(fileContents, linesSep);

	//Ковыряем содержимое.
	for (auto i = linesTok.begin();
		i !=linesTok.end(); i++)
	{
		tokenizer<char_separator<char> >currLineTok(*i, tokensSep);
		auto j = currLineTok.begin();
		if (j!=currLineTok.end())
		{
			if (*j == fieldName)
			{
				//Совпадает имя поля.
				j++;
				if (j!=currLineTok.end())
				{
					//Есть второе поле - это будет число, его и вернём умножив на размер килобайта.
					return stoull(*j) * 1024;
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
	ostringstream sstream;
	sstream << ifile.rdbuf();
	string fileContents(sstream.str());
	ifile.close();

	//Готовим токенайзеры.
	char_separator<char> linesSep("\n"); //Делить файл на строки
	char_separator<char> tokensSep(" \t:");  //Делить строки на поля
	tokenizer<char_separator<char> > linesTok(fileContents, linesSep);

	//Ковыряем содержимое.
	for (auto i = linesTok.begin(); i != linesTok.end(); i++)
	{
		tokenizer<char_separator<char> >currLineTok(*i, tokensSep);
		auto j = currLineTok.begin();
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
					*fieldPointer = stoull(*j) * 1024;
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
const unsigned long long SystemToolsBox::GetSystemMemoryFullSize()
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
const unsigned long long SystemToolsBox::GetSystemMemoryFreeSize()
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
const unsigned long long SystemToolsBox::GetMaxProcessMemorySize()
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
void SystemToolsBox::GetSysResInfo(SysResInfo &infoStruct)
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

//Узнать текущую ширину консоли. Вернёт 0 в случае ошибки.
unsigned short SystemToolsBox::GetConsoleWidth()
{
	#ifdef _WIN32
	HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO consInfo;
	if (!GetConsoleScreenBufferInfo(hStdOut, &consInfo))
	{
		return 0;
	}
	else
	{
		return consInfo.dwSize.X;
	}
	#elif (defined(unix) || defined(__unix__) || defined(__unix))
	struct winsize consSize;
	if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &consSize) == 0)
	{
		return consSize.ws_col;
	}
	else
	{
		return 0;
	}
	#else
	//#error Unknown target OS. Cant preprocess console width detecting code :(
	return 0;
	#endif
}

//Возвращает используемый в данной системе разделитель путей к файлам, в виде обычной строки.
const string& SystemToolsBox::GetFilesystemSeparator()
{
	if (filesystemSeparator_ != "") return filesystemSeparator_;

	//Если разделитель ещё не был инициализирован - вытащим его из boost::filesystem и запомним.
	//Придётся напрямую копировать память.
	namespace b_fs = boost::filesystem;
	//Вот это шаманство с auto и копированием сепаратора - связано с тем, что как минимум
	//под Boost 1.62 напрямую компиллятор preferred_separator не видит при линковке, вероятно
	//это как-то связано с constexpr в недрах буста. Хотя на Boost 1.66 всё было норм :).
	auto prefSep = b_fs::path::preferred_separator;
	if (is_same<decltype(prefSep), char>::value)
	{
		//Обычный char. Делаем двухбайтовый буфер (последний символ - ноль) и копируем первый байт.
		//Значение первого символа по умолчанию - просто наиболее вероятное.
		char charBuf[] = "/";
		memcpy(charBuf, &prefSep, sizeof(char));
		filesystemSeparator_ = string(charBuf);
		return filesystemSeparator_;
	}
	else
	{
		//wchar. В целом, аналогично.
		wchar_t charBuf[] = L"\\";
		memcpy(charBuf, &prefSep, sizeof(wchar_t));
		filesystemSeparator_ = StrTB::WstringToUtf8(wstring(charBuf));
		return filesystemSeparator_;
	}
}

}	//namespace geoimgconv
