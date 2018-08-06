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

//Небольшой сборник функций для работы со строками. Локали, перекодировка, более
//удобные чем стандартные преобразования из и в строки. Синглтон - не нужен,
//глобальный объект - тем более, поэтому статический класс.

//Методами Select*Encoding() можно переключаться между системной и консольной кодировкой.
//Методы для работы с SelectedEncoding после этого работают с выбранной кодировкой.
//Может быть нужно если в системе кодировка в GUI и в консоли различается.
//Изначально выбрана консольная кодировка.

//Прежде чем пользоваться методами для работы с кодировками - кодировки надо инициализировать
//методом InitEncodings(), хотя по идее при первом обращении они должны инициализироваться
//автоматически.

//Если требуется работать с классом из нескольких потоков - InitEncodings()
//и Select*Encoding() должно быть выполнено до запуска остальных потоков.

//TODO - надо прекратить использовать возврат const value (именно значений, не указателей и не ссылок) из
//методов! Духи говорят что во времена C++11 это не эффективно в связи с существованием rvalues.

//#include <iostream>
#include <string>
#include <chrono>
#include <locale>

namespace geoimgconv
{

class StringsToolsBox
{
	public:
		//Экземпляр создавать запрещено.
		StringsToolsBox() = delete;
		~StringsToolsBox() = delete;

		//Можно узнать текущие кодировки.
		static std::string const& GetConsoleEncoding() { return consoleEncoding_; };
		static std::string const& GetSystemEncoding() { return systemEncoding_; };

		//Остальные методы.

		//Переключает глобальную локаль на текущую, в которой запущено приложение.\
		//Узнаёт и запоминает кодировки - в консоли и системную. Желательно вызвать
		//до обращения к функциям перекодировки.
		static void InitEncodings();

		//Выбрать консольную кодировку для работы с SelectedCharset
		static void SelectConsoleEncoding() { consoleEncodingIsSelected_ = true; };

		//Выбрать системную кодировку для работы с SelectedCharset
		static void SelectSystemEncoding() { consoleEncodingIsSelected_ = false; };

		//Перекодирует строку из utf8 в кодировку, подходящую для вывода в консоль.
		static std::string Utf8ToConsoleCharset(const std::string &InputStr);

		//Перекодирует строку из кодировки консоли в utf8
		static std::string ConsoleCharsetToUtf8(const std::string &InputStr);

		//Перекодирует строку из utf8 в системную кодировку.
		static std::string Utf8ToSystemCharset(const std::string &InputStr);

		//Перекодирует строку из системной кодировки в utf8
		static std::string SystemCharsetToUtf8(const std::string &InputStr);

		//Перекодирует строку из utf8 в кодировку, выбранную ранее методами SelectCharset*.
		static std::string Utf8ToSelectedCharset(const std::string &InputStr);

		//Перекодирует строку из кодировки, выбранной ранее методами SelectCharset* в utf8
		static std::string SelectedCharsetToUtf8(const std::string &InputStr);

		//Перекодирует wstring в string в кодировке utf8
		static std::string WstringToUtf8(const std::wstring &inputStr);

		//Перекодирует string в кодировке utf8 в wstring
		static std::wstring Utf8ToWstring(const std::string &inputStr);

		//Преобразует double в строку с указанным количеством знаков после запятой.
		static std::string DoubleToString(const double &input, const unsigned int &precision);

		//Преобразует bool в строку. Потоки или lexical_cast имхо лишнее.
		static std::string BoolToString(const bool &input);

		//Преобразовать целое число в строку фиксированного размера с заданным количеством
		//нулей в начале.
		static std::string IntToString(const long long value, int width);

		//Перевести в нижний регистр utf8-строку.
		static void Utf8ToLower(const std::string &inputStr, std::string &outputStr);

		//Перевести в верхний регистр utf8-строку.
		static void Utf8ToUpper(const std::string &inputStr, std::string &outputStr);

		//Преобразовать количество байт в удобную для юзера строку с мегабайтами-гигабайтами.
		static const std::string BytesNumToInfoSizeStr(const unsigned long long &bytesNum);

		//Прочитать количество информации в байтах из строки. Формат не совпадает с форматом,
		//выводимым методом выше. Здесь все символы кроме последнего должны быть беззнаковым
		//целым числом, последний же символ может быть B или b - байты, K или k - килобайты,
		//M или m - мегабайты, G или g - гигабайты, T или t - терабайты. Если символ не указан
		//- применяется символ по умолчанию (второй аргумент).
		static const unsigned long long InfoSizeToBytesNum(const std::string &inputStr, char defaultUnit = 'b');

		//Проверить содержится ли в строке целое беззнаковое число.
		static const bool CheckUnsIntStr(const std::string &inputStr);

		//Проверить содержится ли в строке целое со знаком.
		static const bool CheckSignedIntStr(const std::string &inputStr);

		//Проверить содержится ли в строке число с плавающей запятой
		static const bool CheckFloatStr(const std::string &inputStr);

		//Проверить содержится ли в строке размер чего-либо в байтах (формат тот же, что в
		//методе InfoSizeToBytesNum()
		static const bool CheckInfoSizeStr(const std::string &inputStr);

		//Преобразует период времени std::chrono::microseconds в строку формата hh:mm:ss.ms (миллисекунды
		//опционально.
		static const std::string TimeDurationToString(const std::chrono::microseconds &duration,
			bool printMilliseconds = false);

		//Преобразует период времени std::chrono::milliseconds в строку формата hh:mm:ss.ms (миллисекунды
		//опционально.
		static const std::string TimeDurationToString(const std::chrono::milliseconds &duration,
			bool printMilliseconds = false);

		//Преобразует период времени std::chrono::seconds в строку формата hh:mm:ss.
		static const std::string TimeDurationToString(const std::chrono::seconds &duration);
		
	private:
		//Поля
		static std::string consoleEncoding_;
		static std::string systemEncoding_;
		static bool encodingsInited_;
		static std::locale utf8Locale_;
		static bool consoleEncodingIsSelected_;	//Выбор между console и system кодировками.
};

//Сокращённое наименование.
using StrTB = StringsToolsBox;

}	//namespace geoimgconv
