#!/bin/sh

#need pacman -Ss base-devel to work
#Применяем патчи, без которого под mingw gdal не хочет собираться.
#Вот эти исправления - взяты из https://github.com/Alexpux/MINGW-packages/blob/master/mingw-w64-gdal/PKGBUILD
sed -i "s|/usr/local|${mingw_prfx}|g" configure.ac
sed -i "s|/usr|${mingw_prfx}|g" configure.ac
sed -i "s|mandir='\${prefix}/man'|mandir='\${prefix}/share/man'|g" configure.ac
for p in m4/*.m4
do
  sed -i "s|/usr|${mingw_prfx}|g" $p
done
sed -i -e 's@uchar@unsigned char@' frmts/jpeg2000/jpeg2000_vsil_io.cpp
#Ломаем детектирование iconv
#rm ./m4/iconv.m4
#Генерируем новый конфигурационный скрипт.
./autogen.sh
#Попытаемся сломать детектирование iconv, т.к. линковаться с дополнительной зависимостью не хочется.
#sed -i configure.ac -e "s|AM_ICONV||g"
#Теперь конфигурируем.
CFLAGS+=" -fno-strict-aliasing"
./configure \
    --prefix=${RELEASEDIR} \
    --without-poppler \
    --without-webp \
    --without-spatialite \
    --without-liblzma \
    --without-python \
	--without-libiconv-prefix \
	--without-sqlite3 \
	--without-kea \
	--without-expat \
	--with-libz=internal \
	--disable-rpath \
	--disable-debug \
	--enable-release \
	--without-perl

#И накатим сверху ещё патчей.
#Вот эти исправления - взяты из https://github.com/Alexpux/MINGW-packages/blob/master/mingw-w64-gdal/PKGBUILD
sed -i GDALmake.opt -e "s|EXE_DEP_LIBS.*|EXE_DEP_LIBS = \$\(GDAL_SLIB\)|g"
sed -i GNUmakefile -e "s|\$(GDAL_ROOT)\/||g"
#А вот это - отсебятина. Явно уберём флаг, который заставляет компилировать всё с отладочной информацией. Хз почему этот флаг включается вообще
#в якобы релизной версии.
sed -i GDALmake.opt -e "s| -g||g"
#Сборку с libiconv надо тоже отключить. Ключа для configure тут, увы, не предусмотрено, придётся в лоб.
#sed -i GDALmake.opt -e "s|LIBICONV.*=.*-liconv|LIBICONV = |g"
#sed -i port/cpl_config.h -e "s|#define HAVE_ICONV 1|#define HAVE_ICONV 0|g"