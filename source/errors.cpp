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

#include "errors.h"

namespace geoimgconv
{

const std::string CommonErrorsTexts[] = { "Нет ошибок",	//0
			"Ошибка чтения файла",							//1
			"Ошибка записи файла",							//2
			"Не реализовано",								//3
			"Файл не существует",							//4
			"Формат файла не поддерживается",				//5
			"Файл не был загружен",							//6
			"Неизвестный идентификатор в командной строке"	//7
};

void ErrorInfo::SetError(const CommonErrors &errCode, const std::string &additionalText)
//Записывает в объект код ошибки и соответствующее коду сообщение
//из CommonErrorsTexts[], к которому добавляет содержимое второго
//аргумента если он был указан.
{
	this->errorCode_ = errCode;
	if (additionalText != "")
		this->errorText_ = CommonErrorsTexts[errCode] + additionalText;
	else
		this->errorText_ = CommonErrorsTexts[errCode];
}

}	//namespace geoimgconv