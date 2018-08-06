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

//Небольшой сборник функций для работы со строками. Локали, перекодировка, более
//удобные чем стандартные преобразования из и в строки.

#include "strings_tools_box.h"
#include <boost/locale.hpp>
#include <sstream>
#include <iomanip>

using namespace std;

namespace geoimgconv
{

//Статические поля класса.
string StringsToolsBox::consoleEncoding_ = "";
string StringsToolsBox::systemEncoding_ = "";
bool StringsToolsBox::encodingsInited_ = false;
locale StringsToolsBox::utf8Locale_;
bool StringsToolsBox::consoleEncodingIsSelected_ = false;

//Переключает глобальную локаль на текущую, в которой запущено приложение.\
//Узнаёт и запоминает кодировки - в консоли и системную. Желательно вызвать
//до обращения к функциям перекодировки.
void StringsToolsBox::InitEncodings()
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
	consoleEncoding_ = use_facet<boost::locale::info>(currLocale).encoding();
	systemEncoding_ = consoleEncoding_;
	#endif

	//Сгенерируем локаль для работы с utf8
	utf8Locale_ = locGen("utf8");

	//Готово.
	encodingsInited_ = true;
}

//Перекодирует строку из utf8 в кодировку, подходящую для вывода в консоль.
string StringsToolsBox::Utf8ToConsoleCharset(const string &InputStr)
{
	if (!encodingsInited_)
		InitEncodings();
	return boost::locale::conv::from_utf(InputStr, consoleEncoding_);
}

//Перекодирует строку из кодировки консоли в utf8
string StringsToolsBox::ConsoleCharsetToUtf8(const string &InputStr)
{
	if (!encodingsInited_)
		InitEncodings();
	return boost::locale::conv::to_utf<char>(InputStr, consoleEncoding_);
}

//Перекодирует строку из utf8 в системную кодировку.
string StringsToolsBox::Utf8ToSystemCharset(const string &InputStr)
{
	if (!encodingsInited_)
		InitEncodings();
	return boost::locale::conv::from_utf(InputStr, systemEncoding_);
}

//Перекодирует строку из системной кодировки в utf8
string StringsToolsBox::SystemCharsetToUtf8(const string &InputStr)
{
	if (!encodingsInited_)
		InitEncodings();
	return boost::locale::conv::to_utf<char>(InputStr, systemEncoding_);
}

//Перекодирует строку из utf8 в кодировку, выбранную ранее методами SelectCharset*.
string StringsToolsBox::Utf8ToSelectedCharset(const string &InputStr)
{
	if (consoleEncodingIsSelected_)
		return Utf8ToConsoleCharset(InputStr);
	else
		return Utf8ToSystemCharset(InputStr);
}

//Перекодирует строку из кодировки, выбранной ранее методами SelectCharset* в utf8
string StringsToolsBox::SelectedCharsetToUtf8(const string &InputStr)
{
	if (consoleEncodingIsSelected_)
		return ConsoleCharsetToUtf8(InputStr);
	else
		return SystemCharsetToUtf8(InputStr);
}

//Перекодирует wstring в string в кодировке utf8
string StringsToolsBox::WstringToUtf8(const wstring &inputStr)
{
	if (!encodingsInited_)
		InitEncodings();
	return boost::locale::conv::utf_to_utf<char, wchar_t>(inputStr);
}

//Перекодирует string в кодировке utf8 в wstring
wstring StringsToolsBox::Utf8ToWstring(const string &inputStr)
{
	if (!encodingsInited_)
		InitEncodings();
	return boost::locale::conv::utf_to_utf<wchar_t, char>(inputStr);
}

//Преобразует double в строку с указанным количеством знаков после запятой.
string StringsToolsBox::DoubleToString(const double &input,
	const unsigned int &precision)
{
	ostringstream stringStream;
	stringStream << fixed << setprecision(precision) << input;
	return stringStream.str();
}

//Преобразует bool в строку. Потоки или lexical_cast имхо лишнее.
string StringsToolsBox::BoolToString(const bool &input)
{
	return input ? "true" : "false";
}

//Преобразовать целое число в строку фиксированного размера с заданным количеством
//нулей в начале.
string StringsToolsBox::IntToString(const long long value, int width)
{
	stringstream sStream;
	sStream << setw(width) << setfill('0') << value;
	return sStream.str();
}

//Перевести в нижний регистр utf8-строку.
void StringsToolsBox::Utf8ToLower(const string &inputStr, string &outputStr)
{
	outputStr = boost::locale::to_lower(inputStr, utf8Locale_);
}

//Перевести в верхний регистр utf8-строку.
void StringsToolsBox::Utf8ToUpper(const string &inputStr, string &outputStr)
{
	outputStr = boost::locale::to_upper(inputStr, utf8Locale_);
}

//Преобразовать количество байт в удобную для юзера строку с мегабайтами-гигабайтами.
const string StringsToolsBox::BytesNumToInfoSizeStr(const unsigned long long &bytesNum)
{
	using namespace boost;
	//Если число меньше килобайта - выводим байты, иначе если меньше мегабайта - выводим
	//килобайты, и так далее до терабайтов.
	//TODO: нужна будет какая-то i18n и l10n. Когда-нибудь. В отдалённом светлом будущем.

	if (bytesNum < 1024)
	{
		return to_string(bytesNum) + " байт";
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
const unsigned long long StringsToolsBox::InfoSizeToBytesNum(const string &inputStr, char defaultUnit)
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
		result = stoull(numberStr);
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
const bool StringsToolsBox::CheckUnsIntStr(const string &inputStr)
{
	if ((inputStr.length() > 0) && ((isdigit((unsigned char)(inputStr[0]))) || (inputStr[0] == '+')))
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
const bool StringsToolsBox::CheckSignedIntStr(const string &inputStr)
{
	istringstream strStream(inputStr);
	signed long long testVar;
	//noskipws нужен чтобы всякие пробелы не игнорировались
	strStream >> noskipws >> testVar;
	return strStream && strStream.eof();
}

//Проверить содержится ли в строке число с плавающей запятой
const bool StringsToolsBox::CheckFloatStr(const string &inputStr)
{
	istringstream strStream(inputStr);
	long double testVar;
	//noskipws нужен чтобы всякие пробелы не игнорировались
	strStream >> noskipws >> testVar;
	return strStream && strStream.eof();
}

//Проверить содержится ли в строке размер чего-либо в байтах (формат тот же, что в
//методе InfoSizeToBytesNum()
const bool StringsToolsBox::CheckInfoSizeStr(const string &inputStr)
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

//Преобразует период времени std::chrono::microseconds в строку формата hh:mm:ss.ms (миллисекунды
//опционально.
const string StringsToolsBox::TimeDurationToString(const chrono::microseconds &duration, bool printMilliseconds)
{
	//Поток, в котором будем формировать строку.
	stringstream s_stream;
	//Считаем часы, минуты, секунды, возможно миллисекунды.
	chrono::hours durationHours = chrono::duration_cast<chrono::hours>(duration);
	chrono::microseconds msLeft = duration - chrono::duration_cast<chrono::microseconds>(durationHours);
	chrono::minutes durationMinutes = chrono::duration_cast<chrono::minutes>(msLeft);
	msLeft = msLeft - chrono::duration_cast<chrono::microseconds>(durationMinutes);
	chrono::seconds durationSeconds = chrono::duration_cast<chrono::seconds>(msLeft);
	//Выводим обязательную часть.
	s_stream << setw(2) << setfill('0') << durationHours.count() << ":";
	s_stream << setw(2) << durationMinutes.count() << ":";
	s_stream << setw(2) << durationSeconds.count();
	//Теперь миллисекунды если надо.
	if (printMilliseconds)
	{
		msLeft = msLeft - chrono::duration_cast<chrono::microseconds>(durationSeconds);
		chrono::milliseconds durationMilliseconds = chrono::duration_cast<chrono::milliseconds>(msLeft);
		s_stream << "." << durationMilliseconds.count();
	}

	return s_stream.str();
}

//Преобразует период времени std::chrono::milliseconds в строку формата hh:mm:ss.ms (миллисекунды
//опционально.
const string StringsToolsBox::TimeDurationToString(const chrono::milliseconds &duration, bool printMilliseconds)
{
	return TimeDurationToString(chrono::duration_cast<chrono::microseconds>(duration), printMilliseconds);
}

//Преобразует период времени std::chrono::seconds в строку формата hh:mm:ss.
const string StringsToolsBox::TimeDurationToString(const chrono::seconds & duration)
{
	return TimeDurationToString(chrono::duration_cast<chrono::microseconds>(duration), false);
}

}	//namespace geoimgconv
