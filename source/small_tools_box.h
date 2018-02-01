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

#include <iostream>
#include <string>

namespace geoimgconv
{

class SmallToolsBox
{
	private:
		//Поля
		std::string consoleEncoding_;
		std::string systemEncoding_;
		bool encodingsInited_;
		std::locale utf8Locale_;
		bool consoleEncodingIsSelected_;	//Выбор между console и system кодировками.
	public:
		//Конструктор и деструктор
		SmallToolsBox();
		~SmallToolsBox();

		//Геттеры-сеттеры.
		std::string const& GetConsoleEncoding() const { return this->consoleEncoding_; };

		//Остальные методы.

		//Переключает глобальную локаль на текущую, в которой запущено приложение.\
		//Узнаёт и запоминает кодировки - в консоли и системную. Желательно вызвать
		//до обращения к функциям перекодировки.
		void InitEncodings();

		//Выбрать консольную кодировку для работы с SelectedCharset
		inline void SelectConsoleEncoding() { this->consoleEncodingIsSelected_ = true; };

		//Выбрать системную кодировку для работы с SelectedCharset
		inline void SelectSystemEncoding() { this->consoleEncodingIsSelected_ = false; };
		
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

		//Перевести в нижний регистр utf8-строку.
		void Utf8ToLower(const std::string &inputStr, std::string &outputStr) const;

		//Перевести в верхний регистр utf8-строку.
		void Utf8ToUpper(const std::string &inputStr, std::string &outputStr) const;
};

extern SmallToolsBox STB;

}	//namespace geoimgconv