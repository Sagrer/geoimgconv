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

}	//namespace geoimgconv