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

//Синглтон с глобальным для тестов набором полей. Впервые к нему должна обращаться
//глобальная фикстура.

#include <string>

class CommonVars
{
public:
	//Запретим всё, что надо запретить.
	CommonVars(const CommonVars&) = delete;
	CommonVars(CommonVars&&) = delete;
	CommonVars& operator=(const CommonVars&) = delete;
	CommonVars& operator=(CommonVars&&) = delete;
	//Остаётся только способ получить инстанс.
	static CommonVars& Instance()
	{
		static CommonVars instance;
		return instance;
	}

	//Доступ к полям.

	//appPath
	const std::string& getAppPath() const { return appPath_; }
	void setAppPath(const std::string &appPath) { appPath_ = appPath; }
	//currPath
	const std::string& getCurrPath() const { return currPath_; }
	void setCurrPath(const std::string &currPath) { currPath_ = currPath; }
	//dataPath
	const std::string& getDataPath() const { return dataPath_; }
	void setDataPath(const std::string &dataPath) { dataPath_ = dataPath; }

private:
	//Конструктор и деструктор по умолчанию нужны как минимум самому объекту но не должны
	//быть доступны извне.
	CommonVars() {};
	~CommonVars() {};

	//Приватные поля. По умолчанию всё пусто, заполняться должны из глобальной фикстуры.
	std::string appPath_ = "";	//Путь, по которому лежит исполнимый файл тестов.
	std::string currPath_ = "";  //Путь, который являлся текущим на момент запуска тестов.
	std::string dataPath_ = "";	//Путь, по которому должны лежать тестовые данные
};