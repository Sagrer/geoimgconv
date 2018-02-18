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

//Сборник типов для обработки ошибок.

#include <string>

namespace geoimgconv
{

enum CommonErrors : int
{
	//CMNERR_UNKNOWN_ERROR обязательно должна идти последней!
	CMNERR_NO_ERROR = 0,
	CMNERR_READ_ERROR = 1,
	CMNERR_WRITE_ERROR = 2,
	CMNERR_FEATURE_NOT_READY = 3,
	CMNERR_FILE_NOT_EXISTS = 4,
	CMNERR_UNSUPPORTED_FILE_FORMAT = 5,
	CMNERR_FILE_NOT_LOADED = 6,
	CMNERR_CMDLINE_PARSE_ERROR = 7,
	CMNERR_UNKNOWN_IDENTIF = 8,
	CMNERR_CANT_ALLOC_MEMORY = 9,
	CMNERR_UNKNOWN_ERROR = 10
};

extern const std::string CommonErrorsTexts[];

class ErrorInfo
//Объекты этого класса передаются в функции, которые могут завершиться с ошибкой.
{
private:
	//Код и текст ошибки
	CommonErrors errorCode_;
	std::string errorText_;
public:	
	ErrorInfo() : errorCode_(CMNERR_NO_ERROR) {};
	~ErrorInfo() {};

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
};

}	//namespace geoimgconv