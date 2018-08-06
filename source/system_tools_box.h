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

//Небольшой сборник функций для получения информации о состоянии системы.
//Синглтон - не нужен, глобальный объект - тем более, поэтому статический класс.

//Если требуется работать с классом из нескольких потоков - необходимо
//предварительно выполнить StringsToolsBox::InitEncodings() до запуска остальных
//потоков, поскольку реализация зависит от StringsToolsBox и только в этом случае
//всё будет потокобезопасно.

#include <string>

namespace geoimgconv
{

//Структура для метода GetSysResInfo.
struct SysResInfo
{
	unsigned int cpuCoresNumber;	//Количество ядер процессора.
	unsigned long long systemMemoryFullSize;	//Общее количество (без свопа) оперативной памяти.
	unsigned long long systemMemoryFreeSize;	//Количество свободной (без свопа) оперативной памяти.
	unsigned long long maxProcessMemorySize;	//Сколько памяти может адресовать данный процесс.
};

//Основной класс.
class SystemToolsBox
{
	public:
		//Экземпляр создавать запрещено.
		SystemToolsBox() = delete;
		~SystemToolsBox() = delete;

		//Возвращает число процессорных ядер или 0 если это количество получить не удалось.
		static const unsigned int GetCpuCoresNumber();

		//Возвращает общее количество оперативной памяти (без свопа) в системе или 0 при ошибке.
		static const unsigned long long GetSystemMemoryFullSize();

		//Возвращает количество свободной оперативной памяти (без свопа) в системе или 0 при ошибке.
		static const unsigned long long GetSystemMemoryFreeSize();

		//Возвращает максимальное количество памяти, которое вообще может потребить данный процесс.
		static const unsigned long long GetMaxProcessMemorySize();

		//Вернуть информацию о ресурсах системы - то же, что и несколько методов выше, но должно
		//работать быстрее чем последовательный вызов всех этих методов.
		static void GetSysResInfo(SysResInfo &infoStruct);

		//Узнать текущую ширину консоли. Вернёт 0 в случае ошибки.
		static unsigned short GetConsoleWidth();

		//Возвращает используемый в данной системе разделитель путей к файлам, в виде обычной строки.
		static const std::string& GetFilesystemSeparator();
		
	private:
		//Поля
		static std::string filesystemSeparator_;
};

//Сокращённое наименование.
using SysTB = SystemToolsBox;

}	//namespace geoimgconv
