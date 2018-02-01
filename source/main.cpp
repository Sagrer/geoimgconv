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
//Главный файл с точкой входа.

#include "appui_console.h"
#include "app_config.h"
#include "errors.h"
#include "small_tools_box.h"
#include <string>

using namespace std;
using namespace geoimgconv;

int main(int argc, char** argv)
{
	////Отладочное ожидание ввода.
	//string temp;
	//cout << "Debug wait for attaching. Attach debugger, enter something and press Enter.\n" << endl;
	//cin >> temp;
	
	//Мы не знаем заранее в каком режиме будем работать, поэтому сначала разбираем
	//настройки и командную строку и только потом создаём и передаём управление объекту
	//приложения нужного типа.
	AppConfig confObj;
	ErrorInfo errObj;
	STB.InitEncodings();
	if (!confObj.ParseCommandLine(argc,argv,&errObj))
	{
		//Что-то пошло не так
		cout << STB.Utf8ToConsoleCharset("Ошибка: "+errObj.getErrorText());
		return 1;
	}
	
	//Пока всего 1 вариант режима работы.
	AppUIConsole theApp;
	theApp.InitApp(confObj);
	return theApp.RunApp();
}
