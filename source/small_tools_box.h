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

//Небольшой сборник функций общего назначения, которые пока непонятно куда засунуть.
//В основном - разнообразная работа с консолью, кодировками, файловой системой.
//Оформлено в виде класса с методами.

//Строки местами могут возвращаются копированием потому что в locale::boost
//тоже, похоже, копирование :(. Возможно компиллятор отоптимизирует.

//Методами Select*Encoding() можно переключаться между системной и консольной кодировкой.
//Методы для работы с SelectedEncoding после этого работают с выбранной кодировкой.
//Может быть нужно если в системе кодировка в GUI и в консоли различается.
//Изначально выбрана консольная кодировка.

//Прежде чем пользоваться методами для работы с кодировками - кодировки надо инициализировать
//методом InitEncodings(), хотя по идее при первом обращении они должны инициализироваться
//автоматически.

//TODO - возможно стоит переделать в синглтон, но это не точно.

//TODO - надо прекратить использовать возврат const value (именно значений, не указателей и не ссылок) из
//методов! Духи говорят что во времена C++11 это не эффективно в связи с существованием rvalues.

#include <iostream>
#include <string>

namespace geoimgconv
{

//Структура для метода GetSysResInfo
struct SysResInfo
{
	unsigned int cpuCoresNumber;	//Количество ядер процессора.
	unsigned long long systemMemoryFullSize;	//Общее количество (без свопа) оперативной памяти.
	unsigned long long systemMemoryFreeSize;	//Количество свободной (без свопа) оперативной памяти.
	unsigned long long maxProcessMemorySize;	//Сколько памяти может адресовать данный процесс.
};

//Класс-сборник разных функций.
class SmallToolsBox
{
	private:
		//Поля
		std::string consoleEncoding_ = "";
		std::string systemEncoding_ = "";
		bool encodingsInited_ = false;
		std::locale utf8Locale_;
		bool consoleEncodingIsSelected_ = false;	//Выбор между console и system кодировками.
		std::string filesystemSeparator_ = "";
	public:
		//Конструктор и деструктор
		SmallToolsBox() {}
		~SmallToolsBox() {}

		//Геттеры-сеттеры.
		std::string const& GetConsoleEncoding() const { return consoleEncoding_; };
		std::string const& GetSystemEncoding() const { return systemEncoding_; };

		//Остальные методы.

		//Переключает глобальную локаль на текущую, в которой запущено приложение.\
		//Узнаёт и запоминает кодировки - в консоли и системную. Желательно вызвать
		//до обращения к функциям перекодировки.
		void InitEncodings();

		//Выбрать консольную кодировку для работы с SelectedCharset
		inline void SelectConsoleEncoding() { consoleEncodingIsSelected_ = true; };

		//Выбрать системную кодировку для работы с SelectedCharset
		inline void SelectSystemEncoding() { consoleEncodingIsSelected_ = false; };

		//Перекодирует строку из utf8 в кодировку, подходящую для вывода в консоль.
		std::string Utf8ToConsoleCharset(const std::string &InputStr);

		//Перекодирует строку из кодировки консоли в utf8
		std::string ConsoleCharsetToUtf8(const std::string &InputStr);

		//Перекодирует строку из utf8 в системную кодировку.
		std::string Utf8ToSystemCharset(const std::string &InputStr);

		//Перекодирует строку из системной кодировки в utf8
		std::string SystemCharsetToUtf8(const std::string &InputStr);

		//Перекодирует строку из utf8 в кодировку, выбранную ранее методами SelectCharset*.
		std::string Utf8ToSelectedCharset(const std::string &InputStr);

		//Перекодирует строку из кодировки, выбранной ранее методами SelectCharset* в utf8
		std::string SelectedCharsetToUtf8(const std::string &InputStr);

		//Перекодирует wstring в string в кодировке utf8
		std::string WstringToUtf8(const std::wstring &inputStr);

		//Перекодирует string в кодировке utf8 в wstring
		std::wstring Utf8ToWstring(const std::string &inputStr);

		//Преобразует double в строку с указанным количеством знаков после запятой.
		std::string DoubleToString(const double &input, const unsigned int &precision) const;

		//Преобразует bool в строку. Потоки или lexical_cast имхо лишнее.
		std::string BoolToString(const bool &input) const;

		//Преобразовать целое число в строку фиксированного размера с заданным количеством
		//нулей в начале.
		std::string IntToString(const long long value, int width);

		//Перевести в нижний регистр utf8-строку.
		void Utf8ToLower(const std::string &inputStr, std::string &outputStr) const;

		//Перевести в верхний регистр utf8-строку.
		void Utf8ToUpper(const std::string &inputStr, std::string &outputStr) const;

		//Преобразовать количество байт в удобную для юзера строку с мегабайтами-гигабайтами.
		const std::string BytesNumToInfoSizeStr(const unsigned long long &bytesNum) const;

		//Прочитать количество информации в байтах из строки. Формат не совпадает с форматом,
		//выводимым методом выше. Здесь все символы кроме последнего должны быть беззнаковым
		//целым числом, последний же символ может быть B или b - байты, K или k - килобайты,
		//M или m - мегабайты, G или g - гигабайты, T или t - терабайты. Если символ не указан
		//- применяется символ по умолчанию (второй аргумент).
		const unsigned long long InfoSizeToBytesNum(const std::string &inputStr, char defaultUnit = 'b') const;

		//Проверить содержится ли в строке целое беззнаковое число.
		const bool CheckUnsIntStr(const std::string &inputStr) const;

		//Проверить содержится ли в строке целое со знаком.
		const bool CheckSignedIntStr(const std::string &inputStr) const;

		//Проверить содержится ли в строке число с плавающей запятой
		const bool CheckFloatStr(const std::string &inputStr) const;

		//Проверить содержится ли в строке размер чего-либо в байтах (формат тот же, что в
		//методе InfoSizeToBytesNum()
		const bool CheckInfoSizeStr(const std::string &inputStr) const;

		//Возвращает число процессорных ядер или 0 если это количество получить не удалось.
		const unsigned int GetCpuCoresNumber() const;

		//Возвращает общее количество оперативной памяти (без свопа) в системе или 0 при ошибке.
		const unsigned long long GetSystemMemoryFullSize() const;

		//Возвращает количество свободной оперативной памяти (без свопа) в системе или 0 при ошибке.
		const unsigned long long GetSystemMemoryFreeSize() const;

		//Возвращает максимальное количество памяти, которое вообще может потребить данный процесс.
		const unsigned long long GetMaxProcessMemorySize() const;

		//Вернуть информацию о ресурсах системы - то же, что и несколько методов выше, но должно
		//работать быстрее чем последовательный вызов всех этих методов.
		void GetSysResInfo(SysResInfo &infoStruct) const;

		//Узнать текущую ширину консоли. Вернёт 0 в случае ошибки.
		unsigned short GetConsoleWidth() const;

		//Возвращает используемый в данной системе разделитель путей к файлам, в виде обычной строки.
		const std::string& GetFilesystemSeparator();
};

extern SmallToolsBox STB;

}	//namespace geoimgconv
