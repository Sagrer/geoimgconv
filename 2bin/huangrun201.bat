@rem Скрипт запускает преобразование input.tif в output.tif с окном 101
geoimgconv --medfilter.aperture=201 --medfilter.algo=huang input.tif output.tif
@pause