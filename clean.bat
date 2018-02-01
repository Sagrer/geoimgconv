@echo off
SET /P "INPUT=Delete all generated files (y/n)? > "
If /I "%INPUT%"=="y" goto yes 
If /I "%INPUT%"=="n" goto no
:yes
rmdir /S /Q .\cb_build_debug
rmdir /S /Q .\cb_build_release
rmdir /S /Q .\vs2008_build
rmdir /S /Q .\vs2015_build
:no
