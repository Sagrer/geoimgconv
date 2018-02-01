@rem Этот скрипт можно отредактировать чтобы настроить программу.
@rem medfilter.aperture - апертура (окно) медианного фильтра.
@rem medfilter.threshold - порог (в метрах). Разделитель дробной части - строго точка!
@rem medfilter.margintype - краевое заполнение. Может быть mirror - зеркальное и simple - тупое копирование одной краевой точки.
@rem Последние две опции - имена исходного файла и файла с результатом.
geoimgconv --medfilter.aperture=101 --medfilter.threshold=0.5 --medfilter.margintype=mirror input.tif output.tif
@pause