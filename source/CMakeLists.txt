###########################################################
##                                                       ##
##                  GeoImageConverter                    ##
##       Преобразователь изображений с геоданными        ##
##    Copyright © 2017-2018 Александр (Sagrer) Гриднев   ##
##              Распространяется на условиях             ##
##                 GNU GPL v3 или выше                   ##
##                  см. файл gpl.txt                     ##
##                                                       ##
###########################################################

#Место для списка авторов данного конкретного файла (если изменения
#вносили несколько человек).

########################################################################

################################
##  Файл "проекта" для CMake  ##
################################

cmake_minimum_required(VERSION 3.2 FATAL_ERROR)

#Экспериментальная переменная для отключения генерации хрени в проектах CodeBlocks.
set(CMAKE_CODEBLOCKS_EXCLUDE_EXTERNAL_FILES ON)

#Подключаем локальную директорию с модулями CMake.
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake_modules")

#Включим требование к стандарту с++14
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#Будут использоваться тесты.
enable_testing()

#Проект.
project(geoimgconv VERSION 0.4.1 LANGUAGES CXX C)

##Для gcc особо указать отладочные флаги.
#if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
#	SET(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")
#	SET(CMAKE_C_FLAGS_DEBUG "-O0 -g")
#	#message("GCC debug flags were set!")
#else()
#	#message("GCC debug flags were not set!")
#endif()

#Ищем BOOST и добавляем путь линковки к нему - строго до add_executable.
#Причина - линкер в VisualStudio упорно не видит lib-файлы, если путь к ним не был
#указан явно в параметре линкера для дополнительных путей поиска, и ему совершенно
#наплевать на то, что потом ему скармливаются прямые пути. Поэтому для MSVC нужно
#делать явное link_directories. При этом считаем что в ${Boost_LIBRARY_DIRS}
#могут лежать вообще любые другие либы.
if(WIN32)
	#Имеет смысл включать статическую линковку, ибо копировать и вообще тянуть с
	#собой dll-ки гемор, а толку никакого - всё равно эти либы разделяться с другими
	#приложениями точно не будут.
	#Важно убрать компонент Boost::dynamic_linking ниже если линковка статическая.
	#Сейчас - отключено, т.к. неправильная статика, как и неправильная динамика дают
	#лютые глюки в boost::locale и фасетах.
	set(Boost_USE_STATIC_LIBS OFF)
endif()
find_package(Boost COMPONENTS locale system thread filesystem program_options unit_test_framework REQUIRED)
if(MSVC)
	link_directories(${Boost_LIBRARY_DIRS})
endif()

#Список входящих в проект файлов. Основная часть файлов выделена в отдельную статическкую
#библиотеку чтобы cotire не бунтовал при компилляции одних и тех же cpp в разные executable.
#С этой же библиотекой будут линковаться и все прочие библиотеки. В основном executable
#должен остаться только main.
add_library(geoimgconv_lib STATIC
	strings_tools_box.cpp
	strings_tools_box.h
	system_tools_box.cpp
	system_tools_box.h
	appui_console.cpp
	appui_console.h
	appui_console_callback.cpp
	appui_console_callback.h
	alt_matrix_base.h
	alt_matrix.cpp
	alt_matrix.h
	errors.cpp
	errors.h
	base_filter.h
	median_filter.h
	median_filter_stub.cpp
	median_filter_stub.h
	median_filter_stupid.cpp
	median_filter_stupid.h
	median_filter_huang.cpp
	median_filter_huang.h
	median_filter_base.cpp
	median_filter_base.h
	pixel_type_speciefic_filter_base.h
	pixel_type_speciefic_filter.cpp
	pixel_type_speciefic_filter.h
	common.cpp
	common.h
	call_back_base.h
	app_config.cpp
	app_config.h)
add_executable(geoimgconv 
	main.cpp)

#Список файлов с юнит-тестами.
add_executable(geoimgconv_tests
	tests/main_tests.cpp
	tests/median_filter_tests.cpp
	tests/image_comparer.cpp
	tests/image_comparer.h
	tests/common_vars.h)
	
#Линкуем основную библиотеку к двум получающимся бинарникам.
target_link_libraries(geoimgconv geoimgconv_lib)
target_link_libraries(geoimgconv_tests geoimgconv_lib)
	
#Добавляем тест в виде запуска тестового приложения.
add_test(geoimgconv_tests geoimgconv_tests)
	
#Подключаем библиотеки
##Curses - пока не нужно.
#if(WIN32)
#	set(CURSES_NEED_WIDE TRUE)
#	find_package(Curses)
#	if(CURSES_FOUND)
#		include_directories(${CURSES_INCLUDE_DIRS})
#		target_link_libraries(geoimgconv ${CURSES_LIBRARIES})
#	else()
#		find_package(PDCurses REQUIRED)
#		include_directories(${PDCURSES_INCLUDE_DIRS})
#		target_link_libraries(geoimgconv ${PDCURSES_LIBRARIES})
#	endif()
#else()
#	set(CURSES_NEED_WIDE TRUE)
#	find_package(Curses REQUIRED)
#	include_directories(${CURSES_INCLUDE_DIRS})
#	target_link_libraries(geoimgconv ${CURSES_LIBRARIES})
#endif()

#Boost.
#dynamic_linking - для принудительной динамической линковки под Visual Studio.
include_directories(${Boost_INCLUDE_DIRS})
target_link_libraries(geoimgconv_lib Boost::locale Boost::system Boost::thread Boost::filesystem
	Boost::program_options Boost::dynamic_linking)
#К тестовому проекту нужно прилинковать дополнительно Boost.Test.
target_link_libraries(geoimgconv_tests Boost::unit_test_framework)
#При необходимости добавим define, отключающий ругань буста на компиллятор.
if(BOOST_SUPR_OUTDATED)
	add_definitions(-DBOOST_CONFIG_SUPPRESS_OUTDATED_MESSAGE)
endif()
#А вот без этого нормально не работает Boost.DLL:
target_link_libraries(geoimgconv_lib ${CMAKE_DL_LIBS})

##ICU - скорее всего явно подключать его не нужно.
#find_package(ICU COMPONENTS uc dt in io tu test REQUIRED)
#include_directories(${ICU_INCLUDE_DIRS})
#target_link_libraries(geoimgconv ${ICU_LIBRARIES})

#GDAL
find_package(GDAL REQUIRED)
include_directories(${GDAL_INCLUDE_DIR})
target_link_libraries(geoimgconv_lib ${GDAL_LIBRARY})

if(WIN32)
	if("${CMAKE_SIZEOF_VOID_P}" STREQUAL "4")
		#При сборке 32-битной версии под студию включим использование более 2гигов памяти.
		if(MSVC)
			SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /LARGEADDRESSAWARE")
		elseif(MINGW)
			#То же самое для 32-битной mingw. -Wl - чтобы g++ передал опцию именно линкеру.
			SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--large-address-aware")
		endif()
	endif()
endif()

#Подключаем модуль cotire от Sascha Kratky для использования прекомпилированных заголовков.
include(cotire)
#Запрещаем cotire использовать unity build т.к. он ускоряет скорее только полную сборку а не
#сборку отдельных изменившихся файлов + создаёт кучу проблем.
set_target_properties(geoimgconv PROPERTIES COTIRE_ADD_UNITY_BUILD FALSE)
set_target_properties(geoimgconv_lib PROPERTIES COTIRE_ADD_UNITY_BUILD FALSE)
set_target_properties(geoimgconv_tests PROPERTIES COTIRE_ADD_UNITY_BUILD FALSE)
#Добавляем в прекомпилированный заголовок редко изменяющиеся заголовки из состава файлов проекта.
set_target_properties(geoimgconv PROPERTIES
	COTIRE_PREFIX_HEADER_INCLUDE_PATH "${CMAKE_SOURCE_DIR}/strings_tools_box.h")
set_target_properties(geoimgconv PROPERTIES
	COTIRE_PREFIX_HEADER_INCLUDE_PATH "${CMAKE_SOURCE_DIR}/system_tools_box.h")
set_target_properties(geoimgconv_lib PROPERTIES
	COTIRE_PREFIX_HEADER_INCLUDE_PATH "${CMAKE_SOURCE_DIR}/strings_tools_box.h")
set_target_properties(geoimgconv_lib PROPERTIES
	COTIRE_PREFIX_HEADER_INCLUDE_PATH "${CMAKE_SOURCE_DIR}/system_tools_box.h")
set_target_properties(geoimgconv_tests PROPERTIES
	COTIRE_PREFIX_HEADER_INCLUDE_PATH "${CMAKE_SOURCE_DIR}/strings_tools_box.h")
	set_target_properties(geoimgconv_tests PROPERTIES
	COTIRE_PREFIX_HEADER_INCLUDE_PATH "${CMAKE_SOURCE_DIR}/system_tools_box.h")
#Запрещаем включать заголовки Boost для тестов, иначе всё ломается. Cotire не умеет тут в регулярки,
#приходится приклеивать путь ко всем возможным путям-инклудам в цикле.
get_property(ALL_INCLUDE_DIRECTORIES DIRECTORY PROPERTY INCLUDE_DIRECTORIES)
set(COTIRE_TEST_IGNORE "")
foreach(CURR_DIR ${ALL_INCLUDE_DIRECTORIES})
	list(APPEND COTIRE_TEST_IGNORE "${CURR_DIR}/boost;")
endforeach(CURR_DIR)
set_target_properties(geoimgconv_tests PROPERTIES
	COTIRE_PREFIX_HEADER_IGNORE_PATH "${COTIRE_TEST_IGNORE}")
#Включаем cotire для всего, что мы тут собираем.
cotire(geoimgconv)
cotire(geoimgconv_lib)
cotire(geoimgconv_tests)
