@rem Этот скрипт можно отредактировать чтобы настроить программу.
@rem medfilter.aperture - апертура (окно) медианного фильтра.
@rem medfilter.threshold - порог (в метрах). Разделитель дробной части - строго точка!
@rem medfilter.margintype - краевое заполнение. Может быть mirror - зеркальное и simple - тупое копирование одной краевой точки.
@rem memmode задаёт режим использования памяти. auto означает что выбор сколько съесть памяти оставляется на усмотрение программы. Можно написать например --memmode=limit700m - что означает потреблять не больше 700 мегабайт. Подробнее про эту опцию - см. файл readme.txt.
@rem Последние две опции - имена исходного файла и файла с результатом.
geoimgconv --medfilter.aperture=101 --medfilter.threshold=0.5 --medfilter.margintype=mirror --memmode=auto input.tif output.tif
@pause