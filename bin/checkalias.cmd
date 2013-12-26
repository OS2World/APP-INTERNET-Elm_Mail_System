@echo off

if not *%1 == * goto work

echo ÿ
echo Usage: checkalias alias [alias ...]
goto exit

:work
elm -c %1 %2 %3 %4 %5 %6 %7 %8 %9

:exit
