#Мейкфайл, который тупо запускает cmake и сам почти нифига не делает.
#Но оно же привычно - make-ать только что распакованный исходник.
#Пусть будет! :)
#Применять полагается в этих ваших линуксах, ибо под масдаем вместо make
#проще использовать bat-скрипты.

#Переменные
BUILDDIR := ./build_unix_release
CB_DEBUG_BUILDDIR := ./build_unix_codeblocks_debug

#Отключаем проверку наличия файлов у команд-действий
.PHONY : all clean korovan cb_debug install uninstall

#Неявные правила - лесом.
.SUFFIXES :

#Цель по умолчанию. Запускает cmake и потом сборку.
all : $(BUILDDIR)/prepare.tmp
	cd $(BUILDDIR); cmake -G "Unix Makefiles" -D CMAKE_BUILD_TYPE=Release ../source
	cd $(BUILDDIR); make
	
#Отладочная и разрабоччицкая цель для CodeBlocks.
cb_debug : $(CB_DEBUG_BUILDDIR)/prepare.tmp
	cd $(CB_DEBUG_BUILDDIR); cmake -G "CodeBlocks - Unix Makefiles" -D CMAKE_BUILD_TYPE=Debug ../source
	cd $(CB_DEBUG_BUILDDIR); make

#Подготовительные цели. Подразумевают в том числе создание временного файла.
#Если файл существует - директория точно существует, и её не надо создавать :).
$(BUILDDIR)/prepare.tmp :
	if [ ! -d "$(BUILDDIR)" ];then mkdir $(BUILDDIR); fi
	touch $(BUILDDIR)/prepare.tmp

$(CB_DEBUG_BUILDDIR)/prepare.tmp :
	if [ ! -d "$(CB_DEBUG_BUILDDIR)" ];then mkdir $(CB_DEBUG_BUILDDIR); fi
	touch $(CB_DEBUG_BUILDDIR)/prepare.tmp

#Цель для очистки
clean :
	rm -rf $(BUILDDIR)
	rm -rf $(CB_DEBUG_BUILDDIR)

install:
	cp -f ./build_unix_release/geoimgconv /usr/local/bin/
	chmod uga+x /usr/local/bin/geoimgconv

uninstall:
	rm -f /usr/local/bin/geoimgconv

korovan :
#А ещё можно грабить корованы.
	@echo КОРОВАН ОГРАБЛЕН!
