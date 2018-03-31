###########################################################
##                                                       ##
##                  GeoImageConverter                    ##
##       Преобразователь изображений с геоданными        ##
##       Copyright © 2017 Александр (Sagrer) Гриднев     ##
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

#Проект.
project(geoimgconv VERSION 0.0.1 LANGUAGES CXX C)

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
find_package(Boost COMPONENTS locale system chrono thread filesystem date_time program_options REQUIRED)
if(MSVC)
	link_directories(${Boost_LIBRARY_DIRS})
endif()

#Список входящих в проект файлов.
add_executable(geoimgconv 
	main.cpp
	small_tools_box.cpp
	small_tools_box.h
	appui_console.cpp
	appui_console.h
	alt_matrix.cpp
	alt_matrix.h
	errors.cpp
	errors.h
	base_filter.h
	median_filter.cpp
	median_filter.h
	common.cpp
	common.h
	app_config.cpp
	app_config.h)
	
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

#Boost. dynamic_linking - для принудительной динамической линковки под Visual Studio.
include_directories(${Boost_INCLUDE_DIRS})
target_link_libraries(geoimgconv Boost::locale Boost::system Boost::chrono Boost::thread Boost::filesystem
	Boost::date_time Boost::program_options Boost::dynamic_linking)
#При необходимости добавим define, отключающий ругань буста на компиллятор.
if(BOOST_SUPR_OUTDATED)
	add_definitions(-DBOOST_CONFIG_SUPPRESS_OUTDATED_MESSAGE)
endif()

##ICU - скорее всего явно подключать его не нужно.
#find_package(ICU COMPONENTS uc dt in io tu test REQUIRED)
#include_directories(${ICU_INCLUDE_DIRS})
#target_link_libraries(geoimgconv ${ICU_LIBRARIES})

#GDAL
find_package(GDAL REQUIRED)
include_directories(${GDAL_INCLUDE_DIR})
target_link_libraries(geoimgconv ${GDAL_LIBRARY})

#При сборке 32-битной версии под студию включим использование более 2гигов памяти.
if(WIN32)
	if(NOT "${CMAKE_GENERATOR}" MATCHES "(Win64|IA64)")
		if(MSVC)
			SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /LARGEADDRESSAWARE")
		endif()
	endif()
endif()
