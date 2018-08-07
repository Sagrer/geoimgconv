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

//Обработка ошибок.

#include <string>

namespace geoimgconv
{

enum class CommonErrors : int
{
	//UnknownError обязательно должна идти последней!
	NoError = 0,
	ReadError = 1,
	WriteError = 2,
	FeatureNotReady = 3,
	FileNotExists = 4,
	FileExistsAlready = 5,
	UnsupportedFileFormat = 6,
	FileNotLoaded = 7,
	CmdLineParseError = 8,
	UnknownIdentif = 9,
	CantAllocMemory = 10,
	InternalError = 11,
	UnknownError = 12
};

//Текстовое представление для enum CommonErrors
extern const std::string CommonErrorsTexts[];

class ErrorInfo
//Объекты этого класса передаются в функции, которые могут завершиться с ошибкой.
{
public:	
	ErrorInfo() {};
	virtual ~ErrorInfo() {};

	//Доступ к полям.

	//errorCode
	CommonErrors const& getErrorCode() const { return errorCode_; }
	//errorText_
	std::string const& getErrorText() const { return errorText_; }

	//Записывает в объект код ошибки и соответствующее коду сообщение
	//из CommonErrorsTexts[], к которому добавляет содержимое второго
	//аргумента если он был указан, либо всё сообщение берёт из второго
	//аргумента если истинен третий. Если text пуст - стандартный текст
	//сообщения берётся независимо от значения replaceText
	void SetError(const CommonErrors &errCode, const std::string &text = "", const bool replaceText = false);

private:
	//Код и текст ошибки
	CommonErrors errorCode_ = CommonErrors::NoError;
	std::string errorText_;

};

}	//namespace geoimgconv