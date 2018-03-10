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
	AppConfig confObj(STB.GetConsoleWidth());
	ErrorInfo errObj;
	STB.InitEncodings();
	if (!confObj.ParseCommandLine(argc,argv,&errObj))
	{
		//Что-то пошло не так
		cout << STB.Utf8ToConsoleCharset("Ошибка: "+errObj.getErrorText()) << endl;
		return 1;
	}
	
	//Пока реализовано лишь 2 режима работы.
	if (confObj.getAppMode() == APPMODE_MEDIAN)
	{
		//Медианная фильтрация в консоли.
		AppUIConsole theApp;
		theApp.InitApp(confObj);
		return theApp.RunApp();
	}
	else if (confObj.getAppMode() == APPMODE_DEVTEST)
	{
		//Тестовый режим. Для разработки. Опция командной строки для такого запуска
		//скрытая, в справке не описана, юзер про неё знать не должен.
		AppUIConsole theApp;
		return theApp.RunTestMode();
	}
	else
	{
		cout << STB.Utf8ToConsoleCharset("Ошибка: программа запущена в неизвестном или не реализованном режиме.") << endl;
		return 1;
	}
}
