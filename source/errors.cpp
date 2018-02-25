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

#include "errors.h"

namespace geoimgconv
{

//Текстовое представление для enum CommonErrors
const std::string CommonErrorsTexts[] = { "Нет ошибок",	//0
			"Ошибка чтения файла",							//1
			"Ошибка записи файла",							//2
			"Не реализовано",								//3
			"Файл не существует",							//4
			"Файл уже существует",							//5
			"Формат файла не поддерживается",				//6
			"Файл не был загружен",							//7
			"Неверный синтаксис опций командной строки",	//8
			"Неизвестный идентификатор в командной строке",	//9
			"Не могу выделить достаточно памяти",			//10
			"Внутренняя ошибка",							//11
			"Неизвестная ошибка"							//12
};

//Записывает в объект код ошибки и соответствующее коду сообщение
//из CommonErrorsTexts[], к которому добавляет содержимое второго
//аргумента если он был указан, либо всё сообщение берёт из второго
//аргумента если истинен третий. Если text пуст - стандартный текст
//сообщения берётся независимо от значения replaceText
void ErrorInfo::SetError(const CommonErrors &errCode, const std::string &text, const bool replaceText)
{
	errorCode_ = errCode;
	if (text != "")
	{
		if (replaceText)
			errorText_ = text;
		else
			errorText_ = CommonErrorsTexts[errCode] + text;
	}
	else
		errorText_ = CommonErrorsTexts[errCode];
}

}	//namespace geoimgconv